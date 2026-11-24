// ------------------------------------------------------------------------
// 名称：debug-commands.ts
// 说明：调试服务的命令：调用调试请求、查看请求说明、保存调试截图、监视调试事件、显示调试服务地址；
//       结果写到“FC Dev 调试”输出通道。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-22
// 备注：先按设置里的调试端口找 FC Dev 启动的 Ember（用户数据目录按端口区分），找不到端点文件时再找
//       Ember 自己的用户数据目录；请求名与参数说明来自随扩展发布的接口描述文档。
// ------------------------------------------------------------------------

import * as path from 'node:path';
import * as vscode from 'vscode';
import type { DebugConnector, OpenRpcDocumentSource } from '../../contracts/debug-client/debug-connector';
import type { DebugSession } from '../../contracts/debug-client/debug-session';
import type { RequestMap } from '../../contracts/debug-client/generated/api';
import type { EmulatorSettings } from '../../contracts/emulator/emulator-settings';
import type { EmulatorProcessLauncher } from '../../contracts/emulator/process-launcher';
import type { FileSystem } from '../../contracts/files/file-system';
import type { Logger } from '../../platform/logging/logger';
import { CallArgumentError, parseCallParams, printable, splitArguments } from '../../shared/debug-client/call-arguments';
import { EmberConnectionError, EmberRequestError } from '../../shared/debug-client/errors';
import { describeEvent, describeMethod, listMethods } from '../../shared/debug-client/method-catalog';

/** 命令的标识，与 package.json 里声明的一致。 */
export const DEBUG_COMMANDS = {
  call: 'fcdev.debugCall',
  describe: 'fcdev.debugDescribe',
  screenshot: 'fcdev.debugScreenshot',
  watch: 'fcdev.debugWatch',
  endpoint: 'fcdev.debugEndpoint'
} as const;

/** 调试客户端的名字，显示在 Ember 的连接列表里。 */
const CLIENT_NAME = 'fcdev';

/** 输出通道的名字。 */
const CHANNEL_NAME = 'FC Dev 调试';

/** 命令要用到的能力。 */
export interface DebugCommandDependencies {
  /** 连接调试服务。 */
  readonly connector: DebugConnector;
  /** 接口描述文档的来源。 */
  readonly document: OpenRpcDocumentSource;
  /** 模拟器设置，取调试端口。 */
  readonly settings: EmulatorSettings;
  /** 进程启动，取调试用的用户数据目录。 */
  readonly processes: EmulatorProcessLauncher;
  /** 文件系统，保存截图与二进制数据。 */
  readonly files: FileSystem;
  /** 日志。 */
  readonly logger: Logger;
}

/** 两位数字，不足补 0。 */
function twoDigits(value: number): string {
  return String(value).padStart(2, '0');
}

/** 默认的截图文件名：带本地时间。 */
function defaultScreenshotName(now: Date): string {
  const stamp = `${now.getFullYear()}${twoDigits(now.getMonth() + 1)}${twoDigits(now.getDate())}-${twoDigits(now.getHours())}${twoDigits(now.getMinutes())}${twoDigits(now.getSeconds())}`;
  return `ember-screenshot-${stamp}.png`;
}

/** 把令牌打码。 */
function maskToken(token: string): string {
  return `${token.slice(0, 4)}…（共 ${token.length} 个字符）`;
}

/** 错误转成一行说明。 */
function describeError(error: unknown): string {
  if (error instanceof EmberRequestError) return `错误 ${error.code}：${error.message}`;
  if (error instanceof Error) return error.message;
  return String(error);
}

/** 结果里的二进制 data。 */
function binaryData(result: unknown): Uint8Array | undefined {
  const data = (result as { data?: unknown } | null)?.data;
  return data instanceof Uint8Array ? data : undefined;
}

/**
 * 注册调试服务的命令。
 * @param context 扩展上下文，注册的资源随扩展停用而释放。
 * @param dependencies 命令要用到的能力。
 */
export function registerDebugCommands(context: vscode.ExtensionContext, dependencies: DebugCommandDependencies): void {
  const { connector, document, settings, processes, files, logger } = dependencies;
  const output = vscode.window.createOutputChannel(CHANNEL_NAME);

  /**
   * 连接 Ember：先找 FC Dev 启动的（按设置里的端口），没有端点文件再找 Ember 自己的用户数据目录。
   */
  const connect = async (): Promise<DebugSession> => {
    const options = { clientName: CLIENT_NAME };
    try {
      return await connector.connectToEmber({ ...options, userDataDirectory: processes.userDataDirectory(settings.debugPort()) });
    } catch (error) {
      if (!(error instanceof EmberConnectionError) || error.kind !== 'endpoint') throw error;
      return connector.connectToEmber(options);
    }
  };

  /** 选一个请求。 */
  const pickMethod = async (title: string) => {
    const methods = listMethods(document.load());
    const picked = await vscode.window.showQuickPick(
      methods.map((method) => ({ label: method.name, description: method.summary, method })),
      { title, matchOnDescription: true, placeHolder: '输入请求名或简介里的关键词' }
    );
    return picked?.method;
  };

  /** 把二进制数据保存成文件，问用户存哪里。 */
  const offerSave = async (data: Uint8Array): Promise<void> => {
    const save = '保存二进制数据…';
    if ((await vscode.window.showInformationMessage(`结果里有二进制数据（${data.byteLength} 字节）`, save)) !== save) return;
    const target = await vscode.window.showSaveDialog({ saveLabel: '保存' });
    if (target !== undefined) files.writeBytes(target.fsPath, data);
  };

  /** 调用调试请求。 */
  const call = async (): Promise<void> => {
    const method = await pickMethod('调用调试请求');
    if (method === undefined) return;
    const required = method.params.filter((param) => param.required).map((param) => param.name).join('、');
    const text = await vscode.window.showInputBox({
      title: `调用 ${method.name}`,
      prompt: `参数写成 key=value（值按 JSON 解析，不是 JSON 就当字符串；0x8000 与 $8000 是十六进制数），或一个 JSON 对象${required === '' ? '' : `；必填：${required}`}`,
      placeHolder: method.params.length === 0 ? '这个请求没有参数，直接回车' : '例如 frames=60 wait=true'
    });
    if (text === undefined) return;
    const params = parseCallParams(splitArguments(text));
    const client = await connect();
    try {
      const result = await client.request(method.name as keyof RequestMap, params as never);
      output.appendLine(`${method.name} ${text}`.trimEnd());
      output.appendLine(JSON.stringify(printable(result), null, 2));
      output.show(true);
      const data = binaryData(result);
      if (data !== undefined) await offerSave(data);
    } finally {
      client.close();
    }
  };

  /** 查看请求或事件的说明。 */
  const describe = async (): Promise<void> => {
    const loaded = document.load();
    const items = [
      ...loaded.methods.map((method) => ({ label: method.name, description: method.summary, text: describeMethod(method) })),
      ...loaded['x-events'].map((event) => ({ label: event.name, description: `事件：${event.description}`, text: describeEvent(event) }))
    ];
    const picked = await vscode.window.showQuickPick(items, { title: '查看调试请求说明', matchOnDescription: true, placeHolder: '输入请求或事件名' });
    if (picked === undefined) return;
    output.appendLine(picked.text);
    output.appendLine('');
    output.show(true);
  };

  /** 保存当前画面。 */
  const screenshot = async (): Promise<void> => {
    const folder = vscode.workspace.workspaceFolders?.[0]?.uri.fsPath;
    const defaultName = defaultScreenshotName(new Date());
    const target = await vscode.window.showSaveDialog({
      title: '保存调试截图',
      defaultUri: vscode.Uri.file(folder === undefined ? defaultName : path.join(folder, defaultName)),
      filters: { PNG: ['png'] }
    });
    if (target === undefined) return;
    const client = await connect();
    try {
      const result = await client.request('output.screenshot', { format: 'png' });
      if (result.data === undefined) throw new EmberConnectionError('protocol', '截图的结果里没有图像数据');
      files.writeBytes(target.fsPath, result.data);
      void vscode.window.showInformationMessage(`已保存截图：${target.fsPath}（${result.width}×${result.height}）`);
    } finally {
      client.close();
    }
  };

  /** 订阅事件并写到输出通道，直到用户取消或连接关闭。 */
  const watch = async (): Promise<void> => {
    const text = await vscode.window.showInputBox({
      title: '监视调试事件',
      prompt: '要订阅的事件，空白分隔：事件名（如 runtime.runFinished）、模块名加 .*（如 runtime.*）或 *（全部）',
      value: '*'
    });
    if (text === undefined) return;
    const patterns = text.split(/\s+/).filter((pattern) => pattern !== '');
    const client = await connect();
    output.appendLine(`开始监视：${patterns.join(' ')}`);
    output.show(true);
    try {
      await vscode.window.withProgress({ location: vscode.ProgressLocation.Notification, title: '正在监视调试事件', cancellable: true }, async (_progress, token) => {
        client.onAny((event, data) => output.appendLine(JSON.stringify({ event, data: printable(data) })));
        await client.subscribe(patterns.length === 0 ? ['*'] : patterns);
        await new Promise<void>((resolve) => {
          token.onCancellationRequested(() => resolve());
          void client.closed.then(() => resolve());
        });
      });
    } finally {
      client.close();
      output.appendLine('已停止监视');
    }
  };

  /** 显示正在运行的 Ember 的调试服务地址。 */
  const endpoint = (): void => {
    const options = { userDataDirectory: processes.userDataDirectory(settings.debugPort()) };
    let found;
    try {
      found = connector.readEndpoint(options);
    } catch (error) {
      if (!(error instanceof EmberConnectionError) || error.kind !== 'endpoint') throw error;
      found = connector.readEndpoint();
    }
    output.appendLine(JSON.stringify({ ...found, token: maskToken(found.token) }, null, 2));
    output.show(true);
  };

  /** 命令出错时提示：参数写法与连接错误直接给原因，其余记日志。 */
  const guarded = (name: string, action: () => Promise<void> | void) => async (): Promise<void> => {
    try {
      await action();
    } catch (error) {
      if (error instanceof CallArgumentError || error instanceof EmberConnectionError || error instanceof EmberRequestError) {
        void vscode.window.showErrorMessage(`${name}：${describeError(error)}`);
        return;
      }
      logger.error(`${name}出错`, error);
      void vscode.window.showErrorMessage(`${name}时出错，详情见“输出”面板里的 FC Dev`);
    }
  };

  context.subscriptions.push(
    output,
    vscode.commands.registerCommand(DEBUG_COMMANDS.call, guarded('调用调试请求', call)),
    vscode.commands.registerCommand(DEBUG_COMMANDS.describe, guarded('查看调试请求说明', describe)),
    vscode.commands.registerCommand(DEBUG_COMMANDS.screenshot, guarded('保存调试截图', screenshot)),
    vscode.commands.registerCommand(DEBUG_COMMANDS.watch, guarded('监视调试事件', watch)),
    vscode.commands.registerCommand(DEBUG_COMMANDS.endpoint, guarded('显示调试服务地址', endpoint))
  );
}
