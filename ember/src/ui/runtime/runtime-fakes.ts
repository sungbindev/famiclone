/*
 * ------------------------------------------------------------------------
 * 名称：runtime-fakes.ts
 * 说明：运行页测试用的假核心接口：按预加载脚本的接口形状实现 runtime、cartridges、output 请求与主进程的打开卡带请求，保存机器状态，记录每次调用，可以发出核心事件与核心运行状态变化。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-14
 * 备注：只供测试使用（不含真实的核心逻辑，只做运行页需要的最简单的状态转换）；不引用测试框架，所以放在 ui 层也满足依赖检查。
 * ------------------------------------------------------------------------
 */

import type { CoreStatus } from '../../contracts/core-gateway/core-messages';
import type { RuntimeStatusResult } from '../../contracts/runtime/generated/runtime';
import type { RecentCartridge, RuntimeBackend } from './runtime-controller';

/** 构造机器状态；machine 为 undefined 表示没有开机。 */
export function machineStatus(power: 'off' | 'on', mode: 'paused' | 'running', path?: string): RuntimeStatusResult {
  const base: RuntimeStatusResult = { power, mode, frame: 0, cycles: 0, speed: 100 };
  if (path === undefined) return base;
  return {
    ...base,
    machine: 'fc',
    cartridge: {
      path,
      mapper: 0,
      variant: 0,
      prgSize: 16384,
      chrSize: 8192,
      mirroring: 'horizontal',
      battery: false,
      crc32Prg: '0A1B2C3D',
      crc32All: '0A1B2C3D',
      pluginId: 'nrom',
      pluginName: 'NROM',
      pluginVersion: '1.0.0',
      host: 'fc',
      storage: []
    }
  };
}

/** 假的核心接口。 */
export class FakeRuntimeBackend {
  /** 当前机器状态；undefined 表示读不到（核心没有就绪），status 请求会被拒绝。 */
  status: RuntimeStatusResult | undefined = machineStatus('off', 'paused');
  /** 最近打开的清单（路径），最近的在前；对界面只给编号与文件名。 */
  recentPaths: string[] = [];
  volume = 100;
  muted = false;
  /** 下一次打开卡带时用户选的路径；undefined 表示用户取消。 */
  pickedPath: string | undefined;
  /** 按请求名登记的下一次失败的原因。 */
  readonly failures = new Map<string, unknown>();
  /** 每次请求的记录：请求名加参数。 */
  readonly calls: string[] = [];
  /** 当前核心运行状态。 */
  coreStatus: CoreStatus = { state: 'ready', coreVersion: '0.1.0' };

  private readonly eventListeners = new Set<(name: string, data: unknown) => void>();
  private readonly statusListeners = new Set<(status: CoreStatus) => void>();

  /** 预加载脚本接口里运行页用到的部分。 */
  readonly backend: RuntimeBackend = {
    core: {
      runtime: {
        status: () => this.request('runtime.status', {}, () => (this.status === undefined ? Promise.reject(new Error('核心未就绪')) : Promise.resolve(this.status))),
        powerOn: (params: unknown) =>
          this.request('runtime.powerOn', params, () => {
            this.update({ power: 'on', mode: (params as { resume?: boolean }).resume === true ? 'running' : 'paused' });
            return this.state();
          }),
        powerOff: () =>
          this.request('runtime.powerOff', {}, () => {
            this.update({ power: 'off', mode: 'paused' });
            return this.state();
          }),
        reset: (params: unknown) => this.request('runtime.reset', params, () => this.state()),
        pause: () =>
          this.request('runtime.pause', {}, () => {
            this.update({ mode: 'paused' });
            return this.state();
          }),
        resume: () =>
          this.request('runtime.resume', {}, () => {
            this.update({ mode: 'running' });
            return this.state();
          })
      },
      cartridges: {
        eject: () =>
          this.request('cartridges.eject', {}, () => {
            this.status = machineStatus('off', 'paused');
            this.emit('runtime.stateChanged', { power: 'off', mode: 'paused', reason: 'eject' });
            return this.state();
          })
      },
      output: {
        setVolume: (params: { volume?: number; muted?: boolean }) =>
          this.request('output.setVolume', params, () => {
            if (params.volume !== undefined) this.volume = params.volume;
            if (params.muted !== undefined) this.muted = params.muted;
            return { volume: this.volume, muted: this.muted };
          })
      }
    },
    main: {
      cartridges: {
        open: () =>
          this.request('cartridges.open', {}, () => {
            if (this.pickedPath === undefined) return { canceled: true };
            return { canceled: false, loaded: this.load(this.pickedPath) };
          }),
        recent: () => this.request('cartridges.recent', {}, () => ({ items: this.recentItems() })),
        openRecent: (params: { id: number }) =>
          this.request('cartridges.openRecent', params, () => {
            const path = this.recentPaths[params.id - 1];
            if (path === undefined) throw { code: 'cartridges.recentNotFound', message: '这个卡带已不在最近打开的清单里' };
            return { loaded: this.load(path) };
          })
      }
    },
    onCoreEvent: (listener: (name: string, data: unknown) => void) => {
      this.eventListeners.add(listener);
      return () => this.eventListeners.delete(listener);
    },
    onCoreStatusChange: (listener: (status: CoreStatus) => void) => {
      this.statusListeners.add(listener);
      return () => this.statusListeners.delete(listener);
    },
    getCoreStatus: () => Promise.resolve(this.coreStatus)
  } as unknown as RuntimeBackend;

  /** 发出一个核心事件。 */
  emit(name: string, data: unknown): void {
    for (const listener of this.eventListeners) listener(name, data);
  }

  /** 改变核心运行状态并通知订阅者。 */
  emitCoreStatus(status: CoreStatus): void {
    this.coreStatus = status;
    for (const listener of this.statusListeners) listener(status);
  }

  /** 有多少个核心事件订阅者。 */
  get eventListenerCount(): number {
    return this.eventListeners.size;
  }

  /** 编号从 1 开始，对应 recentPaths 的下标加 1。 */
  recentItems(): RecentCartridge[] {
    return this.recentPaths.map((path, index) => ({ id: index + 1, name: path.split(/[\\/]/).pop() ?? path }));
  }

  /** 装入卡带：记入最近打开、通电运行、发出 cartridges.loaded 事件。 */
  private load(path: string): { cartridge: NonNullable<RuntimeStatusResult['cartridge']>; state: { power: 'on'; mode: 'running' } } {
    this.recentPaths = [path, ...this.recentPaths.filter((entry) => entry !== path)];
    this.status = machineStatus('on', 'running', path);
    const cartridge = this.status.cartridge as NonNullable<RuntimeStatusResult['cartridge']>;
    this.emit('cartridges.loaded', { cartridge });
    return { cartridge, state: { power: 'on', mode: 'running' } };
  }

  private update(patch: Partial<Pick<RuntimeStatusResult, 'power' | 'mode'>>): void {
    if (this.status !== undefined) this.status = { ...this.status, ...patch };
  }

  private state(): { state: { power: 'off' | 'on'; mode: 'paused' | 'running' } } {
    return { state: { power: this.status?.power ?? 'off', mode: this.status?.mode ?? 'paused' } };
  }

  /** 记录一次请求；登记了失败原因的请求以它拒绝（只失败一次）。 */
  private request<T>(name: string, params: unknown, action: () => T | Promise<T>): Promise<T> {
    this.calls.push(`${name} ${JSON.stringify(params)}`);
    if (this.failures.has(name)) {
      const reason = this.failures.get(name);
      this.failures.delete(name);
      return Promise.reject(reason);
    }
    try {
      return Promise.resolve(action());
    } catch (error) {
      return Promise.reject(error);
    }
  }
}

/** 等已发出的请求与它们的后续处理完成（微任务队列清空）。 */
export async function settle(): Promise<void> {
  for (let index = 0; index < 8; index += 1) await Promise.resolve();
}
