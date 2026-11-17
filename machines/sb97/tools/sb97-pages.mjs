// ------------------------------------------------------------------------
// 名称：sb97-pages.mjs
// 说明：SB97 BIOS 的整体分析：给固定页、各窗口页、各 32K 页找入口，逐个分析，并判断每一页更像哪一种用法。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-17
// 备注：入口的来源：向量（$FFFA/$FFFC/$FFFE）、页首的三个跳转桩（$8000、$8003、$8006 的 JMP）、
//       第 0 页 $C000 起的功能表（功能号 $10-$6F，每项 2 字节）与 $FFE0 起的跳转桩；
//       页的用法是静态推断：能干净地跑通（没有被拒绝的位置）又有足够多的代码才算“像”，是否真是那样要靠运行时的采集核对。
// ------------------------------------------------------------------------
import { decodeInstruction } from '../../sb2000/disassembly/tools/disasm.mjs';
import { analyzeView } from './sb97-analysis.mjs';
import { bankView, fixedView, windowView } from './sb97-memory.mjs';
import { PAGE_COUNT } from './sb97-rom.mjs';

/** 功能表的起点与项数：功能号 $10-$6F，每项一个字（低字节在前） */
export const FUNCTION_TABLE_START = 0xc000;
export const FUNCTION_TABLE_FIRST_NUMBER = 0x10;
export const FUNCTION_TABLE_COUNT = 0x60;
/** 功能表里表示“没有实现”的默认入口 */
export const DEFAULT_FUNCTION_ENTRY = 0xc0c0;
/** 页首的三个跳转桩 */
export const ENTRY_STUBS = [0x8000, 0x8003, 0x8006];
/** 第 0 页末尾的跳转桩 */
export const FIXED_STUBS = [0xffe0, 0xffe3, 0xffe6, 0xffe9];
/** 判断“像一个程序”的最少代码字节数 */
export const MIN_CODE_BYTES = 64;
/** 每个被拒绝的位置至少要对应这么多代码字节 */
export const REJECT_RATIO = 400;

const word = (view, address) => view.read(address) | (view.read(address + 1) << 8);

/**
 * 地址上是不是一条能当入口的官方指令。
 * @param {import('./sb97-memory.mjs').MemoryView} view 视图
 * @param {number} address 地址
 * @returns {boolean}
 */
export function isPlausibleEntry(view, address) {
  if (!view.has(address)) return false;
  const ins = decodeInstruction(view.read, address);
  return ins.info.official && ins.info.mnemonic !== 'BRK' && view.has(address + ins.length - 1);
}

/**
 * 三个向量指向的、像入口的地址。
 * @param {import('./sb97-memory.mjs').MemoryView} view 视图
 * @returns {import('./sb97-analysis.mjs').Seed[]}
 */
export function vectorSeeds(view) {
  const seeds = [];
  for (const [address, name] of [[0xfffa, 'nmi'], [0xfffc, 'reset'], [0xfffe, 'irq']]) {
    if (!view.has(address + 1)) continue;
    const target = word(view, address);
    if (isPlausibleEntry(view, target)) seeds.push({ address: target, name });
  }
  return seeds;
}

/**
 * 以 JMP 绝对开头的跳转桩，目标要落在视图里。
 * @param {import('./sb97-memory.mjs').MemoryView} view 视图
 * @param {number[]} addresses 桩的地址
 * @param {string} prefix 入口名的前缀
 * @returns {import('./sb97-analysis.mjs').Seed[]}
 */
export function stubSeeds(view, addresses, prefix) {
  const seeds = [];
  for (const address of addresses) {
    if (!view.has(address + 2) || view.read(address) !== 0x4c) continue;
    seeds.push({ address, name: `${prefix}_${address.toString(16).toUpperCase()}` });
  }
  return seeds;
}

/**
 * 第 0 页功能表里指向实际代码的入口。
 * @param {import('./sb97-memory.mjs').MemoryView} view 视图（要有第 0 页）
 * @returns {import('./sb97-analysis.mjs').Seed[]}
 */
export function functionTableSeeds(view) {
  const seeds = [];
  for (let index = 0; index < FUNCTION_TABLE_COUNT; index += 1) {
    const target = word(view, FUNCTION_TABLE_START + index * 2);
    if (target === DEFAULT_FUNCTION_ENTRY || !isPlausibleEntry(view, target)) continue;
    seeds.push({ address: target, name: `fn_${(FUNCTION_TABLE_FIRST_NUMBER + index).toString(16).toUpperCase().padStart(2, '0')}` });
  }
  return seeds;
}

/**
 * @typedef {object} PageReport
 * @property {number} page 页号
 * @property {'fixed'|'window'|'bank-upper'|'data'} role 推断的用法：window 是 16K 模式下 $8000 的程序页，bank-upper 是 32K 模式的高半页
 * @property {number} windowCode 按 16K 窗口分析得到的代码字节数
 * @property {number} bankCode 按 32K 页分析得到的代码字节数（整对的两页合计）
 */

/**
 * @typedef {object} RomAnalysis
 * @property {import('./sb97-analysis.mjs').Analysis} fixed 固定页的分析
 * @property {Map<number, import('./sb97-analysis.mjs').Analysis>} windows 第 1-63 页按 16K 窗口的分析
 * @property {Map<number, import('./sb97-analysis.mjs').Analysis>} banks 第 0-31 个 32K 页的分析
 * @property {PageReport[]} pages 每一页的推断
 */

/** 分析是否“像一个程序”：代码够多，被拒绝的位置不多（JSR 后面内嵌数据会让分析走进数据，所以允许少量） */
const looksLikeProgram = (analysis) => analysis.codeBytes >= MIN_CODE_BYTES && analysis.rejected.length * REJECT_RATIO <= analysis.codeBytes;

/**
 * 分析整个 BIOS。
 * @param {import('./sb97-rom.mjs').Sb97Rom} rom 已读取的 BIOS
 * @returns {RomAnalysis}
 */
export function analyzeRom(rom) {
  const fixedMemory = fixedView(rom);
  const fixed = analyzeView(fixedMemory, [...vectorSeeds(fixedMemory), ...stubSeeds(fixedMemory, FIXED_STUBS, 'api'), ...functionTableSeeds(fixedMemory)]);

  const windows = new Map();
  for (let page = 1; page < PAGE_COUNT; page += 1) {
    const view = windowView(rom, page);
    windows.set(page, analyzeView(view, stubSeeds(view, ENTRY_STUBS, 'entry')));
  }
  const banks = new Map();
  for (let bank = 0; bank < PAGE_COUNT / 2; bank += 1) {
    const view = bankView(rom, bank);
    banks.set(bank, analyzeView(view, [...vectorSeeds(view), ...stubSeeds(view, ENTRY_STUBS, 'entry')]));
  }

  const pages = [{ page: 0, role: 'fixed', windowCode: 0, bankCode: 0 }];
  for (let page = 1; page < PAGE_COUNT; page += 1) {
    const window = windows.get(page);
    const bank = banks.get(page >> 1);
    let role = 'data';
    if (looksLikeProgram(window)) role = 'window';
    else if ((page & 1) === 1 && vectorSeeds(bank.view).some((seed) => seed.name === 'reset') && looksLikeProgram(bank)) role = 'bank-upper';
    pages.push({ page, role, windowCode: window.codeBytes, bankCode: bank.codeBytes });
  }
  return { fixed, windows, banks, pages };
}
