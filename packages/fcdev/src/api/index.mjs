// ------------------------------------------------------------------------
// 名称：index.mjs
// 说明：目标的 C 接口文档：从头文件读出函数列表，按名称、模块、系统调用号查询，格式化成文字。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-11
// 备注：fcdev api 命令的实现；没有 include 目录或头文件里没有带文档注释的函数的目标，视为没有 API 文档。
// ------------------------------------------------------------------------
import { readdirSync, readFileSync } from 'node:fs';
import path from 'node:path';
import { parseHeaderFunctions } from './header-parser.mjs';

export { parseHeaderFunctions };

/**
 * 读出目标全部带文档注释的函数。
 * @param {{dir: string, include: string[]}} target 目标（loadTarget 的结果）
 * @returns {import('./header-parser.mjs').ApiFunction[]} 函数列表，按头文件名与声明顺序
 */
export function loadTargetApi(target) {
  const functions = [];
  for (const includeDir of target.include) {
    const dir = path.join(target.dir, includeDir);
    let names;
    try {
      names = readdirSync(dir).filter((name) => name.endsWith('.h')).sort();
    } catch {
      continue;
    }
    for (const name of names) {
      functions.push(...parseHeaderFunctions(readFileSync(path.join(dir, name), 'utf8').replace(/\r\n/g, '\n'), name));
    }
  }
  return functions;
}

/**
 * 头文件名对应的模块名：sb2000-file.h 的模块是 file。
 * @param {string} header 头文件名
 * @returns {string} 模块名
 */
export function moduleOf(header) {
  return header.replace(/^[a-z0-9]+-/, '').replace(/\.h$/, '');
}

/**
 * 按模块分组。
 * @param {import('./header-parser.mjs').ApiFunction[]} functions 函数列表
 * @returns {Map<string, import('./header-parser.mjs').ApiFunction[]>} 模块名到函数列表，保持头文件顺序
 */
export function groupByModule(functions) {
  const groups = new Map();
  for (const fn of functions) {
    const key = moduleOf(fn.header);
    groups.set(key, [...(groups.get(key) ?? []), fn]);
  }
  return groups;
}

/**
 * 查询函数。依次尝试：函数名完全相同；模块名（file 或 sb2000-file.h）；系统调用号（如 $7B、$7B/$09，不分大小写）；名称、说明里包含该词。
 * @param {import('./header-parser.mjs').ApiFunction[]} functions 函数列表
 * @param {string} query 查询词
 * @returns {{kind: 'function'|'module'|'syscall'|'text'|'none', matches: import('./header-parser.mjs').ApiFunction[]}} 查询结果
 */
export function queryApi(functions, query) {
  const word = query.trim();
  const exact = functions.filter((fn) => fn.name === word);
  if (exact.length > 0) return { kind: 'function', matches: exact };
  const moduleName = moduleOf(word.includes('.h') ? word : `x-${word}.h`);
  const byModule = functions.filter((fn) => moduleOf(fn.header) === moduleName);
  if (byModule.length > 0) return { kind: 'module', matches: byModule };
  const lower = word.toLowerCase();
  if (/^\$?[0-9a-f]{2}(\/\$?[0-9a-f]{2})?$/.test(lower)) {
    const [number, sub] = lower.replace(/\$/g, '').split('/');
    const bySyscall = functions.filter((fn) => fn.syscalls.some((tag) => syscallMatches(tag, number, sub)));
    if (bySyscall.length > 0) return { kind: 'syscall', matches: bySyscall };
  }
  const byText = functions.filter((fn) => fn.name.includes(lower) || fn.summary.toLowerCase().includes(lower));
  return { kind: byText.length > 0 ? 'text' : 'none', matches: byText };
}

/**
 * 一个 @syscall 标签是否涉及指定的功能号（与子功能）。标签可以是“$7B/$09”“$30/$04-$07”“$3C、$3D”“$7B/$01、$7B/$06”这样的写法。
 * @param {string} tag 标签内容
 * @param {string} number 功能号，两位十六进制小写
 * @param {string} [sub] 子功能号，两位十六进制小写；省略时只比功能号
 * @returns {boolean} 是否涉及
 */
export function syscallMatches(tag, number, sub) {
  for (const part of tag.split('、')) {
    const match = /\$([0-9A-Fa-f]{2})(?![0-9A-Fa-f])(?:\/\$([0-9A-Fa-f]{2})(?:-\$([0-9A-Fa-f]{2}))?)?/.exec(part);
    if (!match || match[1].toLowerCase() !== number) continue;
    if (sub === undefined || match[2] === undefined) return true;
    const low = parseInt(match[2], 16);
    const high = match[3] === undefined ? low : parseInt(match[3], 16);
    const wanted = parseInt(sub, 16);
    if (wanted >= low && wanted <= high) return true;
  }
  return false;
}

/**
 * 格式化一个函数的完整文档。
 * @param {import('./header-parser.mjs').ApiFunction} fn 函数
 * @returns {string} 多行文字
 */
export function formatFunction(fn) {
  const lines = [`${fn.signature};`, `  所在：${fn.header}`];
  if (fn.syscalls.length > 0) lines.push(`  系统调用：${fn.syscalls.join('；')}`);
  lines.push('', ...fn.description.split('\n').map((line) => `  ${line}`.trimEnd()));
  if (fn.params.length > 0) {
    lines.push('', '  参数：');
    for (const param of fn.params) lines.push(`    ${param}  ${fn.paramText[param] ?? ''}`.trimEnd());
  }
  if (fn.hasReturnDoc) lines.push('', `  返回：${fn.returnText}`);
  return lines.join('\n');
}

/**
 * 格式化函数的一行摘要。
 * @param {import('./header-parser.mjs').ApiFunction} fn 函数
 * @param {number} width 名称列宽
 * @returns {string} 一行文字
 */
export function formatSummary(fn, width) {
  return `  ${fn.name.padEnd(width)}  ${fn.summary}`;
}
