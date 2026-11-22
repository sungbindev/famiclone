// ------------------------------------------------------------------------
// 名称：usage-docs-commands.ts
// 说明：使用文档的命令：列出主题，选一份用 Markdown 预览打开。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-22
// 备注：命令可带主题名参数直接打开；文档随扩展发布，只读、不联网。
// ------------------------------------------------------------------------

import * as vscode from 'vscode';
import type { UsageDocsService } from '../../contracts/usage-docs/usage-docs-service';
import type { Logger } from '../../platform/logging/logger';
import { UserFacingError } from '../../shared/errors/user-facing-error';

/** 命令的标识，与 package.json 里声明的一致。 */
export const OPEN_DOCS_COMMAND = 'fcdev.openDocs';

/**
 * 注册“打开使用文档”命令。
 * @param context 扩展上下文，注册的资源随扩展停用而释放。
 * @param docs 使用文档服务。
 * @param logger 日志。
 */
export function registerUsageDocsCommands(context: vscode.ExtensionContext, docs: UsageDocsService, logger: Logger): void {
  /** 打开文档：给了主题名就直接打开，否则让用户选。 */
  const openDocs = async (topic?: unknown): Promise<void> => {
    const all = docs.list();
    const given = typeof topic === 'string' ? all.find((doc) => doc.topic === topic) : undefined;
    const doc =
      given ??
      (await vscode.window.showQuickPick(
        all.map((item) => ({ label: item.title, description: item.topic, detail: item.summary, doc: item })),
        { placeHolder: '选择要阅读的使用文档', matchOnDescription: true, matchOnDetail: true }
      ))?.doc;
    if (doc === undefined) return;
    await vscode.commands.executeCommand('markdown.showPreview', vscode.Uri.file(doc.localPath));
  };

  context.subscriptions.push(
    vscode.commands.registerCommand(OPEN_DOCS_COMMAND, async (topic?: unknown) => {
      try {
        await openDocs(topic);
      } catch (error) {
        if (error instanceof UserFacingError) {
          void vscode.window.showErrorMessage(error.message);
          return;
        }
        logger.error('打开使用文档出错', error);
        void vscode.window.showErrorMessage('打开使用文档时出错，详情见“输出”面板里的 fcdev');
      }
    })
  );
}
