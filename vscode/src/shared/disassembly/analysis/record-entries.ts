// ------------------------------------------------------------------------
// 名称：record-entries.ts
// 说明：把运行记录换成入口与数据区域：一组 CPU 地址（指令轨迹、调用记录）、按字节标志的代码段起点与
//       只被当作数据读取的字节段（代码数据日志）。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-21
// 备注：记录里常有不在映像里的地址（RAM、别的 bank），不生成入口，原样返回在 skipped 里，由调用者汇
//       总提示；字节标志与布局里各段按文件偏移顺序首尾相接的字节一一对应，每个段的第一个字节也算一
//       段代码的起点。
// ------------------------------------------------------------------------
import { FcdevError } from '../../errors/fcdev-error';
import { createEntry } from '../model/index';
import { entryAtAddress } from './entry-points';
import type { Entry } from '../model/entries';
import type { AddressSpace } from './address-space';
import type { Layout } from '../model/layout';

/** 字节标志里表示“被当作代码执行过”的位 */
const CODE_BIT = 0x01;

/** 字节标志里表示“被当作数据读取过”的位 */
const DATA_BIT = 0x02;

/**
 * 由一组 CPU 地址创建入口。
 * @param space 地址空间
 * @param addresses CPU 地址
 * @param options 来源与说明
 * @returns 入口与没能生成入口的地址
 */
export function entriesFromAddresses(space: AddressSpace, addresses: Iterable<number>, { source, description }: {source: string, description?: string}): {entries: Array<Readonly<Entry>>, skipped: Array<{address: number, reason: string}>} {
  const entries: Array<Readonly<Entry>> = [];
  const skipped: Array<{address: number, reason: string}> = [];
  for (const address of addresses) {
    try {
      entries.push(entryAtAddress(space, address, { source, description }));
    } catch (error) {
      if (!(error instanceof FcdevError)) throw error;
      skipped.push({ address, reason: error.message });
    }
  }
  return { entries, skipped };
}

/**
 * 布局里全部段的字节数。
 * @param layout 布局
 * @returns 字节数
 */
export function mappedSize(layout: Layout): number {
  return layout.segments.reduce((sum, segment) => sum + segment.length, 0);
}

/**
 * 由按字节的标志创建入口：每段连续的代码字节的起点都是被执行过的指令的开头。
 * @param layout 布局
 * @param flags 标志，长度不少于布局里全部段的字节数；位 0 为代码
 * @param options 来源与说明；startBits 是除“代码段第一个字节”外也算起点的标志位（如跳转目标、子程
 *   序入口），默认没有
 * @returns 入口
 * @throws FcdevError 标志比布局里的字节少
 */
export function entriesFromCodeFlags(layout: Layout, flags: Uint8Array, { source, description, startBits = 0 }: {source: string, description?: string, startBits?: number}): Array<Readonly<Entry>> {
  if (flags.length < mappedSize(layout)) throw new FcdevError(`标志只有 ${flags.length} 个，少于布局里的 ${mappedSize(layout)} 字节`, { code: 'DISASM_CDL_INVALID' });
  const entries: Array<Readonly<Entry>> = [];
  let index = 0;
  for (const segment of [...layout.segments].toSorted((a, b) => a.fileOffset - b.fileOffset)) {
    for (let offset = 0; offset < segment.length; offset += 1) {
      const flag = flags[index + offset] ?? 0;
      const isCode = (flag & CODE_BIT) !== 0;
      const startsRun = offset === 0 || ((flags[index + offset - 1] ?? 0) & CODE_BIT) === 0 || (flag & startBits) !== 0;
      if (isCode && startsRun) entries.push(createEntry(layout, { segmentId: segment.id, address: segment.address + offset, source, description }));
    }
    index += segment.length;
  }
  return entries;
}

/**
 * 由按字节的标志找出只被当作数据读取过的字节段：每段连续的这类字节是一个数据区域。
 * @param layout 布局
 * @param flags 标志，长度不少于布局里全部段的字节数；位 0 为代码，位 1 为数据
 * @returns 数据段：所在段、起始 CPU 地址、起始文件偏移与字节数；不跨段
 * @throws FcdevError 标志比布局里的字节少
 */
export function dataRunsFromFlags(layout: Layout, flags: Uint8Array): Array<{segmentId: number, address: number, offset: number, length: number}> {
  if (flags.length < mappedSize(layout)) throw new FcdevError(`标志只有 ${flags.length} 个，少于布局里的 ${mappedSize(layout)} 字节`, { code: 'DISASM_CDL_INVALID' });
  const runs: Array<{segmentId: number, address: number, offset: number, length: number}> = [];
  let index = 0;
  for (const segment of [...layout.segments].toSorted((a, b) => a.fileOffset - b.fileOffset)) {
    let runStart = -1;
    for (let offset = 0; offset <= segment.length; offset += 1) {
      const flag = offset < segment.length ? (flags[index + offset] ?? 0) : 0;
      const isData = (flag & DATA_BIT) !== 0 && (flag & CODE_BIT) === 0;
      if (isData && runStart < 0) {
        runStart = offset;
      } else if (!isData && runStart >= 0) {
        runs.push({ segmentId: segment.id, address: segment.address + runStart, offset: segment.fileOffset + runStart, length: offset - runStart });
        runStart = -1;
      }
    }
    index += segment.length;
  }
  return runs;
}
