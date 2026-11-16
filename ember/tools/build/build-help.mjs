/*
 * ------------------------------------------------------------------------
 * 名称：build-help.mjs
 * 说明：构建 Ember 的帮助文档站点：先由调试 SDK 把 OpenRPC 接口文档生成成 help/api 下的各模块页，再用 VitePress 把 help/ 构建成静态站点，输出到 build/ember/help。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-17
 * 备注：要先运行过 npm run schema（调试 SDK 的 generated/openrpc.json 由它生成）；调试 SDK 的生成脚本依赖仓库根目录 npm install 装好的 esbuild；
 *       npm start、npm run package* 与 npm run make:win 会先运行本脚本；站点随安装包分发，开发版从 build/ember/help 读取。
 * ------------------------------------------------------------------------
 */

import { execFileSync } from 'node:child_process';
import { existsSync } from 'node:fs';
import { dirname, join, resolve } from 'node:path';
import { fileURLToPath } from 'node:url';
import { build } from 'vitepress';
import { HELP_BUILD_DIRECTORY } from './build-paths.mjs';

const root = resolve(dirname(fileURLToPath(import.meta.url)), '..', '..');
const sdk = join(root, '..', 'packages', 'ember-debug-sdk');
const helpRoot = join(root, 'help');

/** 用户可见的文字。 */
const TEXT = {
  noSdk: `找不到调试 SDK：${sdk}`,
  noOpenRpc: '调试 SDK 里没有 generated/openrpc.json：先运行 npm run schema',
  noEsbuild: '调试 SDK 的生成脚本需要 esbuild：先在仓库根目录运行 npm install'
};

try {
  if (!existsSync(sdk)) throw new Error(TEXT.noSdk);
  if (!existsSync(join(sdk, 'generated', 'openrpc.json'))) throw new Error(TEXT.noOpenRpc);
  if (!existsSync(join(root, '..', 'node_modules', 'esbuild')) && !existsSync(join(sdk, 'node_modules', 'esbuild'))) throw new Error(TEXT.noEsbuild);

  // 调试接口的各模块页：由接口定义生成，写到 help/api（不提交）。
  execFileSync(process.execPath, [join(sdk, 'scripts', 'docs.mjs')], { cwd: sdk, stdio: 'inherit' });
  await build(helpRoot);
  console.log(`帮助文档：已生成 ${HELP_BUILD_DIRECTORY}`);
} catch (error) {
  console.error(error instanceof Error ? error.message : String(error));
  process.exit(1);
}
