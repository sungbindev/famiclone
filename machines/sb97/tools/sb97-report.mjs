// ------------------------------------------------------------------------
// 名称：sb97-report.mjs
// 说明：SB97 BIOS 静态分析的命令行：文件信息、每页的推断、寄存器访问统计、$5000 的取值、生成各页的清单。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-17
// 备注：用法见 package.json 的脚本或不带参数运行本文件；清单写到 listing/ 目录（提交进仓库，BIOS 或分析规则变了要重新生成）。
// ------------------------------------------------------------------------
import { mkdirSync, writeFileSync } from 'node:fs';
import { dirname, resolve } from 'node:path';
import { fileURLToPath } from 'node:url';
import { collectAccesses, summarizeAccesses } from './sb97-analysis.mjs';
import { renderListing } from './sb97-listing.mjs';
import { HIGH_HALF_START } from './sb97-memory.mjs';
import { analyzeRom } from './sb97-pages.mjs';
import { MAPPER_NUMBER, PAGE_COUNT, PAGE_SIZE, loadRom } from './sb97-rom.mjs';

const hex = (value, width = 4) => value.toString(16).toUpperCase().padStart(width, '0');

/**
 * 参与统计的分析：固定页、被推断为程序的窗口页、被推断为 32K 高半页的整页。
 * @param {import('./sb97-pages.mjs').RomAnalysis} result 整体分析
 * @returns {{name: string, analysis: import('./sb97-analysis.mjs').Analysis, owns: (pc: number) => boolean}[]} owns 判断一个指令地址是不是这次分析自己的页（窗口视图里的第 0 页归固定页）
 */
export function programAnalyses(result) {
  const list = [{ name: '第 0 页', analysis: result.fixed, owns: () => true }];
  for (const { page, role } of result.pages) {
    if (role === 'window') list.push({ name: `第 ${page} 页（16K 窗口）`, analysis: result.windows.get(page), owns: (pc) => pc < HIGH_HALF_START });
    if (role === 'bank-upper') list.push({ name: `第 ${page - 1}、${page} 页（32K 页）`, analysis: result.banks.get(page >> 1), owns: () => true });
  }
  return list;
}

/**
 * 一份分析自己拥有的访问。
 * @param {ReturnType<typeof programAnalyses>[number]} item 参与统计的分析
 * @returns {import('./sb97-analysis.mjs').Access[]}
 */
function ownAccesses(item) {
  return collectAccesses(item.analysis).filter((access) => item.owns(access.pc));
}

/**
 * 一页的清单：按推断的用法选视图与地址范围。
 * @param {import('./sb97-pages.mjs').RomAnalysis} result 整体分析
 * @param {number} page 页号
 * @returns {string}
 */
export function pageListing(result, page) {
  const { role } = result.pages[page];
  if (role === 'fixed') return renderListing(result.fixed, { title: '第 0 页（固定在 $C000-$FFFF）', first: HIGH_HALF_START, last: 0xffff });
  if (role === 'bank-upper') {
    return renderListing(result.banks.get(page >> 1), { title: `第 ${page} 页（32K 页的高半页，在 $C000-$FFFF）`, first: HIGH_HALF_START, last: 0xffff });
  }
  const window = result.windows.get(page);
  return renderListing(window, { title: `第 ${page} 页（16K 窗口，在 $8000-$BFFF）${role === 'data' ? '，没有找到代码' : ''}`, first: 0x8000, last: HIGH_HALF_START - 1 });
}

function printInfo(rom) {
  console.log(`程序区 ${rom.prg.length} 字节，${PAGE_COUNT} 页，CRC32 ${hex(rom.crc, 8)}，按 mapper ${MAPPER_NUMBER} 处理`);
}

function printPages(result) {
  console.log('页  用法        窗口代码  32K 代码');
  for (const item of result.pages) console.log(`${String(item.page).padStart(2)}  ${item.role.padEnd(10)}  ${String(item.windowCode).padStart(8)}  ${String(item.bankCode).padStart(8)}`);
}

function printAccesses(result) {
  const all = [];
  for (const item of programAnalyses(result)) all.push(...ownAccesses(item));
  console.log('地址   读  写  写入的立即数');
  for (const item of summarizeAccesses(all)) {
    if (item.address >= 0x6000) continue;
    console.log(`$${hex(item.address)}  ${String(item.reads).padStart(3)} ${String(item.writes).padStart(3)}  ${item.values.map((value) => `$${hex(value, 2)}`).join(' ')}`);
  }
}

function printBanks(result) {
  console.log('写 $5000 的立即数（按所在的分析）：');
  for (const item of programAnalyses(result)) {
    const values = new Set();
    for (const access of ownAccesses(item)) if (access.address === 0x5000 && access.value !== undefined) values.add(access.value);
    if (values.size > 0) console.log(`${item.name}：${[...values].sort((a, b) => a - b).map((value) => `$${hex(value, 2)}`).join(' ')}`);
  }
}

function writeListings(result, directory) {
  mkdirSync(directory, { recursive: true });
  for (let page = 0; page < PAGE_COUNT; page += 1) writeFileSync(resolve(directory, `page-${String(page).padStart(2, '0')}.asm`), pageListing(result, page), 'utf8');
  console.log(`已写 ${PAGE_COUNT} 个清单到 ${directory}（每页 ${PAGE_SIZE} 字节）`);
}

function main(argv) {
  const [command = 'help', argument] = argv;
  const rom = loadRom();
  if (command === 'info') return printInfo(rom);
  const result = analyzeRom(rom);
  if (command === 'pages') return printPages(result);
  if (command === 'io') return printAccesses(result);
  if (command === 'banks') return printBanks(result);
  if (command === 'listing') return writeListings(result, resolve(argument ?? resolve(dirname(fileURLToPath(import.meta.url)), '../listing')));
  console.log('用法：node tools/sb97-report.mjs info | pages | io | banks | listing [目录]');
  return undefined;
}

if (process.argv[1] === fileURLToPath(import.meta.url)) main(process.argv.slice(2));
