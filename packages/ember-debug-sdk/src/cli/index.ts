/*
 * ------------------------------------------------------------------------
 * 名称：index.ts
 * 说明：终端客户端 ember-debug 的入口：组装真实的连接、输出与文件环境，运行命令并以对应的退出码结束。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-14
 * ------------------------------------------------------------------------
 */

import { writeFileSync } from 'node:fs';

import packageInfo from '../../package.json' with { type: 'json' };
import { connectToEmber } from '../entry/connect-node.js';
import { OPENRPC_DOCUMENT } from '../entry/document.js';
import { readEndpoint } from '../infra/endpoint-file.js';
import { runCli } from './commands.js';

const CLIENT_NAME = 'ember-debug-cli';

const exitCode = await runCli(process.argv.slice(2), {
  document: OPENRPC_DOCUMENT,
  version: packageInfo.version,
  out: (text) => console.log(text),
  err: (text) => console.error(text),
  readEndpoint: (target) => readEndpoint(target),
  connect: (target) => connectToEmber({ ...target, clientName: CLIENT_NAME }),
  writeFile: (path, bytes) => writeFileSync(path, bytes),
  now: () => new Date()
});
process.exitCode = exitCode;
