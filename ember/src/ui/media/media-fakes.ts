/*
 * ------------------------------------------------------------------------
 * 名称：media-fakes.ts
 * 说明：设备面板测试用的假核心接口：按预加载脚本的接口形状实现 media 的软驱与打印机请求与主进程的载入镜像请求，保存软驱与打印机的状态，记录每次调用，可以发出核心事件与核心运行状态变化。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-15
 * 备注：只供测试使用（不含真实的核心逻辑，只做设备面板需要的最简单的状态转换）；不引用测试框架，所以放在 ui 层也满足依赖检查。
 * ------------------------------------------------------------------------
 */

import type { CoreStatus } from '../../contracts/core-gateway/core-messages';
import type { FloppyStatus, PrinterStatus, TapeStatus } from '../../contracts/media/generated/media';
import type { MediaBackend } from './media-controller';

/** 构造一个磁带机的状态；默认是没有接在通电的主机上（connected 为 false）。 */
export function tapeStatus(patch: Partial<TapeStatus> = {}): TapeStatus {
  return { connected: false, loaded: false, mode: 'stopped', position: 0, duration: 0, stereo: false, ...patch };
}

/** 构造一个软驱的状态；fileName 为 undefined 表示没有盘片。 */
export function floppyStatus(drive: number, fileName?: string, patch: Partial<FloppyStatus> = {}): FloppyStatus {
  const base: FloppyStatus = {
    drive,
    inserted: fileName !== undefined,
    writeProtected: false,
    modified: false,
    size: fileName === undefined ? 0 : 1474560,
    cylinder: 0,
    motor: false,
    faults: { ejected: false, noIndex: false, seekFail: false, unresponsive: false, badSectors: [] }
  };
  return { ...base, ...(fileName === undefined ? {} : { fileName }), ...patch };
}

/** 假的核心接口。 */
export class FakeMediaBackend {
  /** 各软驱的状态；为空表示没有开机或主机没有软驱。 */
  floppies: FloppyStatus[] = [floppyStatus(0), floppyStatus(1)];
  printer: PrinterStatus = { state: 'idle', hasContent: false, pages: 0 };
  /** 磁带机的状态。 */
  tape: TapeStatus = tapeStatus();
  /** 下一次载入磁带时用户选的文件名；undefined 表示用户取消。 */
  pickedTape: string | undefined;
  /** 下一次录音时用户选的文件名；undefined 表示用户取消。 */
  pickedRecording: string | undefined;
  /** 下一次载入镜像时用户选的文件名；undefined 表示用户取消。 */
  pickedFile: string | undefined;
  /** 按请求名登记的下一次失败的原因。 */
  readonly failures = new Map<string, unknown>();
  /** 每次请求的记录：请求名加参数。 */
  readonly calls: string[] = [];
  /** 当前核心运行状态。 */
  coreStatus: CoreStatus = { state: 'ready', coreVersion: '0.1.0' };

  private readonly eventListeners = new Set<(name: string, data: unknown) => void>();
  private readonly statusListeners = new Set<(status: CoreStatus) => void>();

  /** 预加载脚本接口里设备面板用到的部分。 */
  readonly backend: MediaBackend = {
    core: {
      media: {
        floppyStatus: () => this.request('media.floppyStatus', {}, () => ({ drives: this.floppies })),
        printerStatus: () => this.request('media.printerStatus', {}, () => ({ status: this.printer })),
        tapeStatus: () => this.request('media.tapeStatus', {}, () => ({ status: this.tape })),
        tapeEject: () =>
          this.request('media.tapeEject', {}, () => {
            this.tape = { ...this.tape, loaded: false, mode: 'stopped', position: 0, duration: 0, stereo: false };
            delete (this.tape as { fileName?: string }).fileName;
            this.emit('media.changed', { device: 'tape', drive: 0 });
            return { status: this.tape };
          }),
        tapePlay: () =>
          this.request('media.tapePlay', {}, () => {
            if (!this.tape.loaded) throw { code: 'media.noTape', message: '磁带机里没有磁带' };
            if (!this.tape.connected) throw { code: 'media.tapeNotConnected', message: '主机没有通电，磁带机不能播放' };
            this.tape = { ...this.tape, mode: 'playing' };
            this.emit('media.changed', { device: 'tape', drive: 0 });
            return { status: this.tape };
          }),
        tapeStop: () =>
          this.request('media.tapeStop', {}, () => {
            this.tape = { ...this.tape, mode: 'stopped' };
            this.emit('media.changed', { device: 'tape', drive: 0 });
            return { status: this.tape };
          }),
        floppyEject: (params: { drive: number }) =>
          this.request('media.floppyEject', params, () => {
            const status = this.change(params.drive, () => floppyStatus(params.drive));
            this.emit('media.changed', { device: 'floppy', drive: params.drive });
            return { status };
          }),
        floppySave: (params: { drive: number }) =>
          this.request('media.floppySave', params, () => {
            const status = this.change(params.drive, (current) => ({ ...current, modified: false }));
            this.emit('media.changed', { device: 'floppy', drive: params.drive });
            return { status };
          }),
        floppyWriteProtect: (params: { drive: number; writeProtected: boolean }) =>
          this.request('media.floppyWriteProtect', params, () => {
            const status = this.change(params.drive, (current) => ({ ...current, writeProtected: params.writeProtected }));
            this.emit('media.changed', { device: 'floppy', drive: params.drive });
            return { status };
          })
      }
    },
    main: {
      media: {
        openFloppy: (params: { drive: number }) =>
          this.request('media.openFloppy', params, () => {
            if (this.pickedFile === undefined) return { canceled: true };
            const file = this.pickedFile;
            const status = this.change(params.drive, () => floppyStatus(params.drive, file));
            this.emit('media.changed', { device: 'floppy', drive: params.drive });
            return { canceled: false, status };
          }),
        openTape: () =>
          this.request('media.openTape', {}, () => {
            if (this.pickedTape === undefined) return { canceled: true };
            this.tape = { ...this.tape, loaded: true, fileName: this.pickedTape, mode: 'stopped', position: 0, duration: 60, stereo: true };
            this.emit('media.changed', { device: 'tape', drive: 0 });
            return { canceled: false, status: this.tape };
          }),
        openTapeRecording: () =>
          this.request('media.openTapeRecording', {}, () => {
            if (this.pickedRecording === undefined) return { canceled: true };
            this.tape = { ...this.tape, loaded: true, fileName: this.pickedRecording, mode: 'recording', position: 0, duration: 0, stereo: false };
            this.emit('media.changed', { device: 'tape', drive: 0 });
            return { canceled: false, status: this.tape };
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
  } as unknown as MediaBackend;

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

  /** 某个请求被调用了几次。 */
  count(name: string): number {
    return this.calls.filter((call) => call.startsWith(`${name} `)).length;
  }

  /** 改写一个软驱的状态并返回新状态。 */
  private change(drive: number, make: (current: FloppyStatus) => FloppyStatus): FloppyStatus {
    const index = this.floppies.findIndex((item) => item.drive === drive);
    if (index < 0) throw { code: 'media.noDrive', message: '没有这个驱动器' };
    const next = make(this.floppies[index] as FloppyStatus);
    this.floppies = this.floppies.map((item, position) => (position === index ? next : item));
    return next;
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
