// ------------------------------------------------------------------------
// 名称：c-intellisense-commands.ts
// 说明：C 智能感知的入口：注册“同步配置”命令，并在打开工作区、fcdev.json 变化时自动同步。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-10
// 备注：自动同步由设置 fcdev.intelliSense.autoSync 控制；自动同步只记日志，失败时不弹提示，
//       手动运行命令才弹出结果；fcdev.json 的变化经 500 毫秒合并后再同步。
//       同步成功后，项目的 c_cpp_properties.json 里还有其他配置时，每个项目每次会话提示一次，
//       因为 C/C++ 扩展使用哪个配置只能由用户选；设置 promptConfigurationSelection 可关闭提示。
// ------------------------------------------------------------------------

import * as path from 'node:path';
import * as vscode from 'vscode';
import type { IntelliSenseSyncService, SyncResult } from '../../contracts/c-intellisense/intellisense-sync-service';
import type { CppConfigurationInspector } from '../../contracts/c-intellisense/cpp-configuration-inspector';
import type { ProjectInfoProvider } from '../../contracts/project/project-info';
import type { Logger } from '../../platform/logging/logger';

/** 同步命令的标识，与 package.json 里声明的命令一致。 */
export const SYNC_COMMAND = 'fcdev.syncIntelliSense';

/** 设置分组与自动同步开关的键，与 package.json 里声明的设置一致。 */
const SETTINGS_SECTION = 'fcdev.intelliSense';
const AUTO_SYNC_KEY = 'autoSync';
const PROMPT_KEY = 'promptConfigurationSelection';

/** C/C++ 扩展的标识，以及它选择配置的命令与提示里的按钮文字。 */
const CPPTOOLS_EXTENSION_ID = 'ms-vscode.cpptools';
const SELECT_CONFIGURATION_COMMAND = 'C_Cpp.ConfigurationSelect';
const SELECT_BUTTON = '选择配置';

/** 项目配置文件的监视模式。 */
const CONFIG_FILES = '**/fcdev.json';

/** 合并连续变化的等待时间，单位毫秒。 */
const DEBOUNCE_MS = 500;

/** 手动同步时进度通知的标题。 */
const PROGRESS_TITLE = 'fcdev：同步 C/C++ 智能感知配置';

/** 自动同步是否开启。 */
function isAutoSyncEnabled(): boolean {
  return vscode.workspace.getConfiguration(SETTINGS_SECTION).get<boolean>(AUTO_SYNC_KEY, true);
}

/** 提示选择配置是否开启。 */
function isPromptEnabled(): boolean {
  return vscode.workspace.getConfiguration(SETTINGS_SECTION).get<boolean>(PROMPT_KEY, true);
}

/** 把同步结果逐条写进日志。 */
function logResults(logger: Logger, results: readonly SyncResult[]): void {
  for (const result of results) logger.info(`${result.ok ? '已同步' : '同步失败'}（${result.root}）：${result.message}`);
}

/**
 * 注册 C 智能感知的命令与自动同步。
 * @param context 扩展上下文，注册的资源随扩展停用而释放。
 * @param sync C 智能感知配置同步服务。
 * @param projects 项目信息提供者，fcdev.json 变化时丢弃它的缓存。
 * @param inspector C/C++ 配置检查器，判断同步后是否还有其他配置。
 * @param logger 日志。
 */
export function registerCIntelliSense(
  context: vscode.ExtensionContext,
  sync: IntelliSenseSyncService,
  projects: ProjectInfoProvider,
  inspector: CppConfigurationInspector,
  logger: Logger
): void {
  const timers = new Map<string, NodeJS.Timeout>();
  const prompted = new Set<string>();

  /** 同步成功的项目里还有其他配置时，提示去选 fcdev 配置；每个项目每次会话只提示一次。 */
  const promptSelection = async (results: readonly SyncResult[]): Promise<void> => {
    if (!isPromptEnabled() || vscode.extensions.getExtension(CPPTOOLS_EXTENSION_ID) === undefined) return;
    const roots: string[] = [];
    for (const result of results) {
      if (!result.ok || prompted.has(result.root)) continue;
      if (await inspector.hasOtherConfigurations(result.root)) {
        prompted.add(result.root);
        roots.push(result.root);
      }
    }
    if (roots.length === 0) return;
    const names = roots.map((root) => path.basename(root)).join('、');
    const choice = await vscode.window.showInformationMessage(
      `${names} 的 C/C++ 配置里除 fcdev 外还有其他配置。C/C++ 扩展用哪个配置由你选择，选 fcdev 才会使用 cc65 的头文件。`,
      SELECT_BUTTON
    );
    if (choice === SELECT_BUTTON) await vscode.commands.executeCommand(SELECT_CONFIGURATION_COMMAND);
  };

  /** 提示出错只记日志，不影响同步流程。 */
  const promptQuietly = (results: readonly SyncResult[]): void => {
    void promptSelection(results).catch((error: unknown) => logger.error('提示选择 C/C++ 配置出错', error));
  };

  /** 自动同步：只记日志。 */
  const syncQuietly = async (task: () => Promise<readonly SyncResult[]>): Promise<void> => {
    try {
      const results = await task();
      logResults(logger, results);
      promptQuietly(results);
    } catch (error) {
      logger.error('自动同步 C/C++ 配置出错', error);
    }
  };

  /** 手动同步：带进度，结束后弹出结果。 */
  const syncWithNotice = async (): Promise<void> => {
    try {
      const results = await vscode.window.withProgress(
        { location: vscode.ProgressLocation.Notification, title: PROGRESS_TITLE },
        () => sync.syncWorkspace()
      );
      logResults(logger, results);
      if (results.length === 0) {
        void vscode.window.showInformationMessage('工作区里没有 fcdev 项目（找不到 fcdev.json）');
        return;
      }
      const failures = results.filter((result) => !result.ok);
      if (failures.length > 0) {
        void vscode.window.showWarningMessage(failures.map((result) => `${path.basename(result.root)}：${result.message}`).join('\n'));
      } else {
        void vscode.window.showInformationMessage(`已同步 ${results.length} 个 fcdev 项目的 C/C++ 配置`);
      }
      promptQuietly(results);
    } catch (error) {
      logger.error('同步 C/C++ 配置出错', error);
      void vscode.window.showErrorMessage('同步 C/C++ 配置时出错，详情见“输出”面板里的 fcdev');
    }
  };

  /** fcdev.json 变化后：丢弃项目信息缓存，开启自动同步时重新同步该项目。 */
  const onConfigChanged = (uri: vscode.Uri): void => {
    const root = path.dirname(uri.fsPath);
    clearTimeout(timers.get(root));
    timers.set(
      root,
      setTimeout(() => {
        timers.delete(root);
        projects.invalidate(root);
        if (isAutoSyncEnabled()) void syncQuietly(async () => [await sync.syncProject(root)]);
      }, DEBOUNCE_MS)
    );
  };

  const watcher = vscode.workspace.createFileSystemWatcher(CONFIG_FILES);
  context.subscriptions.push(
    vscode.commands.registerCommand(SYNC_COMMAND, syncWithNotice),
    watcher,
    watcher.onDidChange(onConfigChanged),
    watcher.onDidCreate(onConfigChanged),
    vscode.workspace.onDidChangeWorkspaceFolders(() => {
      if (isAutoSyncEnabled()) void syncQuietly(() => sync.syncWorkspace());
    }),
    new vscode.Disposable(() => timers.forEach((timer) => clearTimeout(timer)))
  );

  if (isAutoSyncEnabled()) void syncQuietly(() => sync.syncWorkspace());
}
