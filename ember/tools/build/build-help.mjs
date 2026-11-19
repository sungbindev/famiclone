/*
 * ------------------------------------------------------------------------
 * 名称：build-help.mjs
 * 说明：构建 Ember 的帮助文档站点：先由调试 SDK 把 OpenRPC 接口文档生成成 help/api 下的各模块页，再把 fcdev 的使用文档导入 help/fcdev，最后用 VitePress 把 help/ 构建成静态站点，输出到 build/ember/help。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-18
 * 备注：要先运行过 npm run schema（调试 SDK 的 generated/openrpc.json 由它生成）；调试 SDK 的生成脚本依赖仓库根目录 npm install 装好的 esbuild；
 *       fcdev 的 C 接口参考、符号参考与示例程序源码由仓库根目录 tools 下的生成脚本先重新生成（没有变化时文件不变），再随手册一起导入，help/fcdev 不提交；
 *       npm run package:win 与 npm run check 会先运行本脚本；站点由 resources:pack 打进资源包，嵌进可执行文件。
 * ------------------------------------------------------------------------
 */

import { execFileSync } from 'node:child_process';
import { existsSync } from 'node:fs';
import { dirname, join, resolve } from 'node:path';
import { fileURLToPath } from 'node:url';
import { build } from 'vitepress';
import { HELP_BUILD_DIRECTORY } from './build-paths.mjs';
import { importFcdevDocs } from './import-fcdev-docs.mjs';

const root = resolve(dirname(fileURLToPath(import.meta.url)), '..', '..');
const repoRoot = join(root, '..');
const sdk = join(root, '..', 'packages', 'ember-debug-sdk');
const helpRoot = join(root, 'help');

/** 先重新生成的 fcdev 文档：C 接口参考、符号参考与示例程序源码。 */
const FCDEV_GENERATORS = ['gen-sb2000-api-doc.mjs', 'gen-sb2000-symbols-doc.mjs', 'gen-sb2000-examples-doc.mjs'];

/** 用户可见的文字。 */
const TEXT = {
  noSdk: `找不到调试 SDK：${sdk}`,
  noOpenRpc: '调试 SDK 里没有 generated/openrpc.json：先运行 npm run schema',
  noEsbuild: '调试 SDK 的生成脚本需要 esbuild：先在仓库根目录运行 npm install',
  noGenerator: (script) => `找不到 fcdev 文档的生成脚本：${join(repoRoot, 'tools', script)}`
};

try {
  if (!existsSync(sdk)) throw new Error(TEXT.noSdk);
  if (!existsSync(join(sdk, 'generated', 'openrpc.json'))) throw new Error(TEXT.noOpenRpc);
  if (!existsSync(join(root, '..', 'node_modules', 'esbuild')) && !existsSync(join(sdk, 'node_modules', 'esbuild'))) throw new Error(TEXT.noEsbuild);

  // 调试接口的各模块页：由接口定义生成，写到 help/api（不提交）。
  execFileSync(process.execPath, [join(sdk, 'scripts', 'docs.mjs')], { cwd: sdk, stdio: 'inherit' });

  // fcdev 的手册：参考文档先重新生成，再连同手册一起导入 help/fcdev（不提交）。
  for (const script of FCDEV_GENERATORS) {
    if (!existsSync(join(repoRoot, 'tools', script))) throw new Error(TEXT.noGenerator(script));
    execFileSync(process.execPath, [join(repoRoot, 'tools', script)], { cwd: repoRoot, stdio: 'inherit' });
  }
  const { pages, written } = importFcdevDocs();
  console.log(`fcdev 手册：${pages} 页，写入 ${written} 页：ember/help/fcdev`);
  await build(helpRoot);
  console.log(`帮助文档：已生成 ${HELP_BUILD_DIRECTORY}`);
} catch (error) {
  console.error(error instanceof Error ? error.message : String(error));
  process.exit(1);
}
