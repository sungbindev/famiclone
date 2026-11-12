// ------------------------------------------------------------------------
// 名称：info-command.mjs
// 说明：info 命令：显示当前项目的目标、工具链与头文件目录，--json 时输出机器可读格式。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-10
// 备注：无
// ------------------------------------------------------------------------
import { describeProject } from './project-info.mjs';

/**
 * 执行 info 命令。
 * @param {{options: {target?: string, json: boolean}, context: import('../index.mjs').CliContext}} input 选项与运行环境
 * @returns {Promise<number>} 退出码
 */
export async function runInfoCommand({ options, context }) {
  const info = await describeProject({ cwd: context.cwd, targetOverride: options.target, env: context.env });
  if (options.json) {
    context.stdout.write(`${JSON.stringify(info, null, 2)}\n`);
    return 0;
  }
  const lines = [
    `项目根目录：${info.root}`,
    `配置文件：${info.configFile ?? '（无）'}`,
    `目标：${info.target.name}（${info.target.title}）`,
    `工具链：${info.toolchain.home}（${info.toolchain.source}，${info.toolchain.version}）`,
    `输出目录：${info.outDir}`,
    `头文件目录：\n  ${info.includeDirs.join('\n  ')}`,
    `汇编引入目录：\n  ${info.asmIncludeDirs.join('\n  ')}`,
  ];
  context.stdout.write(`${lines.join('\n')}\n`);
  return 0;
}
