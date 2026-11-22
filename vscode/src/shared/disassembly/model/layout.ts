// ------------------------------------------------------------------------
// 名称：layout.ts
// 说明：映像布局：把映像文件的字节划分成若干段，每段对应一段 CPU 地址；可换页的窗口里，同一段地址可
//       以有多个段，用 bank 号区分。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-21
// 备注：规则：段在文件里不重叠（每个字节最多属于一个段）；两段的 CPU 地址范围重叠时，必须都有 bank
//       号且号不同；段不能超出 $FFFF。布局只含数字与布尔值，可以直接写进工程文件。
// ------------------------------------------------------------------------
import { FcdevError } from '../../errors/fcdev-error';

/** CPU 地址空间的大小 */
const ADDRESS_SPACE = 0x10000;

export interface Segment {
  /** 段号，在布局内唯一 */
  id: number;
  /** 段的第一个字节在映像文件里的偏移 */
  fileOffset: number;
  /** 段的字节数 */
  length: number;
  /** 段的第一个字节的 CPU 地址 */
  address: number;
  /** bank 号；不换页的段为 null */
  bank: number|null;
  /** 运行时是否可写（RAM 为 true，ROM 为 false） */
  writable: boolean;
}

export interface Layout {
  /** 映像文件的字节数 */
  imageSize: number;
  /** 全部段，按给定顺序 */
  segments: ReadonlyArray<Readonly<Segment>>;
}

/**
 * 抛出布局错误。
 * @param message 说明
 * @returns
 */
function invalid(message: string): never {
  throw new FcdevError(`布局无效：${message}`, { code: 'DISASM_LAYOUT_INVALID' });
}

/**
 * 检查并规范化一个段。
 * @param input 段
 * @param imageSize 映像大小
 * @returns 规范化的段
 */
function normalizeSegment(input: Partial<Segment>, imageSize: number): Readonly<Segment> {
  const bank = input.bank ?? null;
  const writable = input.writable ?? false;
  const numberField = (name: 'id' | 'fileOffset' | 'length' | 'address'): number => {
    const value = input[name];
    if (typeof value !== 'number' || !Number.isInteger(value) || value < 0) return invalid(`段 ${input.id} 的 ${name} 必须是非负整数`);
    return value;
  };
  const id = numberField('id');
  const fileOffset = numberField('fileOffset');
  const length = numberField('length');
  const address = numberField('address');
  if (length === 0) invalid(`段 ${id} 的长度不能是 0`);
  if (fileOffset + length > imageSize) invalid(`段 ${id} 超出映像文件（偏移 ${fileOffset}，长度 ${length}，文件 ${imageSize} 字节）`);
  if (address + length > ADDRESS_SPACE) invalid(`段 ${id} 超出 $FFFF（地址 $${address.toString(16).toUpperCase()}，长度 ${length}）`);
  if (bank !== null && (!Number.isInteger(bank) || bank < 0)) invalid(`段 ${id} 的 bank 必须是非负整数或 null`);
  if (typeof writable !== 'boolean') invalid(`段 ${id} 的 writable 必须是布尔值`);
  return Object.freeze({ id, fileOffset, length, address, bank, writable });
}

/**
 * 创建布局，检查段之间没有冲突。
 * @param segments 段；bank 省略为 null，writable 省略为 false
 * @param options 映像文件的字节数
 * @returns 布局
 * @throws FcdevError 没有段、字段不合法、段号重复、文件范围重叠、同一 bank 的地址重叠或超出范围
 */
export function createLayout(segments: Array<Partial<Segment>>, { imageSize }: {imageSize: number}): Readonly<Layout> {
  if (!Number.isInteger(imageSize) || imageSize <= 0) invalid('映像大小必须是正整数');
  if (!Array.isArray(segments) || segments.length === 0) invalid('至少要有一个段');
  const normalized = segments.map((segment) => normalizeSegment(segment, imageSize));

  for (let first = 0; first < normalized.length; first += 1) {
    for (let second = first + 1; second < normalized.length; second += 1) {
      const a = normalized[first]!;
      const b = normalized[second]!;
      if (a.id === b.id) invalid(`段号 ${a.id} 重复`);
      if (a.fileOffset < b.fileOffset + b.length && b.fileOffset < a.fileOffset + a.length) invalid(`段 ${a.id} 与段 ${b.id} 在文件里重叠`);
      const addressOverlap = a.address < b.address + b.length && b.address < a.address + a.length;
      if (addressOverlap && (a.bank === null || b.bank === null || a.bank === b.bank)) {
        invalid(`段 ${a.id} 与段 ${b.id} 的 CPU 地址重叠，且不是不同的 bank`);
      }
    }
  }
  return Object.freeze({ imageSize, segments: Object.freeze(normalized) });
}

/**
 * 按段号取段。
 * @param layout 布局
 * @param id 段号
 * @returns 段；不存在时为 undefined
 */
export function getSegment(layout: Layout, id: number): Readonly<Segment>|undefined {
  return layout.segments.find((segment) => segment.id === id);
}

/**
 * 按段号取段，段必须存在。
 * @param layout 布局
 * @param id 段号
 * @returns 段
 * @throws FcdevError 段不存在
 */
export function expectSegment(layout: Layout, id: number): Readonly<Segment> {
  const segment = getSegment(layout, id);
  if (segment === undefined) throw new FcdevError(`布局里没有段 ${id}`, { code: 'DISASM_LAYOUT_INVALID' });
  return segment;
}

/**
 * 找出覆盖某个 CPU 地址的全部段：不换页的地址只有一段，换页窗口里每个 bank 一段。
 * @param layout 布局
 * @param address CPU 地址
 * @returns 段，按布局里的顺序
 */
export function findSegmentsAt(layout: Layout, address: number): Array<Readonly<Segment>> {
  return layout.segments.filter((segment) => address >= segment.address && address < segment.address + segment.length);
}

/**
 * 把段内的 CPU 地址换算成映像文件的偏移。
 * @param segment 段
 * @param address CPU 地址
 * @returns 文件偏移
 * @throws RangeError 地址不在这个段里
 */
export function addressToOffset(segment: Segment, address: number): number {
  if (address < segment.address || address >= segment.address + segment.length) {
    throw new RangeError(`地址 $${address.toString(16).toUpperCase()} 不在段 ${segment.id} 里`);
  }
  return segment.fileOffset + (address - segment.address);
}

/**
 * 把映像文件的偏移换算成所在的段与 CPU 地址。
 * @param layout 布局
 * @param fileOffset 文件偏移
 * @returns 偏移不属于任何段时为 null
 */
export function locateOffset(layout: Layout, fileOffset: number): {segment: Readonly<Segment>, address: number}|null {
  const segment = layout.segments.find((item) => fileOffset >= item.fileOffset && fileOffset < item.fileOffset + item.length);
  return segment === undefined ? null : { segment, address: segment.address + (fileOffset - segment.fileOffset) };
}

/**
 * 列出映像文件里不属于任何段的字节范围。
 * @param layout 布局
 * @returns 范围，按偏移从小到大
 */
export function findUncovered(layout: Layout): Array<{offset: number, length: number}> {
  const sorted = [...layout.segments].toSorted((a, b) => a.fileOffset - b.fileOffset);
  const gaps = [];
  let cursor = 0;
  for (const segment of sorted) {
    if (segment.fileOffset > cursor) gaps.push({ offset: cursor, length: segment.fileOffset - cursor });
    cursor = segment.fileOffset + segment.length;
  }
  if (cursor < layout.imageSize) gaps.push({ offset: cursor, length: layout.imageSize - cursor });
  return gaps;
}
