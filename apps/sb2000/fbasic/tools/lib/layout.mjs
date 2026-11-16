// ------------------------------------------------------------------------
// 名称：layout.mjs
// 说明：读取 analysis/data-layout.txt（数据区段布局）与 analysis/symbols.txt（符号名）。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-16
// 备注：布局每行写作 “起-止 类型 说明”，类型见 data-layout.txt 的开头注释；符号每行写作 “地址 名字 说明”，地址是十六进制，
//       地址前加 ! 表示它虽然落在映像的地址范围内，其实是映像之外的硬件或别名地址（如 NMI 向量），汇编时按外部符号处理。
// ------------------------------------------------------------------------
import { readFileSync } from 'node:fs';

/**
 * 去掉注释与空行。
 * @param {string} file 文件路径
 * @returns {string[]} 有内容的行
 */
function contentLines(file) {
  return readFileSync(file, 'utf8')
    .split(/\r?\n/)
    .map((line) => line.replace(/^\s*#.*$/, '').trimEnd())
    .filter((line) => line.trim() !== '');
}

/**
 * 读取数据区段布局。
 * @param {string} file 文件路径
 * @returns {{start: number, end: number, kind: string, pattern: string, note: string}[]} 区段，按起点排序
 * @throws {Error} 格式不对或类型不认识
 */
export function readLayout(file) {
  const result = contentLines(file).map((line) => {
    const match = /^([0-9A-Fa-f]{4})-([0-9A-Fa-f]{4})\s+(\S+)\s*(.*)$/.exec(line.trim());
    if (!match) throw new Error(`${file}：区段格式不对：${line}`);
    const [kind, pattern = ''] = match[3].split(':');
    if (!['fill', 'bytes', 'words', 'addrs', 'text', 'rec'].includes(kind)) throw new Error(`${file}：不认识的类型 ${match[3]}`);
    if (kind === 'rec' && !/^[bwa]+$/.test(pattern)) throw new Error(`${file}：记录模式不对：${match[3]}`);
    return { start: Number.parseInt(match[1], 16), end: Number.parseInt(match[2], 16), kind, pattern, note: match[4] };
  });
  return result.sort((left, right) => left.start - right.start);
}

/**
 * 把 addrs、words、rec 区段拆成元素。
 * @param {{start: number, end: number, kind: string, pattern: string}} range 区段
 * @returns {{start: number, size: number, kind: 'b'|'w'|'a'}[]} 元素
 */
export function dataElements(range) {
  const elements = [];
  if (range.kind === 'addrs' || range.kind === 'words') {
    for (let address = range.start; address <= range.end; address += 2) elements.push({ start: address, size: 2, kind: range.kind === 'addrs' ? 'a' : 'w' });
    return elements;
  }
  let address = range.start;
  while (address <= range.end) {
    for (const letter of range.pattern) {
      const size = letter === 'b' ? 1 : 2;
      elements.push({ start: address, size, kind: letter });
      address += size;
    }
  }
  return elements;
}

/**
 * 读取符号名。
 * @param {string} file 文件路径
 * @returns {Map<number, {name: string, note: string, external: boolean}>} 地址到名字的映射
 * @throws {Error} 格式不对或地址重复
 */
export function readSymbols(file) {
  const symbols = new Map();
  for (const line of contentLines(file)) {
    const match = /^(!)?\$?([0-9A-Fa-f]{4})\s+([A-Za-z_][A-Za-z0-9_]*)\s*(.*)$/.exec(line.trim());
    if (!match) throw new Error(`${file}：符号格式不对：${line}`);
    const address = Number.parseInt(match[2], 16);
    if (symbols.has(address)) throw new Error(`${file}：地址 $${match[2]} 重复`);
    symbols.set(address, { name: match[3], note: match[4], external: match[1] === '!' });
  }
  return symbols;
}
