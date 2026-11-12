// ------------------------------------------------------------------------
// 名称：assembly-registration.ts
// 说明：注册汇编语言的智能感知：四个提供者，以及监视汇编文件变化使缓存失效的文件监视器。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-10
// 备注：提供者只对磁盘上的文件（file 方案）生效；补全在输入 . 和 @ 时自动触发。
// ------------------------------------------------------------------------

import * as vscode from 'vscode';
import type { AssemblyIntelliSenseService } from '../../contracts/assembly/assembly-intellisense-service';
import type { Logger } from '../../platform/logging/logger';
import { ASSEMBLY_LANGUAGE_ID } from './assembly-conversion';
import {
  AssemblyCompletionProvider,
  AssemblyDefinitionProvider,
  AssemblyDocumentSymbolProvider,
  AssemblyHoverProvider
} from './assembly-providers';

/** 汇编文件的监视模式。 */
const ASSEMBLY_FILES = '**/*.{s,asm,inc}';

/** 自动触发补全的字符：汇编指令的前导点与局部标签的 @。 */
const COMPLETION_TRIGGERS = ['.', '@'];

/**
 * 注册汇编语言的智能感知。
 * @param context 扩展上下文，注册的资源随扩展停用而释放。
 * @param service 汇编智能感知服务。
 * @param logger 日志。
 */
export function registerAssemblyIntelliSense(
  context: vscode.ExtensionContext,
  service: AssemblyIntelliSenseService,
  logger: Logger
): void {
  const selector: vscode.DocumentSelector = { language: ASSEMBLY_LANGUAGE_ID, scheme: 'file' };
  const watcher = vscode.workspace.createFileSystemWatcher(ASSEMBLY_FILES);
  const invalidate = (uri: vscode.Uri): void => service.invalidate(uri.fsPath);

  context.subscriptions.push(
    vscode.languages.registerHoverProvider(selector, new AssemblyHoverProvider(service, logger)),
    vscode.languages.registerCompletionItemProvider(selector, new AssemblyCompletionProvider(service, logger), ...COMPLETION_TRIGGERS),
    vscode.languages.registerDefinitionProvider(selector, new AssemblyDefinitionProvider(service, logger)),
    vscode.languages.registerDocumentSymbolProvider(selector, new AssemblyDocumentSymbolProvider(service, logger)),
    watcher,
    watcher.onDidChange(invalidate),
    watcher.onDidCreate(invalidate),
    watcher.onDidDelete(invalidate)
  );
}
