// ------------------------------------------------------------------------
// 名称：scaffold-commands.ts
// 说明：新建项目的命令：选目标与语言，选一个空目录，按模板生成项目，并询问是否打开。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-22
// 备注：目录不为空时报错并说明原因，不覆盖已有文件。
// ------------------------------------------------------------------------

import * as vscode from 'vscode';
import type { ScaffoldService } from '../../contracts/scaffold/scaffold-service';
import type { Logger } from '../../platform/logging/logger';
import { FcdevError } from '../../shared/errors/fcdev-error';
import { UserFacingError } from '../../shared/errors/user-facing-error';
import { LANGUAGE_LABELS } from '../../shared/scaffold/scaffold-files';

/** 命令的标识，与 package.json 里声明的一致。 */
export const NEW_PROJECT_COMMAND = 'fcdev.newProject';

/** 创建完成后的选项文字。 */
const OPEN_HERE = '在当前窗口打开';
const OPEN_NEW_WINDOW = '在新窗口打开';

/**
 * 注册“新建项目”命令。
 * @param context 扩展上下文，注册的资源随扩展停用而释放。
 * @param scaffold 新建项目服务。
 * @param logger 日志。
 */
export function registerScaffoldCommands(context: vscode.ExtensionContext, scaffold: ScaffoldService, logger: Logger): void {
  /** 新建项目的完整流程。 */
  const createProject = async (): Promise<void> => {
    const items = scaffold.listTemplates().map((template) => ({
      label: `${template.title}，${LANGUAGE_LABELS[template.language]}`,
      template
    }));
    const picked = await vscode.window.showQuickPick(items, { placeHolder: '选择目标与语言' });
    if (picked === undefined) return;
    const folders = await vscode.window.showOpenDialog({
      canSelectFolders: true,
      canSelectFiles: false,
      canSelectMany: false,
      openLabel: '选择项目目录',
      title: '选择一个空目录作为项目目录（不存在的目录请先创建）'
    });
    const folder = folders?.[0];
    if (folder === undefined) return;
    const created = scaffold.create({ directory: folder.fsPath, target: picked.template.target, language: picked.template.language });
    logger.info(`已创建项目 ${created.directory}：${created.files.join('、')}`);
    const choice = await vscode.window.showInformationMessage(`已创建项目，共 ${created.files.length} 个文件`, OPEN_HERE, OPEN_NEW_WINDOW);
    if (choice !== undefined) await vscode.commands.executeCommand('vscode.openFolder', folder, { forceNewWindow: choice === OPEN_NEW_WINDOW });
  };

  context.subscriptions.push(
    vscode.commands.registerCommand(NEW_PROJECT_COMMAND, async () => {
      try {
        await createProject();
      } catch (error) {
        if (error instanceof UserFacingError) {
          const hint = error instanceof FcdevError && error.hint !== '' ? `\n${error.hint}` : '';
          void vscode.window.showErrorMessage(`${error.message}${hint}`);
          return;
        }
        logger.error('新建项目出错', error);
        void vscode.window.showErrorMessage('新建项目时出错，详情见“输出”面板里的 fcdev');
      }
    })
  );
}
