// ------------------------------------------------------------------------
// 名称：disk-command.mjs
// 说明：disk 命令：构建程序并把它放进磁盘镜像。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-10
// 备注：只有声明了磁盘能力的目标可用；镜像生成在程序产物旁边。
// ------------------------------------------------------------------------
import path from 'node:path';
import { buildDiskImage, buildPath } from '../../build/index.mjs';
import { FcdevError } from '../../shared/index.mjs';

/**
 * 执行 disk 命令。
 * @param {{positionals: string[], options: {target?: string}, context: import('../index.mjs').CliContext}} input 参数与运行环境
 * @returns {Promise<number>} 退出码
 * @throws {FcdevError} 位置参数多于一个、目标不支持磁盘镜像，或构建失败
 */
export async function runDiskCommand({ positionals, options, context }) {
  if (positionals.length > 1) {
    throw new FcdevError('disk 只接受一个路径参数', { code: 'ARGUMENT_INVALID', hint: '用法：npx fcdev disk [目录或文件]' });
  }
  const reporter = {
    log: (message) => context.stdout.write(`${message}\n`),
    write: (text) => context.stderr.write(text),
  };
  const { plan, result } = await buildPath(
    { pathArg: positionals[0], cwd: context.cwd, targetOverride: options.target, env: context.env },
    reporter,
  );
  const { imageFile, size } = buildDiskImage(plan, result);
  context.stdout.write(`已生成 ${path.relative(context.cwd, imageFile) || imageFile}（${size} 字节，含 ${path.basename(result.outputFile)}）\n`);
  return 0;
}
