/*
 * ------------------------------------------------------------------------
 * 名称：launch-args.ts
 * 说明：命令行启动参数：从启动参数里取出要打开的卡带路径（--cartridge=路径），以及调试服务的端口与令牌（--debug-port=端口、--debug-token=令牌，也可以写成“--debug-port 端口”）。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-14
 * 备注：卡带路径只用于主进程自己发起的 cartridges.load（见 bootstrap/main/main.ts），只接受 .nes 文件，其他一律忽略；
 *       调试端口必须是 1 到 65535 的整数，令牌必须是 1 到 128 个可见的 ASCII 字符，不合格的忽略。
 * ------------------------------------------------------------------------
 */

/** 卡带路径参数的前缀。 */
const CARTRIDGE_ARGUMENT_PREFIX = '--cartridge=';
/** 卡带文件的扩展名。 */
const CARTRIDGE_EXTENSION = '.nes';
/** 调试端口与令牌参数的名称。 */
const DEBUG_PORT_ARGUMENT = '--debug-port';
const DEBUG_TOKEN_ARGUMENT = '--debug-token';
/** 令牌的最大长度。 */
const MAX_TOKEN_LENGTH = 128;

/** 调试服务的启动参数。 */
export interface DebugLaunchOptions {
  readonly port?: number;
  readonly token?: string;
}

/** 取“--名称=值”或“--名称 值”里的值；没有该参数时为 undefined。 */
function argumentValue(argv: readonly string[], name: string): string | undefined {
  for (let index = 0; index < argv.length; index += 1) {
    const argument = argv[index] ?? '';
    if (argument.startsWith(`${name}=`)) return argument.slice(name.length + 1);
    if (argument === name) return argv[index + 1];
  }
  return undefined;
}

/**
 * 从启动参数里取调试服务的端口与令牌。
 * @param argv 进程的启动参数。
 * @returns 合格的端口与令牌；没有或不合格的没有对应的字段。
 */
export function debugOptionsFromArgv(argv: readonly string[]): DebugLaunchOptions {
  const portText = argumentValue(argv, DEBUG_PORT_ARGUMENT);
  const tokenText = argumentValue(argv, DEBUG_TOKEN_ARGUMENT);
  const port = portText !== undefined && /^\d{1,5}$/.test(portText) && Number(portText) >= 1 && Number(portText) <= 65_535 ? Number(portText) : undefined;
  const token = tokenText !== undefined && tokenText.length >= 1 && tokenText.length <= MAX_TOKEN_LENGTH && /^[\u0021-\u007e]+$/.test(tokenText) ? tokenText : undefined;
  return { ...(port === undefined ? {} : { port }), ...(token === undefined ? {} : { token }) };
}

/**
 * 从启动参数里取卡带路径。
 * @param argv 进程的启动参数。
 * @returns 卡带路径；没有参数或不是 .nes 文件时为 undefined。
 */
export function cartridgePathFromArgv(argv: readonly string[]): string | undefined {
  for (const argument of argv) {
    if (!argument.startsWith(CARTRIDGE_ARGUMENT_PREFIX)) continue;
    const path = argument.slice(CARTRIDGE_ARGUMENT_PREFIX.length);
    return path.toLowerCase().endsWith(CARTRIDGE_EXTENSION) ? path : undefined;
  }
  return undefined;
}
