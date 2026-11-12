// ------------------------------------------------------------------------
// 名称：build-command.mjs
// 说明：build 命令：按路径构建一个程序，输出进度与结果。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-10
// 备注：工具（cc65、ca65、ld65）的输出原样写到标准错误，格式保持不变，供问题匹配器使用。
// ------------------------------------------------------------------------
import path from 'node:path';
import { buildPath } from '../../build/index.mjs';
import { FcdevError } from '../../shared/index.mjs';

/**
 * 执行 build 命令。
 * @param {{positionals: string[], options: {target?: string, verbose: boolean}, context: import('../index.mjs').CliContext}} input 参数与运行环境
 * @returns {Promise<number>} 退出码
 * @throws {FcdevError} 位置参数多于一个，或构建失败
 */
export async function runBuildCommand({ positionals, options, context }) {
  if (positionals.length > 1) {
    throw new FcdevError('build 只接受一个路径参数', { code: 'ARGUMENT_INVALID', hint: '用法：fcdev build [目录或文件]' });
  }
  const reporter = {
    log: (message) => context.stdout.write(`${message}\n`),
    write: (text) => context.stderr.write(text),
  };
  const { plan, result } = await buildPath(
    { pathArg: positionals[0], cwd: context.cwd, targetOverride: options.target, env: context.env },
    reporter,
  );
  const shown = path.relative(context.cwd, result.outputFile) || result.outputFile;
  context.stdout.write(`已生成 ${shown}（${result.size} 字节，目标 ${plan.target.name}，重新编译 ${result.compiled} 个文件，沿用 ${result.reused} 个）\n`);
  return 0;
}
