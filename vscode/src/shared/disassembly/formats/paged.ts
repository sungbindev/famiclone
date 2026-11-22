// ------------------------------------------------------------------------
// 名称：paged.ts
// 说明：分页 ROM 的解析器：文件按固定大小的页排列，一部分页固定映在某个地址（fixed），其余的页换进
//       一个窗口（window）。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-21
// 备注：用来读学习机的 BIOS 一类带页号的 ROM；页号从文件头之后（或文件开头）数起；选项：pageSize
//       （页大小，默认 4096）、headerSize（文件头字节数，默认遇到 NES 文件头取 16，否则 0）、fixed
//       （固定映射：firstPage、pageCount、address，没有 bank 号）、window（可换页的窗口：address 默
//       认 $8000，pageCount 是窗口的页数，默认 4）；窗口里的页按“页号对窗口页数取余”决定放在窗口
//       的第几页，连续的页合成一个段，bank 号是这个段第一页的页号。这只是初始布局：实际每段代码在窗
//       口里的位置要用模拟器的地址解析核实，再用 layout 修改。固定映射里覆盖 $FFFA 到 $FFFF 的那
//       段，末尾六个字节就是三个中断向量。
// ------------------------------------------------------------------------
import { FcdevError } from '../../errors/fcdev-error';
import { createLayout } from '../model/index';
import type { ParsedImage } from './index';

/** 默认的页大小 */
const DEFAULT_PAGE_SIZE = 0x1000;

/** 默认的窗口起点 */
const DEFAULT_WINDOW_ADDRESS = 0x8000;

/** 默认的窗口页数 */
const DEFAULT_WINDOW_PAGES = 4;

/** 文件头魔数后面要跳过的字节数 */
const NES_HEADER_SIZE = 16;

/** 中断向量的起点 */
const VECTOR_ADDRESS = 0xfffa;

/**
 * 检查选项里的整数。
 * @param value 值
 * @param name 选项名，用在错误里
 * @param minimum 最小值
 * @returns 值
 */
function integer(value: unknown, name: string, minimum: number): number {
  if (typeof value !== 'number' || !Number.isInteger(value) || value < minimum) throw new FcdevError(`分页格式的 ${name} 必须是不小于 ${minimum} 的整数`, { code: 'DISASM_INPUT_INVALID' });
  return value;
}

/**
 * 解析分页 ROM。
 * @param buffer 文件内容
 * @param [options] 页大小、文件头、固定映射、窗口
 * @returns 解析结果
 * @throws FcdevError 选项不合法，或固定映射超出文件
 */
export function parsePaged(buffer: Buffer, options: {pageSize?: number, headerSize?: number, fixed?: Array<{firstPage: number, pageCount: number, address: number}>, window?: {address?: number, pageCount?: number}} = {}): ParsedImage {
  const pageSize = integer(options.pageSize ?? DEFAULT_PAGE_SIZE, 'pageSize', 256);
  const headerSize = options.headerSize ?? (buffer.subarray(0, 4).toString('latin1') === 'NES\x1a' ? NES_HEADER_SIZE : 0);
  integer(headerSize, 'headerSize', 0);
  const windowAddress = integer(options.window?.address ?? DEFAULT_WINDOW_ADDRESS, 'window.address', 0);
  const windowPages = integer(options.window?.pageCount ?? DEFAULT_WINDOW_PAGES, 'window.pageCount', 1);
  const fixed = options.fixed ?? [];
  if (!Array.isArray(fixed)) throw new FcdevError('分页格式的 fixed 必须是数组', { code: 'DISASM_INPUT_INVALID' });
  const dataSize = buffer.length - headerSize;
  if (dataSize <= 0) throw new FcdevError('文件里没有页的内容', { code: 'DISASM_INPUT_INVALID' });
  const totalPages = Math.ceil(dataSize / pageSize);

  const fixedPages = new Set<number>();
  const segments: Array<{fileOffset: number, length: number, address: number, bank: number|null}> = [];
  const makeSegment = (firstPage: number, pageCount: number, address: number, bank: number | null) => {
    const fileOffset = headerSize + firstPage * pageSize;
    return { fileOffset, length: Math.min(pageCount * pageSize, buffer.length - fileOffset), address, bank };
  };
  for (const [index, part] of fixed.entries()) {
    const firstPage = integer(part?.firstPage, `fixed[${index}].firstPage`, 0);
    const pageCount = integer(part?.pageCount, `fixed[${index}].pageCount`, 1);
    const address = integer(part?.address, `fixed[${index}].address`, 0);
    if (firstPage + pageCount > totalPages) throw new FcdevError(`fixed[${index}] 超出文件（共 ${totalPages} 页）`, { code: 'DISASM_INPUT_INVALID' });
    for (let page = firstPage; page < firstPage + pageCount; page += 1) fixedPages.add(page);
    segments.push(makeSegment(firstPage, pageCount, address, null));
  }

  for (let page = 0; page < totalPages;) {
    if (fixedPages.has(page)) {
      page += 1;
      continue;
    }
    let last = page;
    while (last + 1 < totalPages && !fixedPages.has(last + 1) && Math.floor((last + 1) / windowPages) === Math.floor(page / windowPages)) last += 1;
    segments.push(makeSegment(page, last - page + 1, windowAddress + (page % windowPages) * pageSize, page));
    page = last + 1;
  }

  segments.sort((a, b) => a.fileOffset - b.fileOffset);
  const layout = createLayout(segments.map((segment, id) => ({ id, ...segment })), { imageSize: buffer.length });
  const vectorSegment = layout.segments.find((segment) => segment.bank === null && segment.address <= VECTOR_ADDRESS && segment.address + segment.length === 0x10000);
  const vectors = vectorSegment === undefined ? null : (() => {
    const end = vectorSegment.fileOffset + vectorSegment.length;
    return { nmi: buffer.readUInt16LE(end - 6), reset: buffer.readUInt16LE(end - 4), irq: buffer.readUInt16LE(end - 2) };
  })();
  return {
    format: 'paged',
    layout,
    vectors,
    vectorSegmentId: vectorSegment?.id,
    unmapped: headerSize > 0 ? [{ offset: 0, length: headerSize, description: '文件头' }] : [],
    description: `分页 ROM，${totalPages} 页（每页 ${pageSize} 字节），${layout.segments.length} 个段`,
  };
}
