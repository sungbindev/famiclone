/*
 * ------------------------------------------------------------------------
 * 名称：launch-args.ts
 * 说明：命令行启动参数：从启动参数里取出要打开的卡带路径（--cartridge=路径）。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-14
 * 备注：路径只用于主进程自己发起的 cartridges.load（见 bootstrap/main/main.ts）；只接受 .nes 文件，其他一律忽略。
 * ------------------------------------------------------------------------
 */

/** 卡带路径参数的前缀。 */
const CARTRIDGE_ARGUMENT_PREFIX = '--cartridge=';
/** 卡带文件的扩展名。 */
const CARTRIDGE_EXTENSION = '.nes';

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
