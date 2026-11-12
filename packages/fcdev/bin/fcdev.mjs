#!/usr/bin/env node
// ------------------------------------------------------------------------
// 名称：fcdev.mjs
// 说明：fcdev 命令入口：把终端环境交给 cli，并以其返回值作为退出码。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-10
// 备注：只经 src/cli/index.mjs 进入，不直接引用其他目录。
// ------------------------------------------------------------------------
import { runCli } from '../src/cli/index.mjs';

process.exitCode = await runCli(process.argv.slice(2), {
  cwd: process.cwd(),
  env: process.env,
  stdout: process.stdout,
  stderr: process.stderr,
});
