// ------------------------------------------------------------------------
// 名称：rom.mjs
// 说明：Family BASIC v3 的 ROM 文件位置、PRG 与 CHR 的大小和读取方法，以及项目根目录与 PRG 的加载地址。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-16
// 备注：其余工具都从这里取 ROM，不各自写路径。
// ------------------------------------------------------------------------
import { readFileSync } from 'node:fs';
import { dirname, join, resolve } from 'node:path';
import { fileURLToPath } from 'node:url';

export const ROOT = resolve(dirname(fileURLToPath(import.meta.url)), '..', '..');
export const BASE = 0x8000;
export const ROM_FILE = join(ROOT, '..', '..', '..', 'reference', 'nes-games', 'Family BASIC (Japan) (v3.0).nes');
const INES_HEADER = 16;
export const PRG_SIZE = 32768;
export const CHR_SIZE = 8192;

/**
 * 读取 ROM 里的 PRG 与 CHR。
 * @returns {{prg: Buffer, chr: Buffer}} 两段数据
 */
export function readRom() {
  const rom = readFileSync(ROM_FILE);
  return { prg: rom.subarray(INES_HEADER, INES_HEADER + PRG_SIZE), chr: rom.subarray(INES_HEADER + PRG_SIZE, INES_HEADER + PRG_SIZE + CHR_SIZE) };
}
