// ------------------------------------------------------------------------
// 名称：docs-command.mjs
// 说明：docs 命令：列出开发文档的主题，查询某份文档的本机路径。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-11
// 备注：只读、不联网、不需要 fcdev.json；--json 输出机器可读格式。
// ------------------------------------------------------------------------
import { existsSync } from 'node:fs';
import { docsDirectory, listDocs, queryDocs } from '../../docs/index.mjs';
import { FcdevError } from '../../shared/index.mjs';

/**
 * 执行 docs 命令。
 * @param {{positionals: string[], options: {json: boolean}, context: import('../index.mjs').CliContext}} input 参数、选项与运行环境
 * @returns {Promise<number>} 退出码
 */
export async function runDocsCommand({ positionals, options, context }) {
  const directory = docsDirectory();
  if (!existsSync(directory)) {
    throw new FcdevError(`找不到文档目录：${directory}`, { code: 'DOCS_MISSING', hint: '这份 fcdev 不带文档，请升级到带文档的版本' });
  }

  const query = positionals.join(' ').trim();
  if (query === '') {
    const docs = listDocs();
    if (options.json) {
      context.stdout.write(`${JSON.stringify(docs, null, 2)}\n`);
      return 0;
    }
    const width = Math.max(...docs.map((doc) => doc.topic.length));
    const lines = [`fcdev 开发文档：${docs.length} 份`, `本机目录：${directory}`, ''];
    for (const doc of docs) lines.push(`  ${doc.topic.padEnd(width)}  ${doc.title}：${doc.summary}`);
    lines.push('', '查询：fcdev docs <主题>（本机路径）、fcdev docs <词>（按主题、文件名、标题与说明搜索）');
    context.stdout.write(`${lines.join('\n')}\n`);
    return 0;
  }

  const { kind, matches } = queryDocs(query);
  if (matches.length === 0) {
    throw new FcdevError(`没有找到“${query}”`, { code: 'DOCS_NOT_FOUND', hint: '不带查询词运行 fcdev docs 可以看到全部主题' });
  }
  if (options.json) {
    context.stdout.write(`${JSON.stringify(matches, null, 2)}\n`);
    return 0;
  }
  if (matches.length === 1) {
    const [doc] = matches;
    context.stdout.write(`${doc.title}（主题 ${doc.topic}）\n说明：${doc.summary}\n本机：${doc.localPath}\n`);
    return 0;
  }
  const width = Math.max(...matches.map((doc) => doc.topic.length));
  const lines = [`包含“${query}”的文档：${matches.length} 份（${kind === 'file' ? '按文件名' : '按关键词'}）`];
  for (const doc of matches) lines.push(`  ${doc.topic.padEnd(width)}  ${doc.title}：${doc.summary}`);
  lines.push('', '用 fcdev docs <主题> 查看某份文档的本机路径');
  context.stdout.write(`${lines.join('\n')}\n`);
  return 0;
}
