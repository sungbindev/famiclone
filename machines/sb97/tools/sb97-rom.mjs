// ------------------------------------------------------------------------
// 名称：sb97-rom.mjs
// 说明：SB97 BIOS 文件的读取与检查：iNES 文件头、程序区大小、校验和，以及按 16K 页取字节。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-17
// 备注：文件头里的 mapper 号不对（NES 2.0 头，按头解析得到 366），这台机器按 mapper 168 处理，
//       文件用程序区的 CRC32 认定；程序区 1MB，共 64 页、每页 16K，没有 CHR。
// ------------------------------------------------------------------------
import { readFileSync } from 'node:fs';
import { dirname, resolve } from 'node:path';
import { fileURLToPath } from 'node:url';

/** iNES 文件头的字节数 */
export const HEADER_SIZE = 16;
/** 一页的字节数（16K） */
export const PAGE_SIZE = 0x4000;
/** 程序区的页数 */
export const PAGE_COUNT = 64;
/** 程序区的字节数 */
export const PRG_SIZE = PAGE_SIZE * PAGE_COUNT;
/** 程序区的 CRC32，对应本机型的 BIOS */
export const PRG_CRC32 = 0x40a4c574;
/** 本机型按这个 mapper 号处理（文件头里的号不对） */
export const MAPPER_NUMBER = 168;

/** 仓库里 SB97.nes 的默认位置 */
export const DEFAULT_ROM_PATH = resolve(dirname(fileURLToPath(import.meta.url)), '../../../reference/rom/小霸王/SB97.nes');

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
 * 解析 iNES 文件头。
 * @param {Uint8Array} bytes 文件内容
 * @returns {{prgPages: number, chrPages: number, flags6: number, flags7: number, nes2: boolean, headerMapper: number, verticalMirroring: boolean}}
 */
export function parseHeader(bytes) {
  if (bytes.length < HEADER_SIZE || bytes[0] !== 0x4e || bytes[1] !== 0x45 || bytes[2] !== 0x53 || bytes[3] !== 0x1a) {
    throw new Error('不是 iNES 文件：缺少 NES 0x1A 标记');
  }
  const flags6 = bytes[6];
  const flags7 = bytes[7];
  const nes2 = (flags7 & 0x0c) === 0x08;
  const low = (flags6 >> 4) | (flags7 & 0xf0);
  return {
    prgPages: bytes[4],
    chrPages: bytes[5],
    flags6,
    flags7,
    nes2,
    headerMapper: nes2 ? low | ((bytes[8] & 0x0f) << 8) : low,
    verticalMirroring: (flags6 & 1) !== 0,
  };
}

/**
 * @typedef {object} Sb97Rom
 * @property {ReturnType<typeof parseHeader>} header 文件头
 * @property {Uint8Array} prg 程序区（不含文件头）
 * @property {number} crc 程序区的 CRC32
 */

/**
 * 读取并检查 BIOS 文件：程序区必须是 1MB，CRC32 必须是已知的 BIOS。
 * @param {string} [path] 文件路径，默认用仓库里的 SB97.nes
 * @returns {Sb97Rom}
 */
export function loadRom(path = DEFAULT_ROM_PATH) {
  const bytes = new Uint8Array(readFileSync(path));
  const header = parseHeader(bytes);
  if (header.prgPages !== PAGE_COUNT || header.chrPages !== 0) {
    throw new Error(`程序区应为 ${PAGE_COUNT} 页、没有 CHR，实际 PRG ${header.prgPages} 页、CHR ${header.chrPages} 页`);
  }
  const prg = bytes.subarray(HEADER_SIZE, HEADER_SIZE + PRG_SIZE);
  if (prg.length !== PRG_SIZE) throw new Error(`文件长度不足：程序区应有 ${PRG_SIZE} 字节，实际 ${prg.length} 字节`);
  const crc = crc32(prg);
  if (crc !== PRG_CRC32) throw new Error(`程序区的 CRC32 是 ${crc.toString(16)}，不是 SB97 的 BIOS（${PRG_CRC32.toString(16)}）`);
  return { header, prg, crc };
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
