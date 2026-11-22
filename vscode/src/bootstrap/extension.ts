// ------------------------------------------------------------------------
// 名称：extension.ts
// 说明：扩展的组装根：创建全部具体实现并按接口注入，注册汇编智能感知、C 智能感知、构建任务与命令、
//       新建项目、运行与调试、使用文档、C 接口查询与语言模型工具。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-10
// 备注：只负责创建与连接，不写业务判断；替换任何一项实现只需修改这里。
// ------------------------------------------------------------------------

import * as vscode from 'vscode';
import type { EmulatorEnvironment } from '../contracts/emulator/emulator-environment';
import type { EmulatorOperations } from '../contracts/emulator/emulator-operations';
import { registerAssemblyIntelliSense } from '../entry/assembly/assembly-registration';
import { registerBuildCommands } from '../entry/build/build-commands';
import { registerBuildTasks } from '../entry/build/build-task-provider';
import { registerCApiCommands } from '../entry/c-api/c-api-commands';
import { registerCIntelliSense } from '../entry/c-intellisense/c-intellisense-commands';
import { registerDebugCommands } from '../entry/debug-client/debug-commands';
import { registerDisassemblyCommands } from '../entry/disassembly/disassembly-commands';
import { registerDisassemblyTools } from '../entry/disassembly/disassembly-tool-registration';
import { WorkspaceEmulatorSettings } from '../entry/emulator/workspace-emulator-settings';
import { registerFcdevTools } from '../entry/fcdev-tools/fcdev-tool-registration';
import { registerScaffoldCommands } from '../entry/scaffold/scaffold-commands';
import { registerUsageDocsCommands } from '../entry/usage-docs/usage-docs-commands';
import { WorkspaceAssemblySourceStore } from '../infra/assembly/workspace-assembly-source-store';
import { FileCppConfigurationInspector } from '../infra/c-intellisense/file-cpp-configuration-inspector';
import { FileEndpointResolver } from '../infra/debug-client/endpoint-file';
import { FileOpenRpcDocumentSource } from '../infra/debug-client/file-open-rpc-document-source';
import { openWebSocket } from '../infra/debug-client/web-socket-transport';
import { NodeEmulatorProcessLauncher } from '../infra/emulator/node-process-launcher';
import { NodeFileSystem } from '../infra/files/node-file-system';
import { WorkspaceFcdevProjectLocator } from '../infra/project/workspace-fcdev-project-locator';
import { createExtensionResources } from '../infra/resources/extension-resources-factory';
import { BundledToolchainLocator } from '../infra/toolchain/bundled-toolchain-locator';
import { ProcessToolRunner } from '../infra/toolchain/process-tool-runner';
import { AssemblySymbolResolver } from '../logic/assembly/assembly-symbol-resolver';
import { ResolvingAssemblyIntelliSenseService } from '../logic/assembly/resolving-assembly-intellisense-service';
import { DefaultBuildService } from '../logic/build/default-build-service';
import { TargetCApiService } from '../logic/c-api/target-c-api-service';
import { FileIntelliSenseSyncService } from '../logic/c-intellisense/file-intellisense-sync-service';
import { EmberConnector } from '../logic/debug-client/ember-connector';
import { BuildServiceComparer } from '../logic/disassembly/build-service-comparer';
import { CoreDisasmOperations } from '../logic/disassembly/core-disasm-operations';
import { CoreEmulatorOperations } from '../logic/emulator/core-emulator-operations';
import { EmulatorProgramRunner } from '../logic/emulator/emulator-program-runner';
import { DefaultProjectDescriber } from '../logic/project/default-project-describer';
import { DescribingProjectInfoProvider } from '../logic/project/describing-project-info-provider';
import { FileProjectConfigReader } from '../logic/project/file-project-config-reader';
import { FileScaffoldService } from '../logic/scaffold/file-scaffold-service';
import { FileTargetRepository } from '../logic/targets/file-target-repository';
import { FileTargetProfileReader } from '../logic/targets/file-target-profile-reader';
import { FileUsageDocsService } from '../logic/usage-docs/file-usage-docs-service';
import { OutputChannelLogger } from '../platform/logging/output-channel-logger';

/** 启用中的模拟器操作，停用时用它收尾。 */
let emulatorOperations: EmulatorOperations | undefined;

/**
 * 激活扩展：创建各项实现并注册全部入口。
 * @param context VS Code 提供的扩展上下文，登记的资源随扩展停用而释放。
 */
export function activate(context: vscode.ExtensionContext): void {
  const logger = new OutputChannelLogger();

  const resources = createExtensionResources(context.extensionPath);
  const fileSystem = new NodeFileSystem();
  const configReader = new FileProjectConfigReader(fileSystem);
  const targets = new FileTargetRepository(fileSystem, resources);
  const toolchainLocator = new BundledToolchainLocator(fileSystem, resources);
  const toolRunner = new ProcessToolRunner();
  const buildService = new DefaultBuildService({
    fileSystem,
    configReader,
    targets,
    toolchainLocator,
    toolRunner,
    resources
  });
  const describer = new DefaultProjectDescriber(configReader, targets, toolchainLocator);
  const locator = new WorkspaceFcdevProjectLocator();
  const projects = new DescribingProjectInfoProvider(fileSystem, describer, logger);
  const assembly = new ResolvingAssemblyIntelliSenseService(
    new AssemblySymbolResolver(new WorkspaceAssemblySourceStore(), projects)
  );
  const cSync = new FileIntelliSenseSyncService(fileSystem, describer, locator);
  const scaffold = new FileScaffoldService(fileSystem, resources);
  const docs = new FileUsageDocsService(fileSystem, resources);
  const api = new TargetCApiService(fileSystem, configReader, targets);
  const profiles = new FileTargetProfileReader(fileSystem, resources);
  const settings = new WorkspaceEmulatorSettings();
  const processes = new NodeEmulatorProcessLauncher();
  const connector = new EmberConnector(openWebSocket, new FileEndpointResolver());
  const emulatorEnvironment: EmulatorEnvironment = { files: fileSystem, profiles, connector, settings, processes };
  const emulator = new CoreEmulatorOperations(emulatorEnvironment);
  emulatorOperations = emulator;
  const runner = new EmulatorProgramRunner(buildService, emulatorEnvironment);

  context.subscriptions.push(logger);
  registerAssemblyIntelliSense(context, assembly, logger);
  registerCIntelliSense(context, cSync, projects, new FileCppConfigurationInspector(), logger);
  const tasks = registerBuildTasks(context, { buildService, runner, fileSystem, locator, logger });
  registerBuildCommands(context, { tasks, fileSystem, locator, describer, settings, logger });
  registerScaffoldCommands(context, scaffold, logger);
  registerUsageDocsCommands(context, docs, logger);
  registerCApiCommands(context, api, logger);
  registerFcdevTools(context, { build: buildService, runner, scaffold, describer, api, docs }, logger);
  registerDebugCommands(context, {
    connector,
    document: new FileOpenRpcDocumentSource(fileSystem, resources),
    settings,
    processes,
    files: fileSystem,
    logger
  });
  const operations = new CoreDisasmOperations({
    files: fileSystem,
    profiles,
    toolchain: toolchainLocator,
    tools: toolRunner
  });
  registerDisassemblyCommands(context, operations, logger);
  registerDisassemblyTools(context, {
    operations,
    emulator,
    comparer: new BuildServiceComparer(fileSystem, buildService),
    logger
  });
}

/** 停用扩展：关闭模拟器会话并结束工具启动的 Ember，其余登记在上下文里的资源由 VS Code 释放。 */
export function deactivate(): Promise<void> | undefined {
  return emulatorOperations?.shutdown();
}
