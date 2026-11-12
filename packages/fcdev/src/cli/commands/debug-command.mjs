// ------------------------------------------------------------------------
// 名称：debug-command.mjs
// 说明：debug 命令：把命令原样转发给模拟器的调试服务并打印应答。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-10
// 备注：模拟器的工作目录与终端不同，命令里的相对路径（shot、disk、trace 等）先换成绝对路径；端口取环境变量 FCDEV_DEBUG_PORT，默认 5699。
// ------------------------------------------------------------------------
import path from 'node:path';
import { DEFAULT_DEBUG_PORT, DebugClient } from '../../emulator/index.mjs';
import { FcdevError } from '../../shared/index.mjs';

/** 耗时长的命令使用的应答超时，单位毫秒 */
const LONG_COMMAND_TIMEOUT_MS = 10 * 60 * 1000;

/** 耗时长的命令 */
const LONG_COMMANDS = new Set(['run', 'step']);

/** 命令里路径参数的位置（命令名之后的第几个词，从 1 起）；disk 的路径可含空格，取剩余全部 */
const PATH_POSITIONS = {
  shot: 1,
  disk: 1,
  trace: 1,
  io: 1,
  disklog: 1,
  calllog: 1,
  savestate: 1,
  loadstate: 1,
};

/**
 * 把命令里的相对路径换成绝对路径。
 * @param {string[]} tokens 命令的各个词
 * @param {string} cwd 终端当前目录
 * @returns {string[]} 处理后的词
 */
export function resolveCommandPaths(tokens, cwd) {
  const [name, ...rest] = tokens;
  if (name === 'cdl' && rest[0] === 'save' && rest[1]) {
    return [name, rest[0], path.resolve(cwd, rest[1]), ...rest.slice(2)];
  }
  const position = PATH_POSITIONS[name];
  if (position === undefined || rest.length < position || rest[position - 1] === 'off') return tokens;
  if (name === 'disk') return [name, path.resolve(cwd, rest.join(' '))];
  const resolved = [...rest];
  resolved[position - 1] = path.resolve(cwd, rest[position - 1]);
  return [name, ...resolved];
}

/**
 * 执行 debug 命令。
 * @param {{positionals: string[], context: import('../index.mjs').CliContext}} input 调试命令的各个词与运行环境
 * @returns {Promise<number>} 应答为 OK 时 0，否则 1
 * @throws {FcdevError} 没有给命令，或无法连接调试服务
 */
export async function runDebugCommand({ positionals, context }) {
  if (positionals.length === 0) {
    throw new FcdevError('debug 需要一条调试命令', { code: 'ARGUMENT_INVALID', hint: '例如：fcdev debug status、fcdev debug run 60、fcdev debug shot out.png' });
  }
  const port = Number(context.env.FCDEV_DEBUG_PORT) || DEFAULT_DEBUG_PORT;
  const tokens = resolveCommandPaths(positionals, context.cwd);
  const client = await DebugClient.connect({ port });
  try {
    const reply = await client.send(tokens.join(' '), { timeoutMs: LONG_COMMANDS.has(tokens[0]) ? LONG_COMMAND_TIMEOUT_MS : undefined });
    context.stdout.write(`${[reply.head, ...reply.lines].join('\n')}\n`);
    return reply.ok ? 0 : 1;
  } finally {
    client.close();
  }
}
