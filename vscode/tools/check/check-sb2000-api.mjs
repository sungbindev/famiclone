// ------------------------------------------------------------------------
// 名称：check-sb2000-api.mjs
// 说明：检查 SB2000 目标的 C 接口：头文件里的每个函数都有完整的文档注释和实现，库里没有头文件之外的公开函数，系统调用的封装覆盖都有交代。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-11
// 备注：用法 node vscode/tools/check/check-sb2000-api.mjs [--list]；--list 另外列出各函数对应的系统调用（文档注释里的 @syscall 标签）。
//       头文件是接口文档的唯一来源：注释写函数说明、@param、@return、@syscall；库内部使用的辅助函数以 sbi_ 开头，不在检查范围内。
//       覆盖检查：KNOWN_FUNCTIONS 是 BIOS 里有处理例程的全部功能号（BIOS 里登记过处理例程的功能号，共 83 个），每个都必须“被某个函数的 @syscall 标签涉及”
//       或“在 targets/sb2000/api-unwrapped.json 的 functions 里写明不封装的原因”，两者不能同时成立；封装了一部分子功能的，在 partial 里写明没封装的部分。
// ------------------------------------------------------------------------
import { readdirSync, readFileSync } from 'node:fs';
import path from 'node:path';
import { fileURLToPath } from 'node:url';
import { parseHeaderFunctions, syscallMatches } from '../../out/shared/c-api/api-functions.js';

export { parseHeaderFunctions };

/** 仓库根目录 */
const REPO_ROOT = path.resolve(path.dirname(fileURLToPath(import.meta.url)), '..', '..', '..');

/** SB2000 目标目录 */
const TARGET_DIR = path.join(REPO_ROOT, 'vscode', 'targets', 'sb2000');

/** 不检查函数声明的头文件 */
const SKIPPED_HEADERS = new Set(['sb2000-symbols.h']);

/** BIOS 里有处理例程的功能号（两位大写十六进制），含 $00-$0F 的运行时登记表之外的全部 */
export const KNOWN_FUNCTIONS = [
  '10', '11', '12', '13', '14', '15', '17', '18', '19', '1A', '1B', '1C', '1D', '1E', '1F',
  '24', '25', '26', '27', '28', '29', '2A', '2B', '2C',
  '30', '31', '32', '33', '34',
  '3B', '3C', '3D', '3E', '3F', '40', '41', '42', '43', '44', '45', '46', '47', '48',
  '4A', '4B', '4C', '4D', '4E', '4F', '50', '51', '52', '53', '54',
  '59', '5A', '5B', '5C', '5D', '5E', '5F', '60', '61',
  '65', '66', '6A', '6B', '6C', '6D',
  '74', '75', '76', '77', '78', '79', '7A',
  '7B', '7C', '7D', '7E', '7F', '80', '81',
];

/**
 * 列出目录下指定扩展名的文件。
 * @param {string} dir 目录
 * @param {RegExp} pattern 文件名匹配
 * @returns {string[]} 文件的绝对路径
 */
function listFiles(dir, pattern) {
  return readdirSync(dir, { recursive: true, withFileTypes: true })
    .filter((item) => item.isFile() && pattern.test(item.name))
    .map((item) => path.join(item.parentPath, item.name));
}

/**
 * 收集库源码里公开的函数定义。
 * @param {string} targetDir 目标目录
 * @returns {Map<string, string[]>} 函数名到定义它的源文件（相对目标目录）
 */
export function collectDefinitions(targetDir) {
  const definitions = new Map();
  const add = (name, file) => {
    const files = definitions.get(name) ?? [];
    files.push(path.relative(targetDir, file).split(path.sep).join('/'));
    definitions.set(name, files);
  };
  const files = [path.join(targetDir, 'src', 'sb2000.s'), ...listFiles(path.join(targetDir, 'src', 'lib'), /\.(c|s)$/)];
  for (const file of files) {
    const text = readFileSync(file, 'utf8');
    if (file.endsWith('.c')) {
      for (const match of text.matchAll(/^(?!static\b)[\w\s*]*?\b(sb_\w+)\s*\([^)]*\)\s*\{/gm)) add(match[1], file);
    } else {
      for (const match of text.matchAll(/^\s*\.export\s+(.+)$/gm)) {
        for (const symbol of match[1].split(',').map((item) => item.trim())) {
          if (symbol.startsWith('_sb_')) add(symbol.slice(1), file);
        }
      }
    }
  }
  return definitions;
}

/**
 * 读出全部带文档注释的函数。
 * @param {string} targetDir 目标目录
 * @returns {import('../../out/shared/c-api/api-functions.js').ApiFunction[]} 函数列表
 */
export function readFunctions(targetDir = TARGET_DIR) {
  const includeDir = path.join(targetDir, 'include');
  return readdirSync(includeDir)
    .filter((name) => name.endsWith('.h') && !SKIPPED_HEADERS.has(name))
    .flatMap((name) => parseHeaderFunctions(readFileSync(path.join(includeDir, name), 'utf8').replace(/\r\n/g, '\n'), name));
}

/**
 * 读不封装清单。
 * @param {string} targetDir 目标目录
 * @returns {{functions: Record<string, string>, partial: Record<string, string>}} 清单
 */
export function readUnwrapped(targetDir = TARGET_DIR) {
  return JSON.parse(readFileSync(path.join(targetDir, 'api-unwrapped.json'), 'utf8'));
}

/**
 * 每个已知功能号被哪些函数涉及。
 * @param {import('../../out/shared/c-api/api-functions.js').ApiFunction[]} functions 函数列表
 * @returns {Map<string, string[]>} 功能号（两位大写十六进制）到函数名
 */
export function coverageOf(functions) {
  const coverage = new Map();
  for (const number of KNOWN_FUNCTIONS) {
    coverage.set(number, functions.filter((fn) => fn.syscalls.some((tag) => syscallMatches(tag, number.toLowerCase()))).map((fn) => fn.name));
  }
  return coverage;
}

/**
 * 检查接口：文档完整、有且只有一处实现、没有多余的公开函数、系统调用覆盖有交代。
 * @param {string} [targetDir] 目标目录
 * @returns {{problems: string[], functions: import('../../out/shared/c-api/api-functions.js').ApiFunction[]}} 问题列表与全部函数
 */
export function checkSb2000Api(targetDir = TARGET_DIR) {
  const problems = [];
  const functions = readFunctions(targetDir);
  const definitions = collectDefinitions(targetDir);

  const seen = new Set();
  for (const fn of functions) {
    if (seen.has(fn.name)) problems.push(`${fn.name}：在头文件里声明了多次`);
    seen.add(fn.name);
    if (fn.summary === '') problems.push(`${fn.name}（${fn.header}）：缺少说明`);
    for (const param of fn.params) {
      if (!fn.paramDocs.includes(param)) problems.push(`${fn.name}（${fn.header}）：缺少 @param ${param}`);
    }
    for (const documented of fn.paramDocs) {
      if (!fn.params.includes(documented)) problems.push(`${fn.name}（${fn.header}）：@param ${documented} 不是函数的参数`);
    }
    if (fn.returnType !== 'void' && !fn.hasReturnDoc) problems.push(`${fn.name}（${fn.header}）：缺少 @return`);
    const files = definitions.get(fn.name) ?? [];
    if (files.length === 0) problems.push(`${fn.name}（${fn.header}）：没有实现`);
    if (files.length > 1) problems.push(`${fn.name}：有多处实现：${files.join('、')}`);
  }
  for (const [name, files] of definitions) {
    if (!seen.has(name)) problems.push(`${name}（${files.join('、')}）：库里有公开函数，头文件里没有声明与文档`);
  }

  const unwrapped = readUnwrapped(targetDir);
  const known = new Set(KNOWN_FUNCTIONS);
  for (const [number, wrappers] of coverageOf(functions)) {
    const reason = unwrapped.functions[`$${number}`];
    if (wrappers.length === 0 && !reason) problems.push(`功能 $${number}：没有函数封装它，api-unwrapped.json 里也没有写不封装的原因`);
    if (wrappers.length > 0 && reason) problems.push(`功能 $${number}：已被 ${wrappers[0]} 等函数封装，却还写在 api-unwrapped.json 的 functions 里`);
  }
  for (const group of ['functions', 'partial']) {
    for (const key of Object.keys(unwrapped[group])) {
      if (!known.has(key.slice(1).toUpperCase())) problems.push(`api-unwrapped.json 的 ${group} 里有未知的功能号 ${key}`);
    }
  }
  for (const key of Object.keys(unwrapped.partial)) {
    if ((coverageOf(functions).get(key.slice(1).toUpperCase()) ?? []).length === 0) problems.push(`api-unwrapped.json 的 partial 里的 ${key} 没有任何函数封装，应写在 functions 里`);
  }
  return { problems, functions };
}

if (process.argv[1] && path.resolve(process.argv[1]) === fileURLToPath(import.meta.url)) {
  const { problems, functions } = checkSb2000Api();
  if (process.argv.includes('--list')) {
    for (const fn of functions) console.log(`${fn.name}\t${fn.header}\t${fn.syscalls.join(' ')}`);
  }
  if (problems.length > 0) {
    console.error(problems.join('\n'));
    process.exit(1);
  }
  console.log(`SB2000 接口检查通过：${functions.length} 个函数，${KNOWN_FUNCTIONS.length} 个功能号都有交代`);
}
