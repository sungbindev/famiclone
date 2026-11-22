// ------------------------------------------------------------------------
// 名称：index.ts
// 说明：formats 目录的出口：映像格式解析器的登记表与统一入口。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-21
// 备注：新增格式只需新增一个解析器并登记在 PARSERS 里；解析器把文件解析成布局，核心逻辑不认识具体格
//       式。
// ------------------------------------------------------------------------
import { FcdevError } from '../../errors/fcdev-error';
import { parseFlat } from './flat';
import { parseInes, parseInesHeader } from './ines';
import { parsePaged } from './paged';
import { parseSbdos } from './sbdos';
import type { Layout } from '../model/layout';

export interface ParsedImage {
  /** 格式名 */
  format: string;
  /** 布局 */
  layout: Layout;
  /** 文件里带的中断向量（CPU 地址）；没有时为 null */
  vectors: {nmi: number, reset: number, irq: number}|null;
  /** 向量是从哪个段读到的；向量指向换页窗口时用它确定 bank */
  vectorSegmentId?: number;
  /** 文件格式自己规定的入口（如程序文件头的 JMP） */
  entries?: Array<{address: number, description: string}>;
  /** 文件里不属于 CPU 地址空间或没有划入段的字节范围 */
  unmapped: Array<{offset: number, length: number, description: string}>;
  /** 来源说明，写进报告 */
  description: string;
}

/** 格式名到解析器 */
const PARSERS = { flat: parseFlat, ines: parseInes, sbdos: parseSbdos, paged: parsePaged };

/** 支持的格式名 */
export const FORMAT_NAMES = Object.freeze(Object.keys(PARSERS));

/** 映像文件开头的魔数到格式名；都不符合的当平铺二进制 */
const SIGNATURES = [{ format: 'ines', magic: Buffer.from([0x4e, 0x45, 0x53, 0x1a]) }];

/**
 * 按文件开头判断格式。
 * @param buffer 文件内容
 * @returns 格式名；认不出时为 flat
 */
export function detectFormat(buffer: Buffer): string {
  const found = SIGNATURES.find(({ magic }) => buffer.subarray(0, magic.length).equals(magic));
  return found === undefined ? 'flat' : found.format;
}

/**
 * 按格式名解析映像文件。
 * @param format 格式名，见 FORMAT_NAMES
 * @param buffer 文件内容
 * @param [options] 传给解析器的选项（flat 的 address、segments）
 * @returns 解析结果
 * @throws FcdevError 格式名未知，或解析器拒绝这个文件
 */
export function parseImage(format: string, buffer: Buffer, options: object = {}): ParsedImage {
  const parser = Object.hasOwn(PARSERS, format) ? (PARSERS[format as keyof typeof PARSERS] as (data: Buffer, parserOptions: object) => ParsedImage) : undefined;
  if (parser === undefined) {
    throw new FcdevError(`不支持的映像格式：${format}`, { code: 'DISASM_FORMAT_UNKNOWN', hint: `可用的格式：${FORMAT_NAMES.join('、')}` });
  }
  return parser(buffer, options);
}

export { CDL_CODE, CDL_DATA, CDL_JUMP_TARGET, CDL_SUB_ENTRY, parseCdl } from './cdl';
export { parseCallLog } from './call-log';
export { parseFlat, parseInes, parseInesHeader, parsePaged, parseSbdos };
export { SUPPORTED_MAPPERS } from './ines';
export { parseTrace } from './trace-log';
