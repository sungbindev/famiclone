// ------------------------------------------------------------------------
// 名称：check-bbg-api.mjs
// 说明：检查步步高目标的汇编运行库：每个函数一个文件、有完整的说明，bbg.inc 与源码一致，每个测试程序里都用到了它，
//       DOS 调用、文件调用与 BIOS 调用的每个功能号都被封装或在 api-unwrapped.json 里写明不封装的原因。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-24
// 备注：用法 node vscode/tools/check/check-bbg-api.mjs。功能号取自 machines/bbg/src/include 下的符号定义文件：
//       DOS_ 开头（不含 DOS_ENTRY）、FILE_ 开头（不含 FILE_ENTRY）以及 bios-calls.inc 里“BIOS 1.0 实现的功能号”一节
//       的 BIOS_ 符号。一个功能号被某个库函数的源码引用了它的符号名，就算已封装。
// ------------------------------------------------------------------------

import { readdirSync, readFileSync } from 'node:fs';
import path from 'node:path';
import { fileURLToPath } from 'node:url';
import { INCLUDE_DIR, LIB_DIR, OUT_FILE, renderBbgInc, TARGET_DIR } from '../../../machines/bbg/tools/gen-bbg-inc.mjs';

/** 仓库根目录。 */
const REPO_ROOT = path.resolve(path.dirname(fileURLToPath(import.meta.url)), '..', '..', '..');

/** 测试程序目录。 */
const FIXTURE_DIR = path.join(REPO_ROOT, 'vscode', 'test', 'fixtures', 'bbg');

/** 不封装清单。 */
const UNWRAPPED_FILE = path.join(TARGET_DIR, 'api-unwrapped.json');

/**
 * 读文本文件，换行统一成 \n。
 * @param {string} file 文件。
 * @returns {string} 内容。
 */
function readText(file) {
  return readFileSync(file, 'utf8').replace(/\r\n/g, '\n');
}

/**
 * 列出目录下全部 .s 文件（递归）。
 * @param {string} dir 目录。
 * @returns {string[]} 绝对路径。
 */
function listSources(dir) {
  return readdirSync(dir, { recursive: true, withFileTypes: true })
    .filter((item) => item.isFile() && item.name.endsWith('.s'))
    .map((item) => path.join(item.parentPath, item.name));
}

/**
 * 读出一个符号定义文件里某一节（以 ;--- 开头的分节线起，到下一条分节线止）的“名字 = 数值”。
 * @param {string} file 符号定义文件名（在 src/include 下）。
 * @param {RegExp} sectionTitle 分节标题。
 * @param {RegExp} namePattern 要收集的符号名。
 * @returns {Map<string, number>} 符号名到数值。
 */
function readSection(file, sectionTitle, namePattern) {
  const result = new Map();
  let inSection = false;
  for (const line of readText(path.join(INCLUDE_DIR, file)).split('\n')) {
    if (line.startsWith(';---')) {
      inSection = sectionTitle.test(line);
      continue;
    }
    const match = /^([A-Z0-9_]+)\s*=\s*(\$[0-9A-Fa-f]+|\d+)/.exec(line);
    if (inSection && match && namePattern.test(match[1])) {
      const [, name, value] = match;
      result.set(name, value.startsWith('$') ? Number.parseInt(value.slice(1), 16) : Number(value));
    }
  }
  return result;
}

/**
 * 三类调用各有哪些功能号（符号名与数值）。
 * @returns {{ dos: Map<string, number>, file: Map<string, number>, bios: Map<string, number> }} 功能号表。
 */
export function readKnownFunctions() {
  return {
    dos: readSection('dos-calls.inc', /DOS 功能号/, /^DOS_(?!ENTRY$)/),
    file: readSection('dos-calls.inc', /文件调用功能号/, /^FILE_(?!ENTRY$)/),
    bios: readSection('bios-calls.inc', /BIOS 1\.0 实现的功能号/, /^BIOS_/)
  };
}

/**
 * 读出库里的公开函数。
 * @returns {Array<{ name: string, file: string, module: string, source: string, documented: boolean }>} 函数列表。
 */
export function readFunctions() {
  const functions = [];
  for (const file of listSources(LIB_DIR)) {
    const source = readText(file);
    const relative = path.relative(LIB_DIR, file).split(path.sep).join('/');
    if (!relative.includes('/')) continue; // 库根目录下是零页单元等内部文件，不是公开函数
    for (const match of source.matchAll(/^(bbg_[a-z0-9_]+):/gm)) {
      const name = match[1];
      const doc = new RegExp(`^; ${name}\\n;   说明：.+\\n;   入口：.+\\n;   出口：.+\\n${name}:`, 'm');
      functions.push({ name, file: relative, module: relative.split('/')[0] ?? '', source, documented: doc.test(source) });
    }
  }
  return functions;
}

/**
 * 读出不封装清单。
 * @returns {{ dos: Record<string, string>, file: Record<string, string>, bios: Record<string, string> }} 清单。
 */
export function readUnwrapped() {
  return JSON.parse(readText(UNWRAPPED_FILE));
}

/**
 * 每个功能号被哪些库函数封装。
 * @param {Array<{ name: string, source: string }>} functions 函数列表。
 * @returns {{ dos: Map<number, string[]>, file: Map<number, string[]>, bios: Map<number, string[]> }} 功能号到函数名。
 */
export function coverageOf(functions) {
  const known = readKnownFunctions();
  const result = {};
  for (const kind of ['dos', 'file', 'bios']) {
    const map = new Map();
    for (const [name, number] of known[kind]) {
      const users = functions.filter((fn) => new RegExp(`\\b${name}\\b`).test(fn.source)).map((fn) => fn.name);
      map.set(number, [...(map.get(number) ?? []), ...users]);
    }
    result[kind] = map;
  }
  return result;
}

/**
 * 检查运行库。
 * @returns {{ problems: string[], functions: ReturnType<typeof readFunctions> }} 发现的问题与函数列表。
 */
export function checkBbgApi() {
  const problems = [];
  const functions = readFunctions();

  const seen = new Map();
  for (const fn of functions) {
    seen.set(fn.name, [...(seen.get(fn.name) ?? []), fn.file]);
    if (fn.file !== `${fn.module}/${fn.name.replace(/_/g, '-')}.s`) problems.push(`${fn.name}：文件名应为 ${fn.module}/${fn.name.replace(/_/g, '-')}.s，实际是 ${fn.file}`);
    if (!fn.documented) problems.push(`${fn.name}：标号前要有“; 函数名”“;   说明：”“;   入口：”“;   出口：”四行注释`);
  }
  for (const [name, files] of seen) if (files.length > 1) problems.push(`${name}：定义了 ${files.length} 次：${files.join('、')}`);

  const tests = listSources(FIXTURE_DIR).map((file) => readText(file)).join('\n');
  for (const fn of functions) if (!new RegExp(`\\b${fn.name}\\b`).test(tests)) problems.push(`${fn.name}：没有测试程序用到它`);

  if (readText(OUT_FILE) !== renderBbgInc().replace(/\r\n/g, '\n')) problems.push('bbg.inc 与源码不一致：运行 node machines/bbg/tools/gen-bbg-inc.mjs 重新生成');

  const known = readKnownFunctions();
  const coverage = coverageOf(functions);
  const unwrapped = readUnwrapped();
  for (const kind of ['dos', 'file', 'bios']) {
    const numbers = new Set(known[kind].values());
    for (const number of [...numbers].sort((a, b) => a - b)) {
      const wrapped = (coverage[kind].get(number) ?? []).length > 0;
      const reason = unwrapped[kind]?.[String(number)];
      if (wrapped && reason) problems.push(`${kind} 功能 ${number}：已封装，不要写在不封装清单里`);
      if (!wrapped && !reason) problems.push(`${kind} 功能 ${number}：没有封装，也没有在不封装清单里写明原因`);
      if (reason !== undefined && reason.length < 8) problems.push(`${kind} 功能 ${number}：不封装的原因太短`);
    }
    for (const key of Object.keys(unwrapped[kind] ?? {})) if (!numbers.has(Number(key))) problems.push(`${kind} 功能 ${key}：不封装清单里的功能号不在符号文件里`);
  }
  return { problems, functions };
}

if (process.argv[1] && path.resolve(process.argv[1]) === fileURLToPath(import.meta.url)) {
  const { problems, functions } = checkBbgApi();
  for (const problem of problems) console.error(problem);
  console.log(`运行库 ${functions.length} 个函数，${problems.length} 个问题`);
  process.exitCode = problems.length === 0 ? 0 : 1;
}
