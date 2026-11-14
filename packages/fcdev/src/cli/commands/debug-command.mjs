// ------------------------------------------------------------------------
// 名称：debug-command.mjs
// 说明：debug 命令：把参数原样交给调试 SDK 的终端客户端 ember-debug，由它连接 Ember 的调试服务。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-10
// 备注：fcdev 不解析调试命令；端口取环境变量 FCDEV_DEBUG_PORT，默认 5699，找到 run --debug 启动的 Ember；
//       ember-debug 的退出码原样作为 fcdev 的退出码，输出边读边转，watch 这类长时间运行的命令也能看到实时结果。
// ------------------------------------------------------------------------
import { spawn } from 'node:child_process';
import { debugEnvironment, resolveDebugCli } from '../../emulator/index.mjs';
import { FcdevError } from '../../shared/index.mjs';

/**
 * 执行 debug 命令。
 * @param {{positionals: string[], context: import('../index.mjs').CliContext}} input ember-debug 的各个参数与运行环境
 * @returns {Promise<number>} ember-debug 的退出码
 * @throws {FcdevError} 没有给命令，或终端客户端不可用
 */
export async function runDebugCommand({ positionals, context }) {
  if (positionals.length === 0) {
    throw new FcdevError('debug 需要一条调试命令', { code: 'ARGUMENT_INVALID', hint: '例如：npx fcdev debug call runtime.status、npx fcdev debug screenshot out.png；npx fcdev debug --help 查看全部命令' });
  }
  const cli = resolveDebugCli();
  const env = debugEnvironment(context.env);
  return new Promise((resolve, reject) => {
    const child = spawn(process.execPath, [cli, ...positionals], { cwd: context.cwd, env, stdio: ['ignore', 'pipe', 'pipe'] });
    child.stdout.setEncoding('utf8');
    child.stderr.setEncoding('utf8');
    child.stdout.on('data', (chunk) => context.stdout.write(chunk));
    child.stderr.on('data', (chunk) => context.stderr.write(chunk));
    child.once('error', (error) => reject(new FcdevError('无法运行调试 SDK 的终端客户端', { code: 'DEBUG_CLI_START_FAILED', cause: error })));
    child.once('close', (code) => resolve(code ?? 1));
  });
}
