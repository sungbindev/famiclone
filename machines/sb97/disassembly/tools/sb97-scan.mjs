// ------------------------------------------------------------------------
// 名称：sb97-scan.mjs
// 说明：小霸王 97 BIOS 的静态分析命令行：页地图、寄存器访问表、页号立即数、指定页的反汇编清单。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-17
// 备注：用法见 README.md。清单写到 out/，其余结果打印到终端。
// ------------------------------------------------------------------------
import { mkdirSync, writeFileSync } from 'node:fs';
import path from 'node:path';
import { fileURLToPath } from 'node:url';
import { findBankImmediates, formatListing, scanRegisterAccess, traceCode } from './sb97-analysis.mjs';
import { FIXED_PAGE, WINDOW_BASE, classifyPage, createCpuView, loadRom, pageVectors } from './sb97-rom.mjs';

const TOOLS_DIR = path.dirname(fileURLToPath(import.meta.url));

/** 原始 BIOS 文件，位置相对仓库根目录固定。 */
export const ROM_PATH = path.resolve(TOOLS_DIR, '../../../../reference/rom/小霸王/SB97.nes');

const OUT_DIR = path.resolve(TOOLS_DIR, '../out');
const hex = (value, width) => value.toString(16).toUpperCase().padStart(width, '0');

/** 打印每一页的判别结果与向量。 */
function printPages(rom) {
  console.log('页  判别       官方   跳转   返回   跳转在范围  填充   NMI   RESET IRQ');
  rom.pages.forEach((page, index) => {
    const info = classifyPage(page);
    const vectors = pageVectors(page);
    const ratio = (value) => value.toFixed(3);
    console.log(
      `${String(index).padStart(2)}  ${info.kind.padEnd(9)}  ${ratio(info.officialRatio)}  ${ratio(info.jumpRatio)}  ${ratio(info.rtsRatio)}  ${ratio(info.inRangeRatio)}       ${ratio(info.fillRatio)}  ${hex(vectors.nmi, 4)}  ${hex(vectors.reset, 4)}  ${hex(vectors.irq, 4)}`,
    );
  });
}

/** 取判为代码的页。 */
function codePages(rom) {
  const numbers = rom.pages.map((_, index) => index).filter((index) => classifyPage(rom.pages[index]).kind === 'code-like');
  return { numbers, pages: numbers.map((index) => rom.pages[index]) };
}

/** 打印寄存器访问表。 */
function printRegisters(rom) {
  const { numbers, pages } = codePages(rom);
  const table = scanRegisterAccess(pages, numbers);
  console.log('地址  读  写  页');
  for (const [address, entry] of [...table.entries()].sort((a, b) => a[0] - b[0])) {
    if (entry.read + entry.write < 2) continue;
    console.log(`${hex(address, 4)}  ${String(entry.read).padStart(3)} ${String(entry.write).padStart(3)}  ${[...entry.pages].sort((a, b) => a - b).join(',')}`);
  }
}

/** 打印写 $5000 前装入的立即数。 */
function printBankValues(rom) {
  const { numbers, pages } = codePages(rom);
  const table = findBankImmediates(pages, numbers);
  for (const [value, where] of [...table.entries()].sort((a, b) => a[0] - b[0])) {
    console.log(`$${hex(value, 2)}  出现在页 ${[...where].sort((a, b) => a - b).join(',')}`);
  }
}

/**
 * 生成一页的清单。第 0 页（固定页）按 $C000 起，其余页按 $8000 起。
 * @param {object} rom 解析结果
 * @param {number} pageNumber 页号
 * @param {number[]} extraEntries 额外入口
 */
function writeListing(rom, pageNumber, extraEntries) {
  const fixed = pageNumber === FIXED_PAGE;
  const view = createCpuView(rom, fixed ? 1 : pageNumber);
  const base = fixed ? 0xc000 : WINDOW_BASE;
  const vectors = pageVectors(rom.pages[pageNumber]);
  const entries = fixed ? [vectors.nmi, vectors.reset, vectors.irq, ...extraEntries] : [base, ...extraEntries];
  const range = fixed ? (address) => address >= 0xc000 && address <= 0xffff : (address) => address >= base && address < base + 0x4000;
  const code = traceCode(view.read, entries.filter(range), range);
  const lines = formatListing(view.read, code, base, 0x4000);
  mkdirSync(OUT_DIR, { recursive: true });
  const file = path.join(OUT_DIR, `page-${String(pageNumber).padStart(2, '0')}.asm`);
  writeFileSync(file, `${lines.join('\n')}\n`, 'utf8');
  console.log(`页 ${pageNumber}：${code.size} 条可达指令，清单写入 ${file}`);
}

function main(argv) {
  const [command, ...rest] = argv;
  const rom = loadRom(ROM_PATH);
  switch (command) {
    case 'pages': return printPages(rom);
    case 'io': return printRegisters(rom);
    case 'bank': return printBankValues(rom);
    case 'listing': {
      const pageNumber = Number(rest[0]);
      const extra = rest.slice(1).map((value) => parseInt(value, 16));
      return writeListing(rom, pageNumber, extra);
    }
    default:
      console.log('用法：node tools/sb97-scan.mjs <pages | io | bank | listing 页号 [入口十六进制...]>');
      return undefined;
  }
}

if (process.argv[1] && path.resolve(process.argv[1]) === fileURLToPath(import.meta.url)) main(process.argv.slice(2));
