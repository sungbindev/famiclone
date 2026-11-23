// ------------------------------------------------------------------------
// 名称：gen-bbg-api-doc.mjs
// 说明：由步步高汇编运行库源码里每个函数的说明注释生成接口参考文档 vscode/targets/bbg/api-reference.md，末尾有功能号覆盖表。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-24
// 备注：用法 node vscode/tools/gen/gen-bbg-api-doc.mjs；说明只来自库源码里标号前的“说明、入口、出口”注释与文件头的“备注”，
//       不手写；改了源码注释或 api-unwrapped.json 后重新生成，测试会核对入库的文档与生成结果一致。
// ------------------------------------------------------------------------

import { writeFileSync } from 'node:fs';
import path from 'node:path';
import { fileURLToPath } from 'node:url';
import { TARGET_DIR } from '../../../machines/bbg/tools/gen-bbg-inc.mjs';
import { coverageOf, readFunctions, readKnownFunctions, readUnwrapped } from '../check/check-bbg-api.mjs';

/** 生成的文档。 */
export const API_DOC_FILE = path.join(TARGET_DIR, 'api-reference.md');

/** 各模块的标题。 */
const MODULE_TITLES = {
  bios: 'BIOS 调用（BIOS_ENTRY，$5800）',
  dos: 'DOS 调用（DOS_ENTRY，$580C）',
  file: '文件调用（FILE_ENTRY，$5803）',
  text: '文字输出'
};

/** 库函数默认的备注，与它相同时文档里不重复。 */
const DEFAULT_NOTE = '参数与返回值见下方函数说明；不保留 A、X、Y，不使用库以外的零页单元。';

/** 调用约定。 */
const CONVENTION = `## 调用约定

- 汇编程序用 \`.include "bbg.inc"\` 引入符号，\`JSR\` 调用库函数；库放在 \`bbg.lib\`，链接时只带入用到的函数。
- 一个字节的参数放 A；一个 16 位参数（地址、数值）低字节放 A、高字节放 X；再多的参数写进零页单元 \`bbg_arg0\`、\`bbg_arg1\`（各 2 字节，低字节在前）；盘号等第二个字节参数放 Y。
- 返回值：一个字节在 A；16 位在 A（低）、X（高）；更多的返回值在 \`bbg_arg0\`、\`bbg_arg1\`。DOS 与文件调用的封装保持系统的约定：C=0 成功，C=1 出错且 A 为出错码（文件调用成功时 A=0）。
- 不保留 A、X、Y；零页只使用链接配置划给程序的 $0020-$007F 里的 \`bbg_arg0\`、\`bbg_arg1\` 与库内部几个单元（\`bbgi_\` 开头）。
- 文件调用的参数在 48 字节（\`FH_SIZE\`）的文件通道块里，由调用者提供；\`bbg_file_clear\` 清零并设置缓冲区页，\`bbg_file_set_*\` 与 \`bbg_file_get_*\` 读写其中的字段，其余字段可直接按 \`FH_\` 偏移读写。
`;

/**
 * 取标号前的注释字段。
 * @param {string} source 源文件内容。
 * @param {string} name 函数名。
 * @param {string} label 字段名（说明、入口、出口）。
 * @returns {string} 字段内容。
 */
function field(source, name, label) {
  const match = new RegExp(`^; ${name}\\n(?:;   .+\\n)*?;   ${label}：(.+)\\n`, 'm').exec(source);
  return match?.[1] ?? '';
}

/**
 * 取文件头的备注。
 * @param {string} source 源文件内容。
 * @returns {string} 备注。
 */
function noteOf(source) {
  return /^; 备注：(.+)$/m.exec(source)?.[1] ?? '';
}

/**
 * 生成接口参考文档。
 * @returns {string} Markdown 文本。
 */
export function renderApiReference() {
  const functions = readFunctions();
  const lines = [
    '# 步步高汇编运行库接口参考',
    '',
    '本文由 `vscode/tools/gen/gen-bbg-api-doc.mjs` 从运行库源码（`src/lib`）里每个函数的说明注释生成，不手工修改；改了注释或 `api-unwrapped.json` 后重新生成。符号与功能号见 `asminc/bbg.inc`（由 `machines/bbg/tools/gen-bbg-inc.mjs` 生成）。',
    '',
    CONVENTION
  ];
  for (const [module, title] of Object.entries(MODULE_TITLES)) {
    const inModule = functions.filter((fn) => fn.module === module).sort((a, b) => a.name.localeCompare(b.name));
    if (inModule.length === 0) continue;
    lines.push(`## ${title}`, '');
    for (const fn of inModule) {
      lines.push(`### ${fn.name}`, '', field(fn.source, fn.name, '说明'), '');
      lines.push(`- 入口：${field(fn.source, fn.name, '入口')}`, `- 出口：${field(fn.source, fn.name, '出口')}`);
      const note = noteOf(fn.source);
      if (note !== '' && note !== DEFAULT_NOTE) lines.push(`- 备注：${note}`);
      lines.push('');
    }
  }

  const known = readKnownFunctions();
  const coverage = coverageOf(functions);
  const unwrapped = readUnwrapped();
  lines.push('## 功能号覆盖表', '', '每个功能号被哪些库函数封装；没有封装的写明原因。BIOS 只列 bbg1 的 BIOS 1.0 实现的功能号。', '');
  for (const [kind, title] of [['dos', 'DOS 调用'], ['file', '文件调用'], ['bios', 'BIOS 调用']]) {
    lines.push(`### ${title}`, '', '| 功能号 | 符号 | 封装 |', '| --- | --- | --- |');
    const byNumber = new Map();
    for (const [name, number] of known[kind]) byNumber.set(number, [...(byNumber.get(number) ?? []), name]);
    for (const number of [...byNumber.keys()].sort((a, b) => a - b)) {
      const users = coverage[kind].get(number) ?? [];
      const cell = users.length > 0 ? users.map((name) => `\`${name}\``).join('、') : `未封装：${unwrapped[kind]?.[String(number)] ?? ''}`;
      lines.push(`| ${number} | ${(byNumber.get(number) ?? []).map((name) => `\`${name}\``).join('、')} | ${cell.replace(/\|/g, '\\|')} |`);
    }
    lines.push('');
  }
  return `${lines.join('\n').trimEnd()}\n`;
}

if (process.argv[1] && path.resolve(process.argv[1]) === fileURLToPath(import.meta.url)) {
  writeFileSync(API_DOC_FILE, renderApiReference(), 'utf8');
  console.log(`已写出 ${API_DOC_FILE}`);
}
