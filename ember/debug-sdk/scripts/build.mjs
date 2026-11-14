/*
 * ------------------------------------------------------------------------
 * 名称：build.mjs
 * 说明：SDK 的构建：用 esbuild 打包库入口、Node 入口、终端客户端与 MCP 服务，用 TypeScript 6 生成声明文件。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-14
 * 备注：generated/ 由 Ember 的 npm run schema 生成，不提交，这里不生成；声明文件用 SDK 自带的 TypeScript 6.0.3 生成。
 * ------------------------------------------------------------------------
 */

import { execFileSync } from 'node:child_process';
import { rmSync } from 'node:fs';
import { createRequire } from 'node:module';
import { dirname, join } from 'node:path';
import { fileURLToPath } from 'node:url';

import { build } from 'esbuild';

const sdk = join(dirname(fileURLToPath(import.meta.url)), '..');
const require = createRequire(join(sdk, 'package.json'));

rmSync(join(sdk, 'dist'), { recursive: true, force: true });

const common = { bundle: true, format: 'esm', target: 'es2023', charset: 'utf8', logLevel: 'warning', absWorkingDir: sdk };
await build({ ...common, entryPoints: ['src/index.ts'], outfile: 'dist/index.js', platform: 'neutral' });
await build({ ...common, entryPoints: ['src/node.ts'], outfile: 'dist/node.js', platform: 'node' });
const executable = { ...common, platform: 'node', banner: { js: '#!/usr/bin/env node' } };
await build({ ...executable, entryPoints: ['src/cli/index.ts'], outfile: 'dist/cli.js' });
await build({ ...executable, entryPoints: ['src/mcp/index.ts'], outfile: 'dist/mcp.js' });

const tsc = join(dirname(require.resolve('typescript/package.json')), 'bin', 'tsc');
execFileSync(process.execPath, [tsc, '-p', join(sdk, 'tsconfig.build.json')], { stdio: 'inherit' });
console.log('SDK 构建完成：debug-sdk/dist');
