// ------------------------------------------------------------------------
// 名称：sb97-memory.mjs
// 说明：SB97 的程序区在 CPU 地址空间里的几种摆法（视图），供静态分析按“哪一页在哪个地址”取字节。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-17
// 备注：$5000 的位 7 为 0 时程序区是 ROM：$5200 位 2 为 0 是 16K 模式，$8000 是第 $5000 页、$C000 固定是第 0 页；
//       位 2 为 1 是 32K 模式，$5000 的值是 32K 页号，$8000-$FFFF 是第 2n、2n+1 两页；
//       页号按 64 页取模；$5000 位 7 为 1 时 $8000 起换成 128K 的 RAM，不在静态分析的范围内。
// ------------------------------------------------------------------------
import { PAGE_COUNT, PAGE_SIZE, pageBytes } from './sb97-rom.mjs';

/** 程序窗口的起始地址 */
export const WINDOW_START = 0x8000;
/** 程序窗口的结束地址（含） */
export const WINDOW_END = 0xffff;
/** 低半窗口 $8000-$BFFF 与高半窗口 $C000-$FFFF 的分界 */
export const HIGH_HALF_START = 0xc000;

/**
 * @typedef {object} MemoryView
 * @property {string} name 视图名，如 window:2、bank:2、fixed
 * @property {number|null} lowPage $8000-$BFFF 的页号，没有则为 null
 * @property {number|null} highPage $C000-$FFFF 的页号，没有则为 null
 * @property {(address: number) => boolean} has 地址上有没有映射
 * @property {(address: number) => number} read 读一个字节，没有映射时返回 0
 * @property {(address: number) => {page: number, offset: number}|null} origin 地址对应的页号与页内偏移
 */

/**
 * 建立一个视图：$8000-$BFFF 放 lowPage，$C000-$FFFF 放 highPage，空着的半边没有映射。
 * @param {import('./sb97-rom.mjs').Sb97Rom} rom 已读取的 BIOS
 * @param {string} name 视图名
 * @param {number|null} lowPage 低半窗口的页号
 * @param {number|null} highPage 高半窗口的页号
 * @returns {MemoryView}
 */
export function createView(rom, name, lowPage, highPage) {
  const low = lowPage === null ? null : pageBytes(rom, lowPage);
  const high = highPage === null ? null : pageBytes(rom, highPage);
  const bytesOf = (address) => {
    if (address >= WINDOW_START && address < HIGH_HALF_START) return low;
    if (address >= HIGH_HALF_START && address <= WINDOW_END) return high;
    return null;
  };
  return {
    name,
    lowPage,
    highPage,
    has: (address) => bytesOf(address) !== null,
    read: (address) => {
      const bytes = bytesOf(address);
      return bytes === null ? 0 : bytes[address & (PAGE_SIZE - 1)];
    },
    origin: (address) => {
      const bytes = bytesOf(address);
      if (bytes === null) return null;
      return { page: address < HIGH_HALF_START ? lowPage : highPage, offset: address & (PAGE_SIZE - 1) };
    },
  };
}

/**
 * 16K 模式：$8000 放 page，$C000 固定是第 0 页。
 * @param {import('./sb97-rom.mjs').Sb97Rom} rom 已读取的 BIOS
 * @param {number} page 窗口页号
 * @returns {MemoryView}
 */
export function windowView(rom, page) {
  return createView(rom, `window:${page}`, page, 0);
}

/**
 * 32K 模式：$8000-$FFFF 放第 2n、2n+1 两页。
 * @param {import('./sb97-rom.mjs').Sb97Rom} rom 已读取的 BIOS
 * @param {number} bank 32K 页号，0 到 31
 * @returns {MemoryView}
 */
export function bankView(rom, bank) {
  if (!Number.isInteger(bank) || bank < 0 || bank >= PAGE_COUNT / 2) throw new RangeError(`32K 页号超出范围：${bank}`);
  return createView(rom, `bank:${bank}`, bank * 2, bank * 2 + 1);
}

/**
 * 只有固定的第 0 页（$C000-$FFFF），低半窗口没有映射。
 * @param {import('./sb97-rom.mjs').Sb97Rom} rom 已读取的 BIOS
 * @returns {MemoryView}
 */
export function fixedView(rom) {
  return createView(rom, 'fixed', null, 0);
}
