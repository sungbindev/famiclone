// ------------------------------------------------------------------------
// 名称：loaders.mjs
// 说明：读取待反汇编的文件：iNES（NROM）取出 PRG 与中断向量，平铺二进制按指定的加载地址处理。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-10
// 备注：iNES 目前只支持映射器 0（NROM，16K 或 32K 的 PRG）；其他映射器要先确定地址映射，不在首批范围。
// ------------------------------------------------------------------------
import { FcdevError } from '../shared/index.mjs';

/** iNES 文件头长度 */
const INES_HEADER_SIZE = 16;

/** iNES 的 trainer 块长度 */
const INES_TRAINER_SIZE = 512;

/**
 * @typedef {object} Image
 * @property {Buffer} bytes 要反汇编的字节
 * @property {number} base 第一个字节的 CPU 地址
 * @property {{nmi: number, reset: number, irq: number}|null} vectors 中断向量；没有时为 null
 * @property {string} description 来源说明，写进报告
 */

/**
 * 读取 iNES 文件的 PRG 部分。
 * @param {Buffer} buffer 文件内容
 * @returns {Image} PRG 映像：16K 映在 $C000，32K 映在 $8000
 * @throws {FcdevError} 不是 iNES 文件、映射器不是 0，或 PRG 大小不是 16K 与 32K
 */
export function loadInes(buffer) {
  if (buffer.length < INES_HEADER_SIZE || buffer.subarray(0, 4).toString('latin1') !== 'NES\x1a') {
    throw new FcdevError('不是 iNES 文件（缺少 NES 文件头）', { code: 'DISASM_INPUT_INVALID' });
  }
  const mapper = (buffer[7] & 0xf0) | (buffer[6] >> 4);
  if (mapper !== 0) {
    throw new FcdevError(`暂不支持映射器 ${mapper}`, { code: 'DISASM_MAPPER_UNSUPPORTED', hint: '目前只支持 NROM（映射器 0）；其他映射器可先导出某个 bank，用 --format binary 并指定加载地址' });
  }
  const prgSize = buffer[4] * 0x4000;
  if (prgSize !== 0x4000 && prgSize !== 0x8000) {
    throw new FcdevError(`NROM 的 PRG 应为 16K 或 32K，实际 ${prgSize} 字节`, { code: 'DISASM_INPUT_INVALID' });
  }
  const start = INES_HEADER_SIZE + (buffer[6] & 0x04 ? INES_TRAINER_SIZE : 0);
  if (buffer.length < start + prgSize) {
    throw new FcdevError('文件比 iNES 文件头声明的短', { code: 'DISASM_INPUT_INVALID' });
  }
  const bytes = buffer.subarray(start, start + prgSize);
  return {
    bytes,
    base: 0x10000 - prgSize,
    vectors: { nmi: bytes.readUInt16LE(prgSize - 6), reset: bytes.readUInt16LE(prgSize - 4), irq: bytes.readUInt16LE(prgSize - 2) },
    description: `iNES NROM，PRG ${prgSize / 1024}K`,
  };
}

/**
 * 把平铺二进制当作映像。
 * @param {Buffer} buffer 文件内容
 * @param {{base: number}} options 第一个字节的 CPU 地址
 * @returns {Image} 映像
 * @throws {FcdevError} 文件为空，或超出 64K 地址空间
 */
export function loadBinary(buffer, { base }) {
  if (buffer.length === 0) throw new FcdevError('输入文件是空的', { code: 'DISASM_INPUT_INVALID' });
  if (base + buffer.length > 0x10000) {
    throw new FcdevError(`从 $${base.toString(16).toUpperCase()} 起放不下 ${buffer.length} 字节（超出 $FFFF）`, { code: 'DISASM_INPUT_INVALID' });
  }
  return { bytes: buffer, base, vectors: null, description: `平铺二进制，加载地址 $${base.toString(16).toUpperCase()}` };
}
