// ------------------------------------------------------------------------
// 名称：gen-symbols-doc.mjs
// 说明：由符号定义文件（src/include 下的 bios-calls.inc、dos-calls.inc、system-entries.inc）生成接口文档
//       docs/api/symbols-reference.md；说明只取自符号行尾的注释。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-24
// 备注：用法 node gen-symbols-doc.mjs；分节取自“;----…; 标题”形式的分节注释，文件头注释里的“说明”写在每个文件的开头。
// ------------------------------------------------------------------------

import fs from 'node:fs';
import path from 'node:path';
import { fileURLToPath } from 'node:url';

const here = path.dirname(fileURLToPath(import.meta.url));
const includeDir = path.join(here, '..', 'src', 'include');
const outFile = path.join(here, '..', 'docs', 'api', 'symbols-reference.md');
const FILES = ['system-entries.inc', 'bios-calls.inc', 'dos-calls.inc'];

/** 把一个 .inc 文件拆成文件说明与按分节归组的符号。 */
function parseInclude(text) {
  const lines = text.split(/\r?\n/);
  const header = [];
  const sections = [];
  let current = { title: '符号', items: [] };
  sections.push(current);
  let inHeader = true;
  for (const line of lines) {
    if (inHeader) {
      if (line.startsWith(';')) {
        if (!/^; -{10,}/.test(line)) header.push(line.replace(/^;\s?/, ''));
        continue;
      }
      inHeader = false;
    }
    const section = /^;-{5,};?\s*(.*)$/.exec(line);
    if (section) {
      current = { title: section[1].trim() || '符号', items: [] };
      sections.push(current);
      continue;
    }
    const symbol = /^([A-Za-z_][A-Za-z0-9_]*)\s*=\s*([^;\s]+)\s*(?:;\s*(.*))?$/.exec(line);
    if (symbol) current.items.push({ name: symbol[1], value: symbol[2], note: (symbol[3] ?? '').trim() });
  }
  return { header, sections: sections.filter((s) => s.items.length > 0) };
}

const out = ['# 步步高系统符号参考', '', '本文由 `tools/gen-symbols-doc.mjs` 从 `src/include` 下的符号定义文件生成，不手工修改；改符号文件后重新运行。', ''];
for (const file of FILES) {
  const { header, sections } = parseInclude(fs.readFileSync(path.join(includeDir, file), 'utf8'));
  out.push(`## ${file}`, '');
  const description = header.find((l) => l.startsWith('说明：'));
  if (description) out.push(description.replace(/^说明：/, ''), '');
  for (const section of sections) {
    out.push(`### ${section.title}`, '', '| 符号 | 值 | 说明 |', '| --- | --- | --- |');
    for (const item of section.items) out.push(`| \`${item.name}\` | \`${item.value}\` | ${item.note.replace(/\|/g, '\\|')} |`);
    out.push('');
  }
}
fs.mkdirSync(path.dirname(outFile), { recursive: true });
fs.writeFileSync(outFile, out.join('\n'));
console.log(`已写出 ${outFile}`);
