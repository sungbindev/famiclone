/*
 * ------------------------------------------------------------------------
 * 名称：process-core-gateway.ts
 * 说明：核心网关的实现：启动核心进程，经消息与它交换请求、响应与事件；核心进程意外退出时记录错误、拒绝进行中的请求并重新启动。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-13
 * 备注：一分钟内最多重启 3 次，超过后状态改为不可用；核心启动失败（如数据库打不开）不重启。
 *       核心进程按收到的顺序处理消息，所以启动完成前发出的请求也会在启动后处理。
 *       同时是画面与输入端口的入口（CoreStreamSink）：端口随消息转移给当前的核心进程，每次启动（含重启）通知订阅者。
 * ------------------------------------------------------------------------
 */

import { randomUUID } from 'node:crypto';
import type { CoreGateway, CoreRequestOptions } from '../../contracts/core-gateway/core-gateway';
import { CORE_ERROR_CODES, CoreRequestError, type CoreErrorInfo, type CoreHostInbound, type CoreHostOutbound, type CoreStatus } from '../../contracts/core-gateway/core-messages';
import type { CoreEventName, CoreParams, CoreRequestName, CoreResult } from '../../contracts/core-gateway/generated/core-api';
import type { ErrorLogger } from '../../contracts/logging/error-logger';
import type { CoreStreamSink, StreamPortPair } from '../../contracts/stream-ports/stream-ports';
import type { CoreProcess } from '../../platform/core-process/core-process';

/** 统计重启次数的时间窗口。 */
const RESTART_WINDOW_MS = 60_000;
/** 时间窗口内最多重启的次数。 */
const MAX_RESTARTS = 3;
/** 停止时等待核心进程退出的最长时间。 */
const STOP_TIMEOUT_MS = 5_000;

/** 用户可见的说明。 */
const TEXT = {
  exited: '核心进程已退出，正在重新启动',
  gaveUp: '核心进程多次异常退出，已停止重新启动',
  stopped: '核心已停止',
  cancelled: '请求已取消',
  unavailable: '核心不可用'
} as const;

/** 网关的创建参数。 */
export interface ProcessCoreGatewayOptions {
  /** 启动一个核心进程。 */
  readonly startProcess: () => CoreProcess;
  /** 原生模块路径。 */
  readonly nativePath: string;
  /** 用户数据目录。 */
  readonly userDataPath: string;
  /** 应用版本。 */
  readonly appVersion: string;
  /** 预装的 mapper 插件目录；没有时省略。 */
  readonly bundledPluginsPath?: string;
  /** 错误日志。 */
  readonly logger: ErrorLogger;
  /** 当前时间（毫秒），测试可替换。 */
  readonly now?: () => number;
  /** 生成请求标识，测试可替换。 */
  readonly createId?: () => string;
}

/** 进行中的请求。 */
interface PendingRequest {
  readonly resolve: (result: unknown) => void;
  readonly reject: (error: CoreRequestError) => void;
  readonly detach: () => void;
}

/** 经核心进程实现的核心网关。 */
export class ProcessCoreGateway implements CoreGateway, CoreStreamSink {
  private process: CoreProcess | undefined;
  private current: CoreStatus = { state: 'starting' };
  private stopping = false;
  private startFailed = false;
  private restarts: number[] = [];
  private exitWaiter: (() => void) | undefined;
  private readonly pending = new Map<string, PendingRequest>();
  private readonly eventListeners = new Set<(name: CoreEventName, data: unknown) => void>();
  private readonly startedListeners = new Set<() => void>();
  private readonly now: () => number;
  private readonly createId: () => string;

  constructor(private readonly options: ProcessCoreGatewayOptions) {
    this.now = options.now ?? Date.now;
    this.createId = options.createId ?? randomUUID;
  }

  /** 启动核心进程。 */
  start(): void {
    this.stopping = false;
    this.spawn();
  }

  /** 停止核心进程：请求核心停止，超时后结束进程。 */
  async stop(): Promise<void> {
    const child = this.process;
    this.stopping = true;
    if (child === undefined) return;
    await new Promise<void>((resolve) => {
      const timer = setTimeout(() => {
        child.kill();
        resolve();
      }, STOP_TIMEOUT_MS);
      this.exitWaiter = () => {
        clearTimeout(timer);
        resolve();
      };
      this.post({ type: 'stop' });
    });
  }

  request<N extends CoreRequestName>(name: N, params: CoreParams<N>, options?: CoreRequestOptions): Promise<CoreResult<N>> {
    return new Promise<CoreResult<N>>((resolve, reject) => {
      if (this.current.state === 'unavailable' || this.process === undefined || this.stopping) {
        reject(new CoreRequestError(this.current.state === 'unavailable' ? this.current.error : { code: CORE_ERROR_CODES.unavailable, message: TEXT.unavailable }));
        return;
      }
      const signal = options?.signal;
      if (signal?.aborted) {
        reject(new CoreRequestError({ code: CORE_ERROR_CODES.cancelled, message: TEXT.cancelled }));
        return;
      }
      const id = this.createId();
      const onAbort = (): void => {
        const entry = this.pending.get(id);
        if (entry === undefined) return;
        this.pending.delete(id);
        this.post({ type: 'cancel', id });
        entry.reject(new CoreRequestError({ code: CORE_ERROR_CODES.cancelled, message: TEXT.cancelled }));
      };
      signal?.addEventListener('abort', onAbort, { once: true });
      this.pending.set(id, {
        resolve: (result) => resolve(result as CoreResult<N>),
        reject,
        detach: () => signal?.removeEventListener('abort', onAbort)
      });
      this.post({ type: 'request', id, name, params });
    });
  }

  onEvent(listener: (name: CoreEventName, data: unknown) => void): () => void {
    this.eventListeners.add(listener);
    return () => this.eventListeners.delete(listener);
  }

  status(): CoreStatus {
    return this.current;
  }

  attachStreams(ports: StreamPortPair): boolean {
    if (this.process === undefined || this.stopping || this.startFailed) return false;
    this.process.postMessage({ type: 'streams' } satisfies CoreHostInbound, [ports.frames, ports.input]);
    return true;
  }

  onCoreStarted(listener: () => void): () => void {
    this.startedListeners.add(listener);
    return () => this.startedListeners.delete(listener);
  }

  private spawn(): void {
    this.startFailed = false;
    this.setStatus({ state: 'starting' });
    const child = this.options.startProcess();
    this.process = child;
    child.onMessage((message) => this.receive(message as CoreHostOutbound));
    child.onExit((code) => this.exited(child, code));
    const { nativePath, userDataPath, appVersion, bundledPluginsPath } = this.options;
    this.post({ type: 'start', nativePath, userDataPath, appVersion, ...(bundledPluginsPath === undefined ? {} : { bundledPluginsPath }) });
    for (const listener of this.startedListeners) {
      try {
        listener();
      } catch (error) {
        this.options.logger.write({ process: 'main', code: CORE_ERROR_CODES.internal, message: `核心启动通知失败：${error instanceof Error ? error.message : String(error)}` });
      }
    }
  }

  private post(message: CoreHostInbound): void {
    this.process?.postMessage(message);
  }

  private receive(message: CoreHostOutbound): void {
    if (typeof message !== 'object' || message === null) return;
    switch (message.type) {
      case 'ready':
        this.setStatus({ state: 'ready', coreVersion: message.coreVersion });
        return;
      case 'startFailed':
        this.startFailed = true;
        this.options.logger.write({ process: 'main', code: message.error.code, message: `核心启动失败：${message.error.message}` });
        this.setStatus({ state: 'unavailable', error: message.error });
        this.rejectAll(message.error);
        this.process?.kill();
        return;
      case 'response': {
        const entry = this.pending.get(message.id);
        if (entry === undefined) return;
        this.pending.delete(message.id);
        entry.detach();
        if (message.ok) entry.resolve(message.result);
        else entry.reject(new CoreRequestError(message.error));
        return;
      }
      case 'event':
        for (const listener of this.eventListeners) listener(message.name as CoreEventName, message.data);
        return;
      default:
        return;
    }
  }

  private exited(child: CoreProcess, code: number): void {
    if (child !== this.process) return;
    this.process = undefined;
    if (this.stopping) {
      this.rejectAll({ code: CORE_ERROR_CODES.unavailable, message: TEXT.stopped });
      this.setStatus({ state: 'unavailable', error: { code: CORE_ERROR_CODES.unavailable, message: TEXT.stopped } });
      this.exitWaiter?.();
      this.exitWaiter = undefined;
      return;
    }
    if (this.startFailed) return;

    this.options.logger.write({ process: 'main', code: CORE_ERROR_CODES.unavailable, message: `核心进程异常退出，退出码 ${code}` });
    this.rejectAll({ code: CORE_ERROR_CODES.unavailable, message: TEXT.exited });
    const now = this.now();
    this.restarts = this.restarts.filter((time) => now - time < RESTART_WINDOW_MS);
    if (this.restarts.length >= MAX_RESTARTS) {
      const error = { code: CORE_ERROR_CODES.unavailable, message: TEXT.gaveUp };
      this.options.logger.write({ process: 'main', code: error.code, message: error.message });
      this.setStatus({ state: 'unavailable', error });
      return;
    }
    this.restarts.push(now);
    this.spawn();
  }

  private rejectAll(error: CoreErrorInfo): void {
    const entries = [...this.pending.values()];
    this.pending.clear();
    for (const entry of entries) {
      entry.detach();
      entry.reject(new CoreRequestError(error));
    }
  }

  private setStatus(status: CoreStatus): void {
    this.current = status;
  }
}
