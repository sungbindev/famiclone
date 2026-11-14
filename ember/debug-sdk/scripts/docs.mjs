/*
 * ------------------------------------------------------------------------
 * 名称：docs.mjs
 * 说明：生成协议的 Markdown 参考到 Ember 的 docs/api：先打包文档生成器（不属于公开接口），再按 OpenRPC 文档写出各页，并删除不再存在的模块页。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-14
 * 备注：TypeDoc 生成的 SDK 接口参考（debug-sdk/docs/api，不提交）由 package.json 的 docs 脚本接着生成。
 * ------------------------------------------------------------------------
 */

import { existsSync, mkdirSync, mkdtempSync, readdirSync, readFileSync, rmSync, writeFileSync } from 'node:fs';
import { tmpdir } from 'node:os';
import { dirname, join } from 'node:path';
import { fileURLToPath, pathToFileURL } from 'node:url';

import { build } from 'esbuild';

const sdk = join(dirname(fileURLToPath(import.meta.url)), '..');
const output = join(sdk, '..', 'docs', 'api');
const work = mkdtempSync(join(tmpdir(), 'ember-debug-docs-'));

try {
  const bundle = join(work, 'api-reference.mjs');
  await build({ bundle: true, format: 'esm', platform: 'node', target: 'es2023', charset: 'utf8', logLevel: 'warning', absWorkingDir: sdk, entryPoints: ['src/logic/api-reference.ts'], outfile: bundle });
  const { generateApiReference } = await import(pathToFileURL(bundle).href);
  const document = JSON.parse(readFileSync(join(sdk, 'generated', 'openrpc.json'), 'utf8'));
  const files = generateApiReference(document);

  mkdirSync(output, { recursive: true });
  const keep = new Set(files.map((file) => file.path));
  for (const name of readdirSync(output)) if (name.endsWith('.md') && !keep.has(name)) rmSync(join(output, name));
  let written = 0;
  for (const file of files) {
    const path = join(output, file.path);
    if (existsSync(path) && readFileSync(path, 'utf8') === file.content) continue;
    writeFileSync(path, file.content, 'utf8');
    written += 1;
  }
  console.log(`协议参考：${files.length} 页，写入 ${written} 页：docs/api`);
} finally {
  rmSync(work, { recursive: true, force: true });
}
