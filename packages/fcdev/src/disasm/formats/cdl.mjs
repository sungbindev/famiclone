// ------------------------------------------------------------------------
// 名称：cdl.mjs
// 说明：读取 CDL（代码数据日志）：每个字节一个标志，位 0 表示被当作代码执行过，位 1 表示被当作数据读取过。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-21
// 备注：这是 FCEUX 与 Mesen 的通用格式；Mesen 的新格式在开头多 9 个字节（CDLv2 加校验值），会自动跳过；只取与指定大小对应的前若干字节。
// ------------------------------------------------------------------------
import { FcdevError } from '../../shared/index.mjs';

/** CDL 标志：被当作代码执行过 */
export const CDL_CODE = 0x01;

/** CDL 标志：被当作数据读取过 */
export const CDL_DATA = 0x02;

/** Mesen CDLv2 的头：魔数 5 字节加 CRC32 4 字节 */
const MESEN_HEADER = { magic: 'CDLv2', size: 9 };

/**
 * 解析 CDL 文件。
 * @param {Buffer} buffer 文件内容
 * @param {number} size 被记录的程序字节数
 * @returns {Uint8Array} 长度为 size 的标志数组，只保留代码与数据两位
 * @throws {FcdevError} 文件比程序短
 */
export function parseCdl(buffer, size) {
  const start = buffer.subarray(0, MESEN_HEADER.magic.length).toString('latin1') === MESEN_HEADER.magic ? MESEN_HEADER.size : 0;
  if (buffer.length - start < size) {
    throw new FcdevError(`CDL 只有 ${buffer.length - start} 字节，少于程序的 ${size} 字节`, { code: 'DISASM_CDL_INVALID', hint: '确认 CDL 是用同一个映像记录的' });
  }
  const flags = new Uint8Array(size);
  for (let index = 0; index < size; index += 1) flags[index] = buffer[start + index] & (CDL_CODE | CDL_DATA);
  return flags;
}
