// ------------------------------------------------------------------------
// 名称：sbdos.ts
// 说明：SB-DOS 程序（.EXE）的解析器：文件头 `4C lo hi AA 55` 加 3 个字节，从 $8000 起装入；超过 16K
//       的程序按 4K 一页换页。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-21
// 备注：文件头的 JMP 就是程序的入口，入口地址固定是 $8000（文件的第一个字节）；不超过 16K 的程序是
//       一个段；多页程序的页布局取自 FC Dev 开发指南：文件页 0、1 是常驻部分（$8000，8K），页 2 是
//       库页 1（窗口 2，$A000），页 3 是栈页（窗口 3，$B000，不放程序内容），页 4 起是库页 2 起（都
//       换进窗口 2）；库页号 n 对应的文件页：1 是页 2，n 不小于 2 时是页 n+2，bank 号就是库页号。
// ------------------------------------------------------------------------
import { FcdevError } from '../../errors/fcdev-error';
import { createLayout } from '../model/index';
import type { ParsedImage } from './index';

/** 程序装入的地址 */
const LOAD_ADDRESS = 0x8000;

/** 单窗口程序的最大字节数：$8000 到 $BFFF */
const WINDOW_SIZE = 0x4000;

/** 一页的字节数 */
const PAGE_SIZE = 0x1000;

/** 文件头长度：JMP、魔数与 3 个由 DOS 改写或保留的字节 */
const HEADER_SIZE = 8;

/** 文件头里的 JMP 操作码 */
const JMP_OPCODE = 0x4c;

/** 文件头里的魔数 */
const MAGIC = [0xaa, 0x55];

/** 常驻部分的字节数：文件页 0、1 */
const RESIDENT_SIZE = 2 * PAGE_SIZE;

/** 库页 1 所在的文件页 */
const FIRST_LIBRARY_PAGE = 2;

/** 栈页所在的文件页 */
const STACK_PAGE = 3;

/** 库页 2 起所在的第一个文件页 */
const SECOND_LIBRARY_PAGE = 4;

/**
 * 解析 SB-DOS 程序。
 * @param buffer 文件内容
 * @returns 解析结果：入口是 $8000
 * @throws FcdevError 文件头不对
 */
export function parseSbdos(buffer: Buffer): ParsedImage {
  if (buffer.length < HEADER_SIZE || buffer[0] !== JMP_OPCODE || buffer[3] !== MAGIC[0] || buffer[4] !== MAGIC[1]) {
    throw new FcdevError('不是 SB-DOS 程序（文件头应为 4C ?? ?? AA 55）', { code: 'DISASM_INPUT_INVALID', hint: '学习机的 BIOS 用 paged 格式；其他文件用 flat 格式并给出加载地址' });
  }
  const segments: Array<{id: number, fileOffset: number, length: number, address: number, bank: number|null}> = [];
  let description: string;
  if (buffer.length <= WINDOW_SIZE) {
    segments.push({ id: 0, fileOffset: 0, length: buffer.length, address: LOAD_ADDRESS, bank: null });
    description = `SB-DOS 程序，${buffer.length} 字节`;
  } else {
    const piece = (id: number, offset: number, address: number, bank: number | null, limit: number) => ({ id, fileOffset: offset, length: Math.min(limit, buffer.length - offset), address, bank });
    segments.push(piece(0, 0, LOAD_ADDRESS, null, RESIDENT_SIZE));
    segments.push(piece(1, FIRST_LIBRARY_PAGE * PAGE_SIZE, LOAD_ADDRESS + 2 * PAGE_SIZE, 1, PAGE_SIZE));
    segments.push(piece(2, STACK_PAGE * PAGE_SIZE, LOAD_ADDRESS + 3 * PAGE_SIZE, null, PAGE_SIZE));
    for (let page = SECOND_LIBRARY_PAGE; page * PAGE_SIZE < buffer.length; page += 1) {
      segments.push(piece(segments.length, page * PAGE_SIZE, LOAD_ADDRESS + 2 * PAGE_SIZE, page - FIRST_LIBRARY_PAGE, PAGE_SIZE));
    }
    description = `SB-DOS 程序，${buffer.length} 字节，多页（${segments.length - 2} 个库页）`;
  }
  return {
    format: 'sbdos',
    layout: createLayout(segments, { imageSize: buffer.length }),
    vectors: null,
    entries: [{ address: LOAD_ADDRESS, description: '程序入口（文件头的 JMP）' }],
    unmapped: [],
    description,
  };
}
