/*
 * ------------------------------------------------------------------------
 * 名称：media-status.ts
 * 说明：设备边栏用到的纯规则：软驱的盘片、状态文字，打印机的状态文字，以及各按钮是否可用。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-15
 * 备注：规则见 docs/05-界面.md 第 4.3 节：没有盘片时取出、保存镜像与写保护置灰；软驱的状态分无盘片、空闲、读写中（带磁道）；打印机的状态分空闲、打印中、打印完成。
 * ------------------------------------------------------------------------
 */

import type { FloppyStatus, PrinterStatus } from '../../contracts/media/generated/media';

/** 用户可见的文字。 */
const TEXT = {
  empty: '空',
  noDisk: '无盘片',
  idle: '空闲',
  busy: (cylinder: number) => `读写中（磁道 ${cylinder}）`,
  modified: '（有未保存的修改）',
  printer: { idle: '空闲', printing: '打印中', done: '打印完成' }
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

/** 软驱各按钮是否可用。 */
export function floppyAvailability(floppy: FloppyStatus): FloppyAvailability {
  return { insert: true, eject: floppy.inserted, save: floppy.inserted, writeProtect: floppy.inserted };
}
