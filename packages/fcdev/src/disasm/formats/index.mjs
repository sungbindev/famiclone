// ------------------------------------------------------------------------
// 名称：index.mjs
// 说明：formats 目录的出口：映像格式解析器的登记表与统一入口。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-21
// 备注：新增格式只需新增一个解析器并登记在 PARSERS 里；解析器把文件解析成布局，核心逻辑不认识具体格式。
// ------------------------------------------------------------------------
import { FcdevError } from '../../shared/index.mjs';
import { parseFlat } from './flat.mjs';
import { parseInes, parseInesHeader } from './ines.mjs';
import { parsePaged } from './paged.mjs';
import { parseSbdos } from './sbdos.mjs';

/**
 * @typedef {object} ParsedImage
 * @property {string} format 格式名
 * @property {import('../model/layout.mjs').Layout} layout 布局
 * @property {{nmi: number, reset: number, irq: number}|null} vectors 文件里带的中断向量（CPU 地址）；没有时为 null
 * @property {number} [vectorSegmentId] 向量是从哪个段读到的；向量指向换页窗口时用它确定 bank
 * @property {Array<{address: number, description: string}>} [entries] 文件格式自己规定的入口（如程序文件头的 JMP）
 * @property {Array<{offset: number, length: number, description: string}>} unmapped 文件里不属于 CPU 地址空间或没有划入段的字节范围
 * @property {string} description 来源说明，写进报告
 */

/** 格式名到解析器 */
const PARSERS = { flat: parseFlat, ines: parseInes, sbdos: parseSbdos, paged: parsePaged };

/** 支持的格式名 */
export const FORMAT_NAMES = Object.freeze(Object.keys(PARSERS));

/** 映像文件开头的魔数到格式名；都不符合的当平铺二进制 */
const SIGNATURES = [{ format: 'ines', magic: Buffer.from([0x4e, 0x45, 0x53, 0x1a]) }];

/**
 * 按文件开头判断格式。
 * @param {Buffer} buffer 文件内容
 * @returns {string} 格式名；认不出时为 flat
 */
export function detectFormat(buffer) {
  const found = SIGNATURES.find(({ magic }) => buffer.subarray(0, magic.length).equals(magic));
  return found === undefined ? 'flat' : found.format;
}

/**
 * 按格式名解析映像文件。
 * @param {string} format 格式名，见 FORMAT_NAMES
 * @param {Buffer} buffer 文件内容
 * @param {object} [options] 传给解析器的选项（flat 的 address、segments）
 * @returns {ParsedImage} 解析结果
 * @throws {FcdevError} 格式名未知，或解析器拒绝这个文件
 */
export function parseImage(format, buffer, options = {}) {
  const parser = Object.hasOwn(PARSERS, format) ? PARSERS[format] : undefined;
  if (parser === undefined) {
    throw new FcdevError(`不支持的映像格式：${format}`, { code: 'DISASM_FORMAT_UNKNOWN', hint: `可用的格式：${FORMAT_NAMES.join('、')}` });
  }
  return parser(buffer, options);
}

export { CDL_CODE, CDL_DATA, CDL_JUMP_TARGET, CDL_SUB_ENTRY, parseCdl } from './cdl.mjs';
export { parseCallLog } from './call-log.mjs';
export { parseFlat, parseInes, parseInesHeader, parsePaged, parseSbdos };
export { SUPPORTED_MAPPERS } from './ines.mjs';
export { parseTrace } from './trace-log.mjs';
