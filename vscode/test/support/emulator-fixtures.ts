// ------------------------------------------------------------------------
// 名称：emulator-fixtures.ts
// 说明：模拟器测试的公共替身：假的调试会话（记录请求、按方法返回预设结果）、假的调试连接、假的进程
//       启动、假的设置，以及用它们组装好的模拟器操作。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-22
// 备注：文件系统与目标描述读取用真实的实现（读测试临时目录与扩展自带的目标定义），其余外部能力都是
//       替身。
// ------------------------------------------------------------------------

import * as path from 'node:path';
import type { ConnectToEmberOptions, DebugConnector } from '../../src/contracts/debug-client/debug-connector';
import type { ConnectOptions, DebugSession } from '../../src/contracts/debug-client/debug-session';
import type { Endpoint, TargetOptions } from '../../src/contracts/debug-client/endpoint';
import type { EmulatorSettings } from '../../src/contracts/emulator/emulator-settings';
import type { EmulatorProcess, EmulatorProcessLauncher } from '../../src/contracts/emulator/process-launcher';
import { NodeFileSystem } from '../../src/infra/files/node-file-system';
import { createExtensionResources } from '../../src/infra/resources/extension-resources-factory';
import { CoreEmulatorOperations } from '../../src/logic/emulator/core-emulator-operations';
import { FileTargetProfileReader } from '../../src/logic/targets/file-target-profile-reader';
import { EXTENSION_DIRECTORY } from './build-fixtures';

/** 默认的 CPU 状态。 */
export const CPU_STATE = { states: [{ module: 'cpu', writable: true, fields: [{ name: 'pc', value: 0xc003 }, { name: 'a', value: 5 }] }] };

/** 运行状态的预设结果。 */
export const STATUS = { power: 'on', mode: 'paused', frame: 0, cycles: 0, speed: 100, mouse: false };

/** 一条记录下来的请求。 */
export interface RecordedCall {
  method: string;
  params: unknown;
}

/** 假的调试会话：记录请求，按方法返回预设的结果；没有预设的方法抛错。 */
export interface FakeClient extends DebugSession {
  /** 收到的请求，关闭连接记为 close。 */
  readonly calls: RecordedCall[];
  /** 让连接“断开”：兑现 closed。 */
  disconnect(): void;
}

/**
 * 创建假的调试会话。
 * @param handlers 方法名到结果或函数；没给 runtime.status 时用预设的 STATUS。
 * @returns 假会话。
 */
export function createFakeClient(handlers: Record<string, unknown> = {}): FakeClient {
  const responses: Record<string, unknown> = { 'runtime.status': STATUS, ...handlers };
  const calls: RecordedCall[] = [];
  let release: ((info: { code: number; reason: string }) => void) | undefined;
  const closed = new Promise<{ code: number; reason: string }>((resolve) => {
    release = resolve;
  });
  const client = {
    calls,
    info: { appVersion: 'test', protocol: 1 },
    closed,
    disconnect: () => release?.({ code: 1000, reason: '' }),
    close(): void {
      calls.push({ method: 'close', params: null });
    },
    async request(method: string, params?: unknown): Promise<unknown> {
      calls.push({ method, params });
      const handler = responses[method];
      if (handler === undefined) throw new Error(`没有预设的请求 ${method}`);
      return typeof handler === 'function' ? (handler as (value: unknown) => unknown)(params) : handler;
    }
  };
  return client as unknown as FakeClient;
}

/** 假的调试连接：连接请求记在 connects 里，结果由 next 决定。 */
export class FakeConnector implements DebugConnector {
  /** 收到的连接选项。 */
  readonly connects: ConnectToEmberOptions[] = [];
  /** 下一次连接要给的会话；没有时抛出 failure。 */
  next: DebugSession | undefined;
  /** 没有会话可给时抛出的错误。 */
  failure: Error = new Error('连不上');

  /** @inheritdoc */
  async connect(options: ConnectOptions): Promise<DebugSession> {
    return this.connectToEmber(options);
  }

  /** @inheritdoc */
  async connectToEmber(options: ConnectToEmberOptions = {}): Promise<DebugSession> {
    this.connects.push(options);
    if (this.next === undefined) throw this.failure;
    return this.next;
  }

  /** @inheritdoc */
  readEndpoint(_options?: TargetOptions): Endpoint {
    throw new Error('测试里不读端点文件');
  }
}

/** 假的设置。 */
export class FakeSettings implements EmulatorSettings {
  /** Ember 路径。 */
  path: string | undefined;
  /** 调试端口。 */
  port: number;

  /** @param executable Ember 路径。 @param port 调试端口。 */
  constructor(executable: string | undefined = undefined, port = 5699) {
    this.path = executable;
    this.port = port;
  }

  /** @inheritdoc */
  executablePath(): string | undefined {
    return this.path;
  }

  /** @inheritdoc */
  debugPort(): number {
    return this.port;
  }
}

/** 假的进程启动：记录启动与结束。 */
export class FakeProcesses implements EmulatorProcessLauncher {
  /** 启动的进程：可执行文件、参数与是否在退出时算失败。 */
  readonly started: Array<{ executable: string; args: readonly string[]; failOnExit: boolean }> = [];
  /** 结束的进程号。 */
  readonly terminated: number[] = [];
  /** 被问过空闲的端口。 */
  readonly checkedPorts: number[] = [];
  /** 被杀死的假进程数。 */
  kills = 0;
  /** 端口检查要抛出的错误。 */
  portError: Error | undefined;
  /** 进程启动后要立即失败的错误。 */
  startError: Error | undefined;

  /** @inheritdoc */
  async assertPortFree(port: number): Promise<void> {
    this.checkedPorts.push(port);
    if (this.portError !== undefined) throw this.portError;
  }

  /** @inheritdoc */
  userDataDirectory(port: number): string {
    return path.join('fake-user-data', String(port));
  }

  /** @inheritdoc */
  prepareUserDataDirectory(port: number): string {
    return this.userDataDirectory(port);
  }

  /** @inheritdoc */
  start(executable: string, args: readonly string[], failOnExit: boolean): EmulatorProcess {
    this.started.push({ executable, args, failOnExit });
    const failure = this.startError === undefined ? new Promise<never>(() => undefined) : Promise.reject(this.startError);
    failure.catch(() => undefined);
    return { pid: 4242, failure, kill: () => { this.kills += 1; } };
  }

  /** @inheritdoc */
  async confirmStarted(child: EmulatorProcess): Promise<void> {
    await Promise.race([child.failure, Promise.resolve()]);
  }

  /** @inheritdoc */
  terminate(pid: number): boolean {
    this.terminated.push(pid);
    return true;
  }

  /** @inheritdoc */
  createToken(): string {
    return 'test-token';
  }
}

/** 组装好的模拟器操作与它用到的替身。 */
export interface EmulatorHarness {
  readonly operations: CoreEmulatorOperations;
  readonly connector: FakeConnector;
  readonly settings: FakeSettings;
  readonly processes: FakeProcesses;
  /** 运行一个操作。 */
  run(operation: string, params?: Record<string, unknown>): Promise<Record<string, unknown>>;
  /** 用假会话连上（经 connect 操作），并清掉连接过程里记下的请求。 */
  connect(client: FakeClient): Promise<void>;
}

/**
 * 组装模拟器操作：真实的文件系统与目标描述读取，替身的连接、设置与进程启动。
 * @returns 操作与替身。
 */
export function createEmulatorHarness(): EmulatorHarness {
  const files = new NodeFileSystem();
  const resources = createExtensionResources(EXTENSION_DIRECTORY);
  const connector = new FakeConnector();
  const settings = new FakeSettings();
  const processes = new FakeProcesses();
  const operations = new CoreEmulatorOperations({ files, profiles: new FileTargetProfileReader(files, resources), connector, settings, processes });
  return {
    operations,
    connector,
    settings,
    processes,
    run: (operation, params = {}) => operations.run(operation, params),
    async connect(client) {
      connector.next = client;
      await operations.run('connect', {});
      client.calls.length = 0;
    }
  };
}
