// ------------------------------------------------------------------------
// 名称：sb97-rom.mjs
// 说明：SB97 BIOS 文件的读取与检查：程序区大小、校验和，以及按 16K 页取字节。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-21
// 备注：BIOS 是 ember/bios 里不带文件头的纯程序区；这台机器按 mapper 168 处理，
//       文件用程序区的 CRC32 认定；程序区 1MB，共 64 页、每页 16K，没有 CHR。
// ------------------------------------------------------------------------
import { readFileSync } from 'node:fs';
import { dirname, resolve } from 'node:path';
import { fileURLToPath } from 'node:url';

/** 一页的字节数（16K） */
export const PAGE_SIZE = 0x4000;
/** 程序区的页数 */
export const PAGE_COUNT = 64;
/** 程序区的字节数 */
export const PRG_SIZE = PAGE_SIZE * PAGE_COUNT;
/** 程序区的 CRC32，对应本机型的 BIOS */
export const PRG_CRC32 = 0x40a4c574;
/** 本机型按这个 mapper 号处理 */
export const MAPPER_NUMBER = 168;

/** 仓库里 SB97 BIOS 的默认位置 */
export const DEFAULT_ROM_PATH = resolve(dirname(fileURLToPath(import.meta.url)), '../../../ember/bios/sb97_bios.bin');

const CRC_TABLE = (() => {
  const table = new Uint32Array(256);
  for (let n = 0; n < 256; n += 1) {
    let value = n;
    for (let bit = 0; bit < 8; bit += 1) value = value & 1 ? 0xedb88320 ^ (value >>> 1) : value >>> 1;
    table[n] = value >>> 0;
  }
  return table;
})();

/**
 * 计算 CRC32（与 zip、PNG 相同的多项式）。
 * @param {Uint8Array} bytes 数据
 * @returns {number} 无符号 32 位结果
 */
export function crc32(bytes) {
  let value = 0xffffffff;
  for (const byte of bytes) value = CRC_TABLE[(value ^ byte) & 0xff] ^ (value >>> 8);
  return (value ^ 0xffffffff) >>> 0;
}

/**
 * @typedef {object} Sb97Rom
 * @property {Uint8Array} prg 程序区
 * @property {number} crc 程序区的 CRC32
 */

/**
 * 读取并检查 BIOS 文件：程序区必须是 1MB，CRC32 必须是已知的 BIOS。
 * @param {string} [path] 文件路径，默认用仓库里的 BIOS
 * @returns {Sb97Rom}
 */
export function loadRom(path = DEFAULT_ROM_PATH) {
  const prg = new Uint8Array(readFileSync(path));
  if (prg.length !== PRG_SIZE) throw new Error(`文件长度不对：程序区应有 ${PRG_SIZE} 字节，实际 ${prg.length} 字节`);
  const crc = crc32(prg);
  if (crc !== PRG_CRC32) throw new Error(`程序区的 CRC32 是 ${crc.toString(16)}，不是 SB97 的 BIOS（${PRG_CRC32.toString(16)}）`);
  return { prg, crc };
}

/**
 * 取一页的字节。
 * @param {Sb97Rom} rom 已读取的 BIOS
 * @param {number} page 页号，0 到 63
 * @returns {Uint8Array}
 */
export function pageBytes(rom, page) {
  if (!Number.isInteger(page) || page < 0 || page >= PAGE_COUNT) throw new RangeError(`页号超出范围：${page}`);
  return rom.prg.subarray(page * PAGE_SIZE, (page + 1) * PAGE_SIZE);
}
