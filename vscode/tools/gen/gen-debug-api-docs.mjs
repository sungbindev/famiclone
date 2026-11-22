/*
 * ------------------------------------------------------------------------
 * 名称：gen-debug-api-docs.mjs
 * 说明：生成调试协议的 Markdown 参考：按接口描述文档（debug/openrpc.json）写出总览页与每个模块一页到给定的输出目录，并删除不再存在的模块页。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-22
 * 备注：用法：node vscode/tools/gen/gen-debug-api-docs.mjs <输出目录>，输出目录一般是 Ember 帮助文档的 help/api（Ember 的 npm run help:build 调用，生成的页面不提交）；
 *       生成器是扩展的编译产物 out/shared/debug-client/api-reference.js，要先运行 npm run build；内容没有变化的页面不改写。
 * ------------------------------------------------------------------------
 */

import { existsSync, mkdirSync, readdirSync, readFileSync, rmSync, writeFileSync } from 'node:fs';
import { createRequire } from 'node:module';
import { dirname, join, resolve } from 'node:path';
import { fileURLToPath } from 'node:url';

const require = createRequire(import.meta.url);
const extension = resolve(dirname(fileURLToPath(import.meta.url)), '..', '..');

try {
  const output = process.argv[2];
  if (output === undefined) throw new Error('用法：node vscode/tools/gen/gen-debug-api-docs.mjs <输出目录>');
  const generator = join(extension, 'out', 'shared', 'debug-client', 'api-reference.js');
  if (!existsSync(generator)) throw new Error('找不到扩展的编译产物，先运行 npm run build');
  const { generateApiReference } = require(generator);
  const document = JSON.parse(readFileSync(join(extension, 'debug', 'openrpc.json'), 'utf8'));
  const files = generateApiReference(document);

  mkdirSync(output, { recursive: true });
  const keep = new Set(files.map((file) => file.path));
  for (const name of readdirSync(output)) if (name.endsWith('.md') && !keep.has(name)) rmSync(join(output, name));
  let written = 0;
  for (const file of files) {
    const target = join(output, file.path);
    if (existsSync(target) && readFileSync(target, 'utf8') === file.content) continue;
    writeFileSync(target, file.content, 'utf8');
    written += 1;
  }
  console.log(`协议参考：${files.length} 页，写入 ${written} 页：${output}`);
} catch (error) {
  console.error(error instanceof Error ? error.message : String(error));
  process.exit(1);
}
