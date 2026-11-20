// ------------------------------------------------------------------------
// 名称：extension.ts
// 说明：扩展的组装根：创建全部具体实现并按接口注入，注册汇编智能感知、C 智能感知与反汇编工具。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-10
// 备注：只负责创建与连接，不写业务判断；替换任何一项实现只需修改这里。
// ------------------------------------------------------------------------

import * as vscode from 'vscode';
import { registerAssemblyIntelliSense } from '../entry/assembly/assembly-registration';
import { registerCIntelliSense } from '../entry/c-intellisense/c-intellisense-commands';
import { registerDisassemblyTools } from '../entry/disassembly/disassembly-tool-registration';
import { WorkspaceAssemblySourceStore } from '../infra/assembly/workspace-assembly-source-store';
import { FileCppConfigurationInspector } from '../infra/c-intellisense/file-cpp-configuration-inspector';
import { FcdevBuildComparer } from '../infra/disassembly/fcdev-build-comparer';
import { WorkspaceDisasmRunner } from '../infra/disassembly/workspace-disasm-runner';
import { FcdevCliRunner } from '../infra/project/fcdev-cli-runner';
import { FcdevProjectInfoProvider } from '../infra/project/fcdev-project-info-provider';
import { WorkspaceFcdevProjectLocator } from '../infra/project/workspace-fcdev-project-locator';
import { AssemblySymbolResolver } from '../logic/assembly/assembly-symbol-resolver';
import { ResolvingAssemblyIntelliSenseService } from '../logic/assembly/resolving-assembly-intellisense-service';
import { RunnerIntelliSenseSyncService } from '../logic/c-intellisense/runner-intellisense-sync-service';
import { OutputChannelLogger } from '../platform/logging/output-channel-logger';

/**
 * 激活扩展：创建各项实现并注册汇编智能感知、C 智能感知与反汇编工具。
 * @param context VS Code 提供的扩展上下文，登记的资源随扩展停用而释放。
 */
export function activate(context: vscode.ExtensionContext): void {
  const logger = new OutputChannelLogger();
  const runner = new FcdevCliRunner();
  const projects = new FcdevProjectInfoProvider(runner, logger);
  const assembly = new ResolvingAssemblyIntelliSenseService(
    new AssemblySymbolResolver(new WorkspaceAssemblySourceStore(), projects)
  );
  const cSync = new RunnerIntelliSenseSyncService(runner, new WorkspaceFcdevProjectLocator());

  context.subscriptions.push(logger);
  registerAssemblyIntelliSense(context, assembly, logger);
  registerCIntelliSense(context, cSync, projects, new FileCppConfigurationInspector(), logger);
  registerDisassemblyTools(context, { runner: new WorkspaceDisasmRunner(logger), comparer: new FcdevBuildComparer(runner), logger });
}

/** 停用扩展：登记在上下文里的资源由 VS Code 释放，这里没有额外要做的事。 */
export function deactivate(): void {}
