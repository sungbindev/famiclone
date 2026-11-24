// ------------------------------------------------------------------------
// 名称：workspace-fcdev-project-locator.ts
// 说明：在工作区里查找 FC Dev 项目：找到全部 fcdev.json，取它们所在的目录。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-10
// 备注：不含 node_modules 里的 fcdev.json（依赖包自带的模板或示例）。
// ------------------------------------------------------------------------

import * as path from 'node:path';
import * as vscode from 'vscode';
import type { FcdevProjectLocator } from '../../contracts/project/project-locator';

/** 项目配置文件的匹配模式。 */
const CONFIG_FILES = '**/fcdev.json';

/** 排除的目录。 */
const EXCLUDED_DIRECTORIES = '**/node_modules/**';

/** 经 VS Code 在工作区里查找 FC Dev 项目。 */
export class WorkspaceFcdevProjectLocator implements FcdevProjectLocator {
  /** @inheritdoc */
  async findProjectRoots(): Promise<readonly string[]> {
    const uris = await vscode.workspace.findFiles(CONFIG_FILES, EXCLUDED_DIRECTORIES);
    return uris.map((uri) => path.dirname(uri.fsPath));
  }
}
