/*
 * ------------------------------------------------------------------------
 * 名称：media-status.ts
 * 说明：设备面板用到的纯规则：软驱的盘片、状态文字，磁带机的磁带、状态与位置文字，打印机的状态文字，以及各按钮是否可用。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-15
 * 备注：规则见 docs/05-界面.md 第 4.3 节：没有盘片时取出、保存镜像与写保护置灰；软驱的状态分无盘片、空闲、读写中（带磁道）；打印机的状态分空闲、打印中、打印完成；
 *       磁带机的状态分无磁带、已停止、播放中、录音中，没有磁带时取出与播放置灰，播放或录音时载入、取出、播放、录音置灰、只有停止可用。
 * ------------------------------------------------------------------------
 */

import type { FloppyStatus, PrinterStatus, TapeStatus } from '../../contracts/media/generated/media';

/** 用户可见的文字。 */
const TEXT = {
  empty: '空',
  noDisk: '无盘片',
  idle: '空闲',
  busy: (cylinder: number) => `读写中（磁道 ${cylinder}）`,
  modified: '（有未保存的修改）',
  printer: { idle: '空闲', printing: '打印中', done: '打印完成' },
  tape: { noTape: '无磁带', stopped: '已停止', playing: '播放中', recording: '录音中', noPosition: '—' }
} as const;

/** 软驱各按钮是否可用。 */
export interface FloppyAvailability {
  /** 载入镜像：任何时候都可以，与电源无关。 */
  readonly insert: boolean;
  /** 取出。 */
  readonly eject: boolean;
  /** 保存镜像。 */
  readonly save: boolean;
  /** 写保护开关。 */
  readonly writeProtect: boolean;
}

/** 盘片的显示文字：镜像文件名，有未保存的修改时在后面注明；没有盘片时为“空”。 */
export function diskText(floppy: FloppyStatus): string {
  if (!floppy.inserted) return TEXT.empty;
  return `${floppy.fileName ?? ''}${floppy.modified ? TEXT.modified : ''}`;
}

/** 软驱的状态文字：无盘片、空闲、读写中（带磁道）。 */
export function floppyStateText(floppy: FloppyStatus): string {
  if (!floppy.inserted) return TEXT.noDisk;
  return floppy.motor ? TEXT.busy(floppy.cylinder) : TEXT.idle;
}

/** 打印机的状态文字；还没读到时按空闲显示。 */
export function printerStateText(printer: PrinterStatus | undefined): string {
  return TEXT.printer[printer?.state ?? 'idle'];
}

/** 磁带机各按钮是否可用。 */
export interface TapeAvailability {
  /** 载入磁带。 */
  readonly load: boolean;
  /** 取出。 */
  readonly eject: boolean;
  /** 播放。 */
  readonly play: boolean;
  /** 录音。 */
  readonly record: boolean;
  /** 停止。 */
  readonly stop: boolean;
}

/** 软驱各按钮是否可用。 */
export function floppyAvailability(floppy: FloppyStatus): FloppyAvailability {
  return { insert: true, eject: floppy.inserted, save: floppy.inserted, writeProtect: floppy.inserted };
}

/** 磁带的显示文字：声音文件名；没有磁带时为“空”。 */
export function tapeNameText(tape: TapeStatus): string {
  return tape.loaded ? (tape.fileName ?? '') : TEXT.empty;
}

/** 磁带机的状态文字：无磁带、已停止、播放中、录音中。 */
export function tapeStateText(tape: TapeStatus): string {
  if (!tape.loaded) return TEXT.tape.noTape;
  return TEXT.tape[tape.mode];
}

/** 把秒数写成“分:秒”，超过 1 小时写成“时:分:秒”。 */
function clockText(seconds: number): string {
  const total = Math.max(0, Math.floor(seconds));
  const hours = Math.floor(total / 3600);
  const minutes = Math.floor((total % 3600) / 60);
  const rest = String(total % 60).padStart(2, '0');
  return hours > 0 ? `${hours}:${String(minutes).padStart(2, '0')}:${rest}` : `${minutes}:${rest}`;
}

/** 磁带的位置文字：“当前 / 总时长”，总时长不知道时只写当前位置；没有磁带时为破折号。 */
export function tapePositionText(tape: TapeStatus): string {
  if (!tape.loaded) return TEXT.tape.noPosition;
  return tape.duration > 0 ? `${clockText(tape.position)} / ${clockText(tape.duration)}` : clockText(tape.position);
}

/** 磁带机各按钮是否可用：只有停止时可以载入、取出、播放与录音，播放或录音时只有“停止”可用。 */
export function tapeAvailability(tape: TapeStatus): TapeAvailability {
  const stopped = tape.mode === 'stopped';
  return { load: stopped, eject: stopped && tape.loaded, play: stopped && tape.loaded, record: stopped, stop: !stopped };
}
