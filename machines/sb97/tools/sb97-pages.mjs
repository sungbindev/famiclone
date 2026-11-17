// ------------------------------------------------------------------------
// 名称：sb97-pages.mjs
// 说明：SB97 BIOS 的整体分析：给固定页、各窗口页、各 32K 页找入口，逐个分析，并判断每一页更像哪一种用法。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-17
// 备注：入口的来源：向量（$FFFA/$FFFC/$FFFE）、页首的三个跳转桩（$8000、$8003、$8006 的 JMP）、
//       第 0 页 $C000 起的功能表（功能号 $10-$6F，每项 2 字节）与 $FFE0 起的跳转桩、功能分派器用到的各页功能表（见 PAGE_FUNCTION_TABLES）；
//       另外 JSR 后面内嵌的跳转表由分析模块识别；
//       页的用法是静态推断：能干净地跑通（没有被拒绝的位置）又有足够多的代码才算“像”，是否真是那样要靠运行时的采集核对。
// ------------------------------------------------------------------------
import { analyzeView, isPlausibleEntry } from './sb97-analysis.mjs';
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
/**
 * 功能分派器（第 0 页 $C0C2）按功能号切换窗口页后跳转：
 * $10-$6F 查第 0 页的功能表；$70-$7E 切到第 63 页跳 $8000；$7F 切到第 47 页跳 $E0BE（落在第 0 页里）；
 * $80-$9F 切到第 64 页（按 64 取模就是第 0 页）查 $8000 起的表；$A0-$BF 切到 RAM 模式的 PRAM 页（运行时才有，静态看不到）；
 * $C0-$EF 切到第 1 页查 $8000 起的表；$F0-$FF 切到第 5 页查 $B380 起的表；$00-$0F 查 RAM 里 $013D 起的三字节表（运行时填，静态看不到）。
 */
export const PAGE_FUNCTION_TABLES = [
  { page: 1, start: 0x8000, firstNumber: 0xc0, count: 0x30 },
  { page: 5, start: 0xb380, firstNumber: 0xf0, count: 0x10 },
];
/** 窗口页里直接跳进去的入口 */
export const PAGE_ENTRIES = [{ page: 63, address: 0x8000, name: 'fn_70' }];
/** 落在第 0 页里的功能入口 */
export const FIXED_FUNCTION_ENTRIES = [{ address: 0xe0be, name: 'fn_7F' }];
/** 判断“像一个程序”的最少代码字节数 */
export const MIN_CODE_BYTES = 64;
/** 每个被拒绝的位置至少要对应这么多代码字节 */
export const REJECT_RATIO = 400;

const word = (view, address) => view.read(address) | (view.read(address + 1) << 8);

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
 * 一个窗口页的入口：页首的跳转桩、页里的功能表、直接跳进去的入口。
 * @param {import('./sb97-memory.mjs').MemoryView} view 视图
 * @param {number} page 窗口页号
 * @returns {import('./sb97-analysis.mjs').Seed[]}
 */
export function windowPageSeeds(view, page) {
  const seeds = stubSeeds(view, ENTRY_STUBS, 'entry');
  for (const table of PAGE_FUNCTION_TABLES) {
    if (table.page !== page) continue;
    for (let index = 0; index < table.count; index += 1) {
      const target = word(view, table.start + index * 2);
      if (isPlausibleEntry(view, target)) seeds.push({ address: target, name: `fn_${(table.firstNumber + index).toString(16).toUpperCase()}` });
    }
  }
  for (const entry of PAGE_ENTRIES) if (entry.page === page && isPlausibleEntry(view, entry.address)) seeds.push({ address: entry.address, name: entry.name });
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
  const fixed = analyzeView(fixedMemory, [
    ...vectorSeeds(fixedMemory),
    ...stubSeeds(fixedMemory, FIXED_STUBS, 'api'),
    ...functionTableSeeds(fixedMemory),
    ...FIXED_FUNCTION_ENTRIES.filter((entry) => isPlausibleEntry(fixedMemory, entry.address)),
  ]);

  const windows = new Map();
  for (let page = 1; page < PAGE_COUNT; page += 1) {
    const view = windowView(rom, page);
    windows.set(page, analyzeView(view, windowPageSeeds(view, page)));
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
