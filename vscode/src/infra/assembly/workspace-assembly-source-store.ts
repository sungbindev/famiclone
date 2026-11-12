// ------------------------------------------------------------------------
// 名称：workspace-assembly-source-store.ts
// 说明：汇编源文件的存取实现：经 VS Code 的文件系统读取文本，在工作区里查找汇编文件。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-10
// 备注：只列出 .s、.asm、.inc，排除 node_modules 与构建输出目录；读取的是磁盘上的内容。
// ------------------------------------------------------------------------

import * as vscode from 'vscode';
import type { AssemblySourceStore } from '../../contracts/assembly/assembly-source-store';

/** 汇编文件的匹配模式。 */
const ASSEMBLY_FILES = '**/*.{s,asm,inc}';

/** 排除的目录：依赖、构建输出与版本库。 */
const EXCLUDED_DIRECTORIES = '**/{node_modules,build,out,dist,.git}/**';

/** 工作区文件数量的上限，防止巨大的工作区拖慢首次解析。 */
const MAX_FILES = 2000;

/** 经 VS Code 文件系统存取汇编源文件。 */
export class WorkspaceAssemblySourceStore implements AssemblySourceStore {
  private readonly decoder = new TextDecoder('utf-8');

  /** @inheritdoc */
  async read(path: string): Promise<string | undefined> {
    try {
      const bytes = await vscode.workspace.fs.readFile(vscode.Uri.file(path));
      return this.decoder.decode(bytes);
    } catch {
      // 文件不存在或没有权限：对调用者来说都是“读不到”。
      return undefined;
    }
  }

  /** @inheritdoc */
  async listWorkspaceFiles(): Promise<readonly string[]> {
    const uris = await vscode.workspace.findFiles(ASSEMBLY_FILES, EXCLUDED_DIRECTORIES, MAX_FILES);
    return uris.map((uri) => uri.fsPath);
  }
}
