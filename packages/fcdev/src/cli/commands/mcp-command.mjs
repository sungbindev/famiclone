// ------------------------------------------------------------------------
// 名称：mcp-command.mjs
// 说明：mcp 命令：启动调试 SDK 的 MCP 服务 ember-debug-mcp，经标准输入输出与 MCP 客户端通信，工具调用转给运行中的 Ember。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-21
// 备注：MCP 用标准输入输出传输，所以子进程直接继承终端的标准输入输出；找 Ember 的方式与 debug 命令相同（环境变量 FCDEV_DEBUG_PORT，默认 5699）。
// ------------------------------------------------------------------------
import { spawn } from 'node:child_process';
import { debugEnvironment, resolveDebugMcp } from '../../emulator/index.mjs';
import { FcdevError } from '../../shared/index.mjs';

/**
 * 执行 mcp 命令。
 * @param {{positionals: string[], context: import('../index.mjs').CliContext}} input 命令参数与运行环境
 * @returns {Promise<number>} MCP 服务的退出码
 * @throws {FcdevError} MCP 服务不可用
 */
export async function runMcpCommand({ positionals, context }) {
  const server = resolveDebugMcp();
  const env = debugEnvironment(context.env);
  return new Promise((resolve, reject) => {
    const child = spawn(process.execPath, [server, ...positionals], { cwd: context.cwd, env, stdio: 'inherit' });
    child.once('error', (error) => reject(new FcdevError('无法运行调试 SDK 的 MCP 服务', { code: 'DEBUG_MCP_START_FAILED', cause: error })));
    child.once('close', (code) => resolve(code ?? 1));
  });
}
