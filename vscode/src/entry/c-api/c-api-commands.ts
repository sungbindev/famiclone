// ------------------------------------------------------------------------
// 名称：c-api-commands.ts
// 说明：查询 C 接口的命令：输入函数名、模块名、系统调用号或关键词，
//       结果显示在一个 Markdown 文档里。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-22
// 备注：目标取活动文件所属项目的 fcdev.json；不输入查询词时列出全部模块。
// ------------------------------------------------------------------------

import * as path from 'node:path';
import * as vscode from 'vscode';
import type { CApiService } from '../../contracts/c-api/c-api-service';
import type { Logger } from '../../platform/logging/logger';
import { formatModules, formatQueryResult, groupByModule, queryApi } from '../../shared/c-api/api-functions';
import { UserFacingError } from '../../shared/errors/user-facing-error';

/** 命令的标识，与 package.json 里声明的一致。 */
export const QUERY_API_COMMAND = 'fcdev.queryApi';

/** 查询的起始目录：活动文件所在目录，没有时取第一个工作区文件夹。 */
function startDirectory(): string | undefined {
  const document = vscode.window.activeTextEditor?.document;
  if (document?.uri.scheme === 'file') return path.dirname(document.uri.fsPath);
  return vscode.workspace.workspaceFolders?.[0]?.uri.fsPath;
}

/**
 * 注册“查询 C 接口”命令。
 * @param context 扩展上下文，注册的资源随扩展停用而释放。
 * @param api C 接口读取服务。
 * @param logger 日志。
 */
export function registerCApiCommands(context: vscode.ExtensionContext, api: CApiService, logger: Logger): void {
  /** 查询并显示结果。 */
  const queryAndShow = async (): Promise<void> => {
    const cwd = startDirectory();
    if (cwd === undefined) {
      void vscode.window.showInformationMessage('请先打开一个 FC Dev 项目的文件');
      return;
    }
    const target = api.describe({ cwd });
    const query = await vscode.window.showInputBox({
      prompt: `查询 ${target.targetTitle} 的 C 接口：函数名、模块名、系统调用号（如 $7B）或关键词；留空列出全部模块`,
      placeHolder: '例如 sb_file_open、file、$7B'
    });
    if (query === undefined) return;
    let text: string;
    if (query.trim() === '') {
      text = formatModules(target.targetTitle, groupByModule(target.functions));
    } else {
      const result = queryApi(target.functions, query);
      text = result.matches.length === 0 ? `没有找到“${query.trim()}”` : formatQueryResult(query.trim(), result);
    }
    const document = await vscode.workspace.openTextDocument({ content: `\`\`\`text\n${text}\n\`\`\`\n`, language: 'markdown' });
    await vscode.window.showTextDocument(document, { preview: true });
  };

  context.subscriptions.push(
    vscode.commands.registerCommand(QUERY_API_COMMAND, async () => {
      try {
        await queryAndShow();
      } catch (error) {
        if (error instanceof UserFacingError) {
          void vscode.window.showErrorMessage(error.message);
          return;
        }
        logger.error('查询 C 接口出错', error);
        void vscode.window.showErrorMessage('查询 C 接口时出错，详情见“输出”面板里的 FC Dev');
      }
    })
  );
}
