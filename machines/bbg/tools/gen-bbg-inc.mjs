// ------------------------------------------------------------------------
// 名称：gen-bbg-inc.mjs
// 说明：由 machines/bbg/src/include 下的符号定义文件与运行库源码生成开发库的汇编符号文件
//       vscode/targets/bbg/asminc/bbg.inc。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-24
// 备注：用法 node gen-bbg-inc.mjs；符号定义只改 src/include 下的文件，bbg.inc 不手工修改。运行库入口取自
//       vscode/targets/bbg/src/lib 下每个源文件里“bbg_名字:”形式的标号。renderBbgInc 供接口检查复用。
// ------------------------------------------------------------------------

import fs from 'node:fs';
import path from 'node:path';
import { fileURLToPath } from 'node:url';

const here = path.dirname(fileURLToPath(import.meta.url));

/** 符号定义文件所在目录。 */
export const INCLUDE_DIR = path.join(here, '..', 'src', 'include');

/** 开发库（FC Dev 目标）所在目录。 */
export const TARGET_DIR = path.join(here, '..', '..', '..', 'vscode', 'targets', 'bbg');

/** 运行库源码目录。 */
export const LIB_DIR = path.join(TARGET_DIR, 'src', 'lib');

/** 生成的符号文件。 */
export const OUT_FILE = path.join(TARGET_DIR, 'asminc', 'bbg.inc');

/** 合并进 bbg.inc 的符号定义文件，按顺序。 */
const FILES = ['system-entries.inc', 'bios-calls.inc', 'dos-calls.inc'];

/** bbg.inc 的文件头。 */
const HEADER = `; ------------------------------------------------------------------------
; 名称：bbg.inc
; 说明：步步高学生电脑 BBGDOS 程序用的符号定义：系统调用入口、BIOS 功能号、DOS 调用与文件调用的功能号、文件通道块
;       的布局、出错码，以及汇编运行库 bbg.lib 的函数与零页参数单元。
; 作者：sungbin
; 邮箱：sungbin@3578.cn
; 日期：2026-11-24
; 备注：本文件由 machines/bbg/tools/gen-bbg-inc.mjs 生成，不要手工修改；符号定义在 machines/bbg/src/include。
;       调用方法：DOS 调用 A 为功能号，JSR DOS_ENTRY，C=1 出错且 A 为出错码；文件调用 X、Y 为通道块地址高、低字节，
;       JSR FILE_ENTRY，A 为出错码，C=1 出错；BIOS 调用 A 为功能号，JSR BIOS_ENTRY。各调用不保留寄存器。
; ------------------------------------------------------------------------
`;

/**
 * 去掉文件头注释（以 ; 开头、到第一个分节线或非注释行为止）。
 * @param {string} text 文件内容。
 * @returns {string} 正文。
 */
function stripHeader(text) {
  const lines = text.split(/\r?\n/);
  let index = 0;
  while (index < lines.length && (lines[index].startsWith(';') || lines[index].trim() === '')) {
    if (lines[index].startsWith(';-')) break; // 分节线属于正文
    index += 1;
  }
  return lines.slice(index).join('\n').trimEnd();
}

/**
 * 收集运行库源码里的公开函数，按模块分组。
 * @returns {Map<string, string[]>} 模块名到函数名（已排序）。
 */
export function collectLibrary() {
  const modules = new Map();
  for (const module of fs.readdirSync(LIB_DIR, { withFileTypes: true })) {
    if (!module.isDirectory()) continue;
    const names = [];
    for (const file of fs.readdirSync(path.join(LIB_DIR, module.name)).sort()) {
      const text = fs.readFileSync(path.join(LIB_DIR, module.name, file), 'utf8');
      for (const match of text.matchAll(/^(bbg_[a-z0-9_]+):/gm)) names.push(match[1]);
    }
    modules.set(module.name, names.sort());
  }
  return modules;
}

/**
 * 生成 bbg.inc 的全部内容。
 * @returns {string} 文件内容。
 */
export function renderBbgInc() {
  const out = [HEADER];
  for (const file of FILES) out.push(stripHeader(fs.readFileSync(path.join(INCLUDE_DIR, file), 'utf8')), '');
  out.push(';---------------------------------------; 运行库 bbg.lib 的零页参数单元（多于 A、X、Y 的参数写在这里，库函数不保留）');
  out.push('        .globalzp       bbg_arg0, bbg_arg1', '');
  for (const [module, names] of collectLibrary()) {
    out.push(`;---------------------------------------; 运行库函数：${module}`);
    for (const name of names) out.push(`        .global         ${name}`);
    out.push('');
  }
  return out.join('\n');
}

if (process.argv[1] && path.resolve(process.argv[1]) === fileURLToPath(import.meta.url)) {
  fs.mkdirSync(path.dirname(OUT_FILE), { recursive: true });
  fs.writeFileSync(OUT_FILE, renderBbgInc(), 'utf8');
  console.log(`已写出 ${OUT_FILE}`);
}
