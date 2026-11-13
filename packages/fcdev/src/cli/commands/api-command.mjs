// ------------------------------------------------------------------------
// 名称：api-command.mjs
// 说明：api 命令：查询目标的 C 接口文档（函数、模块、系统调用号）。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-11
// 备注：文档来自目标头文件里的文档注释，所以随库更新，不另存一份。没有查询词时列出全部模块；--json 输出机器可读格式。
// ------------------------------------------------------------------------
import { formatFunction, formatSummary, groupByModule, loadTargetApi, queryApi } from '../../api/index.mjs';
import { loadProjectConfig } from '../../config/index.mjs';
import { FcdevError } from '../../shared/index.mjs';
import { loadTarget } from '../../targets/index.mjs';

/**
 * 执行 api 命令。
 * @param {{positionals: string[], options: {target?: string, json: boolean}, context: import('../index.mjs').CliContext}} input 参数、选项与运行环境
 * @returns {Promise<number>} 退出码
 */
export async function runApiCommand({ positionals, options, context }) {
  const config = loadProjectConfig(context.cwd, context.cwd);
  const targetName = options.target ?? config.target;
  if (!targetName) {
    throw new FcdevError('没有指定目标', { code: 'TARGET_MISSING', hint: '在 fcdev.json 里写 "target"，或用 --target <名> 指定' });
  }
  const target = loadTarget(targetName);
  const functions = loadTargetApi(target);
  if (functions.length === 0) {
    throw new FcdevError(`目标 ${target.name} 没有 C 接口文档`, { code: 'API_MISSING', hint: '目前只有 sb2000 目标提供运行库函数的文档' });
  }

  const query = positionals.join(' ').trim();
  if (query === '') {
    const groups = groupByModule(functions);
    if (options.json) {
      const modules = [...groups].map(([name, items]) => ({ module: name, header: items[0].header, functions: items.map((fn) => fn.name) }));
      context.stdout.write(`${JSON.stringify(modules, null, 2)}\n`);
      return 0;
    }
    const lines = [`${target.title} 的 C 接口：${functions.length} 个函数，${groups.size} 个模块`, ''];
    for (const [name, items] of groups) lines.push(`  ${name.padEnd(10)} ${String(items.length).padStart(3)} 个函数  ${items[0].header}`);
    lines.push('', '查询：npx fcdev api <函数名>（完整文档）、npx fcdev api <模块名>（函数列表）、npx fcdev api <$功能号>（用到该系统调用的函数）、npx fcdev api <词>（按名称与说明搜索）');
    context.stdout.write(`${lines.join('\n')}\n`);
    return 0;
  }

  const { kind, matches } = queryApi(functions, query);
  if (matches.length === 0) {
    throw new FcdevError(`没有找到“${query}”`, { code: 'API_NOT_FOUND', hint: '不带查询词运行 npx fcdev api 可以看到全部模块' });
  }
  if (options.json) {
    context.stdout.write(`${JSON.stringify(matches, null, 2)}\n`);
    return 0;
  }
  if (kind === 'function') {
    context.stdout.write(`${matches.map(formatFunction).join('\n\n')}\n`);
    return 0;
  }
  const width = Math.max(...matches.map((fn) => fn.name.length));
  const title = { module: `模块 ${query}`, syscall: `系统调用 ${query}`, text: `包含“${query}”` }[kind];
  context.stdout.write(`${title}：${matches.length} 个函数\n${matches.map((fn) => formatSummary(fn, width)).join('\n')}\n`);
  return 0;
}
