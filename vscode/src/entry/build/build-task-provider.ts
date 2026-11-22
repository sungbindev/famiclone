// ------------------------------------------------------------------------
// 名称：build-task-provider.ts
// 说明：fcdev 任务的提供者：为工作区里的每个 fcdev 项目提供构建与运行任务，
//       并解析 tasks.json 里 type 为 fcdev 的任务。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-22
// 备注：任务用自定义执行（伪终端）运行，输出里的“文件(行): Error: 信息”由问题匹配器 $fcdev 识别；
//       项目的构建路径取其 src 目录，没有 src 时取项目目录。
// ------------------------------------------------------------------------

import * as path from 'node:path';
import * as vscode from 'vscode';
import type { FileSystem } from '../../contracts/files/file-system';
import type { FcdevProjectLocator } from '../../contracts/project/project-locator';
import type { Logger } from '../../platform/logging/logger';
import { TASK_COMMANDS, parseTaskDefinition, type FcdevTaskDefinition, type TaskCommand } from '../../shared/build/task-definition';
import { UserFacingError } from '../../shared/errors/user-facing-error';
import { runBuildTask, type BuildTaskDependencies } from './build-task-runner';

/** 任务类型，与 package.json 里声明的 taskDefinitions 一致。 */
export const TASK_TYPE = 'fcdev';

/** 任务来源名，显示在任务列表里。 */
const TASK_SOURCE = 'fcdev';

/** 问题匹配器，与 package.json 里声明的一致。 */
const PROBLEM_MATCHER = '$fcdev';

/** 各命令在任务名里的说明。 */
const COMMAND_LABELS: Readonly<Record<TaskCommand, string>> = { build: '构建', run: '运行' };

/** 运行任务要用到的能力。 */
export interface BuildTaskProviderDependencies extends BuildTaskDependencies {
  /** 文件系统，用来判断项目有没有 src 目录。 */
  readonly fileSystem: FileSystem;
  /** 查找工作区里的 fcdev 项目。 */
  readonly locator: FcdevProjectLocator;
  /** 日志。 */
  readonly logger: Logger;
}

/** 把换行转成终端要求的 \r\n。 */
function toTerminalText(text: string): string {
  return text.replace(/\r?\n/g, '\r\n');
}

/** 任务的伪终端：打开后立即运行，结束时以退出码关闭。 */
class BuildTaskTerminal implements vscode.Pseudoterminal {
  private readonly writeEmitter = new vscode.EventEmitter<string>();
  private readonly closeEmitter = new vscode.EventEmitter<number | void>();
  readonly onDidWrite = this.writeEmitter.event;
  readonly onDidClose = this.closeEmitter.event;

  /**
   * @param definition 任务定义。
   * @param folder 工作区文件夹路径。
   * @param dependencies 运行任务要用到的能力。
   */
  constructor(
    private readonly definition: FcdevTaskDefinition,
    private readonly folder: string,
    private readonly dependencies: BuildTaskProviderDependencies
  ) {}

  /** 终端打开：运行任务。 */
  open(): void {
    void this.execute();
  }

  /** 终端关闭：没有需要释放的资源。 */
  close(): void {
    this.writeEmitter.dispose();
  }

  /** 运行任务并以退出码结束。 */
  private async execute(): Promise<void> {
    const write = (text: string): void => this.writeEmitter.fire(toTerminalText(text));
    try {
      this.closeEmitter.fire(await runBuildTask(this.definition, this.folder, this.dependencies, write));
    } catch (error) {
      this.dependencies.logger.error('fcdev 任务出错', error);
      write(`错误：任务出错（${error instanceof Error ? error.message : String(error)}），详见“fcdev”输出通道\n`);
      this.closeEmitter.fire(1);
    }
  }
}

/** 构建与运行任务的提供者。 */
export class BuildTaskProvider implements vscode.TaskProvider {
  /** @param dependencies 运行任务要用到的能力。 */
  constructor(private readonly dependencies: BuildTaskProviderDependencies) {}

  /** 为工作区里的每个 fcdev 项目提供任务。 */
  async provideTasks(): Promise<vscode.Task[]> {
    const roots = await this.dependencies.locator.findProjectRoots();
    const tasks: vscode.Task[] = [];
    for (const root of roots) {
      const folder = vscode.workspace.getWorkspaceFolder(vscode.Uri.file(root));
      if (folder === undefined) continue;
      const buildPath = this.dependencies.fileSystem.kindOf(path.join(root, 'src')) === 'directory' ? path.join(root, 'src') : root;
      const relative = path.relative(folder.uri.fsPath, buildPath).split(path.sep).join('/');
      for (const command of TASK_COMMANDS) {
        const common = { type: TASK_TYPE, command, ...(relative === '' ? {} : { path: relative }) };
        tasks.push(this.taskFor(common, folder));
        if (command === 'run') tasks.push(this.taskFor({ ...common, debug: true }, folder));
      }
    }
    return tasks;
  }

  /** 解析 tasks.json 里的任务。 */
  resolveTask(task: vscode.Task): vscode.Task | undefined {
    const folder = task.scope !== undefined && typeof task.scope === 'object' ? task.scope : vscode.workspace.workspaceFolders?.[0];
    if (folder === undefined) return undefined;
    try {
      parseTaskDefinition(task.definition);
    } catch (error) {
      if (error instanceof UserFacingError) this.dependencies.logger.info(`任务定义不合法：${error.message}`);
      return undefined;
    }
    return this.taskFor(task.definition, folder, task.name);
  }

  /**
   * 创建一个自定义执行的任务。
   * @param raw 任务定义里的字段。
   * @param folder 工作区文件夹。
   * @param name 任务名；不给时按命令和路径生成。
   * @returns 任务。
   * @throws FcdevError 任务定义不合法。
   */
  taskFor(raw: vscode.TaskDefinition, folder: vscode.WorkspaceFolder, name?: string): vscode.Task {
    const definition = parseTaskDefinition(raw);
    const { command } = definition;
    const label = name ?? `${definition.debug === true ? '调试运行' : COMMAND_LABELS[command]}${definition.path === undefined ? '' : ` ${definition.path}`}`;
    const task = new vscode.Task(
      { ...raw, type: TASK_TYPE, command },
      folder,
      label,
      TASK_SOURCE,
      new vscode.CustomExecution(() => Promise.resolve(new BuildTaskTerminal(definition, folder.uri.fsPath, this.dependencies))),
      PROBLEM_MATCHER
    );
    if (command === 'build') task.group = vscode.TaskGroup.Build;
    return task;
  }
}

/**
 * 注册 fcdev 任务提供者。
 * @param context 扩展上下文，注册的资源随扩展停用而释放。
 * @param dependencies 运行任务要用到的能力。
 * @returns 任务提供者，命令用它创建任务。
 */
export function registerBuildTasks(context: vscode.ExtensionContext, dependencies: BuildTaskProviderDependencies): BuildTaskProvider {
  const provider = new BuildTaskProvider(dependencies);
  context.subscriptions.push(vscode.tasks.registerTaskProvider(TASK_TYPE, provider));
  return provider;
}
