// ------------------------------------------------------------------------
// 名称：gen-sb2000-examples-doc.mjs
// 说明：由 apps/sb2000/examples 下的示例项目生成示例程序源码文档 examples-source.md：每个示例一节，写明程序名并附完整源码。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-18
// 备注：用法 node tools/gen-sb2000-examples-doc.mjs；改了示例源码或 fcdev.json 后重新运行；test/sb2000-examples-doc.test.mjs 核对入库的文档与生成结果一致。
//       示例的说明只来自源码文件头的注释，这里不手写；示例的顺序由 EXAMPLE_ORDER 决定，没列出的示例按名字排在最后。
// ------------------------------------------------------------------------
import { existsSync, readdirSync, readFileSync, writeFileSync } from 'node:fs';
import path from 'node:path';
import { fileURLToPath } from 'node:url';

/** 仓库根目录 */
const REPO_ROOT = path.resolve(path.dirname(fileURLToPath(import.meta.url)), '..');

/** 示例项目所在目录 */
export const EXAMPLES_DIR = path.join(REPO_ROOT, 'apps', 'sb2000', 'examples');

/** 生成的文档路径 */
export const EXAMPLES_DOC_FILE = path.join(REPO_ROOT, 'packages', 'fcdev', 'targets', 'sb2000', 'examples-source.md');

/** 文档里示例的顺序：由简到繁 */
const EXAMPLE_ORDER = ['hello', 'say', 'show', 'copy', 'peek', 'poke', 'pad', 'mouse', 'play', 'newcall', 'syscall'];

/** 源文件扩展名对应的代码块语言（汇编没有合适的高亮语法，用纯文本） */
const FENCE_LANGUAGES = { '.c': 'c', '.s': 'text', '.asm': 'text' };

/**
 * 读出一个示例：程序名与源文件。
 * @param {string} examplesDir 示例目录
 * @param {string} name 示例的目录名
 * @returns {{name: string, programName: string, files: {relative: string, language: string, text: string}[]}} 示例
 */
function readExample(examplesDir, name) {
  const projectDir = path.join(examplesDir, name);
  const config = JSON.parse(readFileSync(path.join(projectDir, 'fcdev.json'), 'utf8'));
  const sourceDir = path.join(projectDir, 'src');
  const files = readdirSync(sourceDir)
    .filter((file) => path.extname(file) in FENCE_LANGUAGES)
    .sort()
    .map((file) => ({
      relative: `src/${file}`,
      language: FENCE_LANGUAGES[path.extname(file)],
      text: readFileSync(path.join(sourceDir, file), 'utf8').replace(/\r\n/g, '\n').replace(/\s+$/, ''),
    }));
  return { name, programName: String(config.name ?? name).toUpperCase(), files };
}

/**
 * 列出示例目录名：先按 EXAMPLE_ORDER，再按名字排没列出的。
 * @param {string} examplesDir 示例目录
 * @returns {string[]} 目录名
 */
export function listExampleNames(examplesDir = EXAMPLES_DIR) {
  const found = readdirSync(examplesDir, { withFileTypes: true })
    .filter((entry) => entry.isDirectory() && existsSync(path.join(examplesDir, entry.name, 'fcdev.json')))
    .map((entry) => entry.name);
  return [...EXAMPLE_ORDER.filter((name) => found.includes(name)), ...found.filter((name) => !EXAMPLE_ORDER.includes(name)).sort()];
}

/**
 * 生成示例程序源码文档的全文。
 * @param {string} [examplesDir] 示例目录
 * @returns {string} Markdown 文本（LF 换行）
 */
export function renderExamplesSource(examplesDir = EXAMPLES_DIR) {
  const examples = listExampleNames(examplesDir).map((name) => readExample(examplesDir, name));
  const lines = [
    '# SB2000 示例程序源码',
    '',
    '本文由示例项目的源码生成。',
    `共 ${examples.length} 个示例，每个是一个 fcdev 项目：目录里有 \`fcdev.json\`（\`target\` 是 \`sb2000\`，\`name\` 是 DOS 里的程序名）和 \`src/\` 下的一个源文件，下面给出完整源码。每个示例做什么、怎么用见 [SB2000 开发指南](../../docs/sb2000/01-开发指南.md) 第 10 节，源码开头的注释也有说明。`,
    '',
    '把源码放进新建的项目后，`npx fcdev build` 构建，`npx fcdev disk` 另外生成放有程序的软盘镜像，`npx fcdev run` 在 Ember 里运行，见 [命令行与构建](../../docs/01-命令行与构建.md) 与 [模拟器与调试](../../docs/02-模拟器与调试.md)。',
    '',
    '## 目录',
    '',
  ];
  for (const example of examples) lines.push(`- [${example.name}](#${example.name})`);
  lines.push('');
  for (const example of examples) {
    lines.push(`## ${example.name}`, '', `程序名 \`${example.programName}\`。`, '');
    for (const file of example.files) lines.push(`\`${file.relative}\`：`, '', `\`\`\`${file.language}`, file.text, '```', '');
  }
  return lines.join('\n');
}

if (process.argv[1] && path.resolve(process.argv[1]) === fileURLToPath(import.meta.url)) {
  const text = renderExamplesSource();
  writeFileSync(EXAMPLES_DOC_FILE, text);
  console.log(`已生成 ${path.relative(REPO_ROOT, EXAMPLES_DOC_FILE)}（${text.split('\n').length} 行）`);
}
