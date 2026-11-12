#!/usr/bin/env node
// ------------------------------------------------------------------------
// 名称：create-fcdev.mjs
// 说明：npm create fcdev 的命令入口：把终端环境交给创建流程，并以其返回值作为退出码。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-10
// 备注：只经 src/index.mjs 进入。
// ------------------------------------------------------------------------
import { runCommandInTerminal, runCreate } from '../src/index.mjs';

process.exitCode = await runCreate(process.argv.slice(2), {
  cwd: process.cwd(),
  stdin: process.stdin,
  stdout: process.stdout,
  stderr: process.stderr,
  interactive: Boolean(process.stdin.isTTY && process.stdout.isTTY),
  runCommand: runCommandInTerminal,
});
