// ------------------------------------------------------------------------
// 名称：build-commands.ts
// 说明：构建和运行相关的命令：构建项目、构建当前文件、构建当前目录、运行、调试运行、
//       显示项目信息。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-22
// 备注：构建与运行命令都经 fcdev 任务运行，输出在终端面板里，错误经问题匹配器出现在“问题”面板；
//       项目取活动文件所属的项目，工作区里有多个项目又没有活动文件时让用户选；运行需要先在
//       设置里指定 Ember 的位置。
// ------------------------------------------------------------------------

import * as path from 'node:path';
import * as vscode from 'vscode';
import type { FileSystem } from '../../contracts/files/file-system';
import type { EmulatorSettings } from '../../contracts/emulator/emulator-settings';
import type { ProjectDescriber } from '../../contracts/project/project-describer';
import type { FcdevProjectLocator } from '../../contracts/project/project-locator';
import type { Logger } from '../../platform/logging/logger';
import { getSourceLanguage } from '../../shared/build/build-text';
import { formatProjectDescription } from '../../shared/project/project-info';
import { UserFacingError } from '../../shared/errors/user-facing-error';
import { TASK_TYPE, type BuildTaskProvider } from './build-task-provider';

/** 命令的标识，与 package.json 里声明的一致。 */
export const BUILD_COMMANDS = {
  build: 'fcdev.build',
  buildFile: 'fcdev.buildCurrentFile',
  buildDirectory: 'fcdev.buildCurrentDirectory',
  run: 'fcdev.run',
  debugRun: 'fcdev.debugRun',
  showInfo: 'fcdev.showProjectInfo'
} as const;

/** 命令要用到的能力。 */
export interface BuildCommandDependencies {
  /** 任务提供者，用来创建任务。 */
  readonly tasks: BuildTaskProvider;
  /** 文件系统。 */
  readonly fileSystem: FileSystem;
  /** 查找工作区里的 FC Dev 项目。 */
  readonly locator: FcdevProjectLocator;
  /** 项目描述器。 */
  readonly describer: ProjectDescriber;
  /** 模拟器设置，运行前检查有没有指定 Ember 的位置。 */
  readonly settings: EmulatorSettings;
  /** 日志。 */
  readonly logger: Logger;
}

/** 路径 child 是否在目录 parent 里面（含相同）。 */
function isInside(parent: string, child: string): boolean {
  const relative = path.relative(parent, child);
  return relative === '' || (!relative.startsWith('..') && !path.isAbsolute(relative));
}

/** 活动编辑器里文件的路径。 */
function activeFile(): string | undefined {
  const document = vscode.window.activeTextEditor?.document;
  return document?.uri.scheme === 'file' ? document.uri.fsPath : undefined;
}

/**
 * 选出要操作的项目：活动文件所属的项目，只有一个项目就用它，否则让用户选。
 * @param roots 工作区里全部项目的根目录。
 * @returns 项目根目录；没有项目或用户取消时为 undefined。
 */
async function pickProject(roots: readonly string[]): Promise<string | undefined> {
  if (roots.length === 0) {
    void vscode.window.showInformationMessage('工作区里没有 FC Dev 项目（找不到 fcdev.json）');
    return undefined;
  }
  const file = activeFile();
  const owner = file === undefined ? undefined : roots.filter((root) => isInside(root, file)).toSorted((a, b) => b.length - a.length)[0];
  if (owner !== undefined) return owner;
  if (roots.length === 1) return roots[0];
  const picked = await vscode.window.showQuickPick(
    roots.map((root) => ({ label: path.basename(root), description: root, root })),
    { placeHolder: '选择 FC Dev 项目' }
  );
  return picked?.root;
}

/**
 * 注册构建相关的命令。
 * @param context 扩展上下文，注册的资源随扩展停用而释放。
 * @param dependencies 命令要用到的能力。
 */
export function registerBuildCommands(context: vscode.ExtensionContext, dependencies: BuildCommandDependencies): void {
  const { tasks, fileSystem, locator, describer, settings, logger } = dependencies;
  const output = vscode.window.createOutputChannel('FC Dev 项目信息');

  /** 在工作区文件夹里运行一个构建路径的任务。 */
  const runTask = async (command: 'build' | 'run', absolutePath: string, debug = false): Promise<void> => {
    const folder = vscode.workspace.getWorkspaceFolder(vscode.Uri.file(absolutePath));
    if (folder === undefined) {
      void vscode.window.showWarningMessage('只能构建工作区文件夹里的内容');
      return;
    }
    const relative = path.relative(folder.uri.fsPath, absolutePath).split(path.sep).join('/');
    const task = tasks.taskFor({ type: TASK_TYPE, command, ...(relative === '' ? {} : { path: relative }), ...(debug ? { debug } : {}) }, folder);
    await vscode.tasks.executeTask(task);
  };

  /** 运行整个项目：没有指定 Ember 的位置时提示去设置。 */
  const runProject = async (debug: boolean): Promise<void> => {
    if (settings.executablePath() === undefined) {
      const openSettings = '打开设置';
      const choice = await vscode.window.showErrorMessage('还没有设置 Ember 的位置，请在设置 fcdev.emulator.path 里填写 Ember 可执行文件的路径', openSettings);
      if (choice === openSettings) await vscode.commands.executeCommand('workbench.action.openSettings', 'fcdev.emulator.path');
      return;
    }
    const root = await pickProject(await locator.findProjectRoots());
    if (root === undefined) return;
    const source = path.join(root, 'src');
    await runTask('run', fileSystem.kindOf(source) === 'directory' ? source : root, debug);
  };

  /** 构建整个项目：有 src 目录就构建它，否则构建项目目录。 */
  const buildProject = async (): Promise<void> => {
    const root = await pickProject(await locator.findProjectRoots());
    if (root === undefined) return;
    const source = path.join(root, 'src');
    await runTask('build', fileSystem.kindOf(source) === 'directory' ? source : root);
  };

  /** 构建活动文件：只有 .c、.s、.asm 能单独构建。 */
  const buildFile = async (): Promise<void> => {
    const file = activeFile();
    if (file === undefined || getSourceLanguage(file) === null) {
      void vscode.window.showWarningMessage('当前文件不是 .c、.s 或 .asm 源文件');
      return;
    }
    await runTask('build', file);
  };

  /** 构建活动文件所在的目录。 */
  const buildDirectory = async (): Promise<void> => {
    const file = activeFile();
    if (file === undefined) {
      void vscode.window.showWarningMessage('没有打开的文件');
      return;
    }
    await runTask('build', path.dirname(file));
  };

  /** 显示项目信息。 */
  const showInfo = async (): Promise<void> => {
    const root = await pickProject(await locator.findProjectRoots());
    if (root === undefined) return;
    try {
      output.clear();
      output.appendLine(formatProjectDescription(describer.describe({ cwd: root })));
      output.show(true);
    } catch (error) {
      if (!(error instanceof UserFacingError)) throw error;
      void vscode.window.showErrorMessage(error.message);
    }
  };

  /** 命令出错时记日志并提示，不让异常漏到 VS Code。 */
  const guarded = (name: string, action: () => Promise<void>) => async (): Promise<void> => {
    try {
      await action();
    } catch (error) {
      logger.error(`${name} 出错`, error);
      void vscode.window.showErrorMessage(`${name}时出错，详情见“输出”面板里的 fcdev`);
    }
  };

  context.subscriptions.push(
    output,
    vscode.commands.registerCommand(BUILD_COMMANDS.build, guarded('构建项目', buildProject)),
    vscode.commands.registerCommand(BUILD_COMMANDS.buildFile, guarded('构建当前文件', buildFile)),
    vscode.commands.registerCommand(BUILD_COMMANDS.buildDirectory, guarded('构建当前目录', buildDirectory)),
    vscode.commands.registerCommand(BUILD_COMMANDS.run, guarded('运行', () => runProject(false))),
    vscode.commands.registerCommand(BUILD_COMMANDS.debugRun, guarded('调试运行', () => runProject(true))),
    vscode.commands.registerCommand(BUILD_COMMANDS.showInfo, guarded('显示项目信息', showInfo))
  );
}
