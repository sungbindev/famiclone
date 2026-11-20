// ------------------------------------------------------------------------
// 名称：check-structure.mjs
// 说明：检查主包 src 下的分层依赖：每个功能目录只能引用允许的目录，并且只经对方的 index.mjs 引用；反汇编核心里除 formats/ 外不出现机型名与文件格式名。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-10
// 备注：用法 node tools/check-structure.mjs；依赖表与 docs/01-软件架构设计.md 第 5 节一致，改表时同步改文档。
// ------------------------------------------------------------------------
import { readdirSync, readFileSync, statSync } from 'node:fs';
import path from 'node:path';
import { fileURLToPath } from 'node:url';

/** 各目录允许引用的其他目录（不含自身） */
export const ALLOWED_DEPENDENCIES = {
  shared: [],
  api: [],
  docs: ['shared'],
  config: ['shared'],
  targets: ['shared'],
  toolchain: ['shared'],
  packagers: ['shared'],
  emulator: ['shared'],
  disasm: ['shared', 'toolchain'],
  build: ['shared', 'config', 'targets', 'toolchain', 'packagers'],
  cli: ['shared', 'config', 'targets', 'toolchain', 'build', 'emulator', 'disasm', 'api', 'docs'],
};

/** 匹配 import 与 export ... from 的相对路径 */
const IMPORT_PATTERN = /(?:import|export)\s[^'"]*?from\s*['"](\.[^'"]+)['"]|import\s*\(\s*['"](\.[^'"]+)['"]\s*\)|import\s*['"](\.[^'"]+)['"]/g;

/**
 * 列出目录下的全部 .mjs 文件。
 * @param {string} dir 目录
 * @returns {string[]} 文件的绝对路径
 */
function listModuleFiles(dir) {
  const files = [];
  for (const name of readdirSync(dir)) {
    const full = path.join(dir, name);
    if (statSync(full).isDirectory()) files.push(...listModuleFiles(full));
    else if (name.endsWith('.mjs')) files.push(full);
  }
  return files;
}

/**
 * 检查 src 目录下的依赖是否符合分层表。
 * @param {string} srcRoot 主包的 src 目录
 * @returns {string[]} 违规说明，没有违规时为空数组
 */
export function checkImports(srcRoot) {
  const violations = [];
  for (const moduleName of readdirSync(srcRoot)) {
    const moduleDir = path.join(srcRoot, moduleName);
    if (!statSync(moduleDir).isDirectory()) continue;
    const allowed = ALLOWED_DEPENDENCIES[moduleName];
    if (!allowed) {
      violations.push(`${moduleName}：目录没有在依赖表中登记`);
      continue;
    }
    for (const file of listModuleFiles(moduleDir)) {
      const text = readFileSync(file, 'utf8');
      for (const match of text.matchAll(IMPORT_PATTERN)) {
        const specifier = match[1] ?? match[2] ?? match[3];
        const target = path.resolve(path.dirname(file), specifier);
        const relative = path.relative(srcRoot, target).split(path.sep);
        if (relative[0] === '..') continue;
        const targetModule = relative[0];
        if (targetModule === moduleName) continue;
        const shownFile = path.relative(srcRoot, file).split(path.sep).join('/');
        if (!allowed.includes(targetModule)) {
          violations.push(`${shownFile}：不允许引用 ${targetModule}（${specifier}）`);
        } else if (relative.length !== 2 || relative[1] !== 'index.mjs') {
          violations.push(`${shownFile}：必须经 ${targetModule}/index.mjs 引用（${specifier}）`);
        }
      }
    }
  }
  return violations;
}

/** 反汇编核心里不允许出现的机型名与映像格式名 */
const MACHINE_NAMES = /\b(?:i?nes|famicom|sb-?2000|sb-?dos)\b/i;

/**
 * 检查反汇编核心没有写死机型：除 formats/ 外，源码（含注释）里不出现机型名与文件格式名。
 * @param {string} srcRoot 主包的 src 目录
 * @returns {string[]} 违规说明，没有违规或没有 disasm 目录时为空数组
 */
export function checkDisasmNames(srcRoot) {
  const disasmDir = path.join(srcRoot, 'disasm');
  try {
    if (!statSync(disasmDir).isDirectory()) return [];
  } catch {
    return [];
  }
  const violations = [];
  for (const file of listModuleFiles(disasmDir)) {
    const relative = path.relative(disasmDir, file).split(path.sep);
    if (relative[0] === 'formats') continue;
    readFileSync(file, 'utf8').split('\n').forEach((line, index) => {
      const match = MACHINE_NAMES.exec(line);
      if (match) violations.push(`disasm/${relative.join('/')}：第 ${index + 1} 行出现机型或格式名 ${match[0]}（只允许在 formats/ 里）`);
    });
  }
  return violations;
}

if (process.argv[1] && path.resolve(process.argv[1]) === fileURLToPath(import.meta.url)) {
  const root = path.resolve(path.dirname(fileURLToPath(import.meta.url)), '..', 'packages', 'fcdev', 'src');
  const violations = [...checkImports(root), ...checkDisasmNames(root)];
  if (violations.length > 0) {
    console.error(violations.join('\n'));
    process.exit(1);
  }
  console.log('分层依赖检查通过');
}
