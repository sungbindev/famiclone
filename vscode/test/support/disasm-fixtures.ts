// ------------------------------------------------------------------------
// 名称：disasm-fixtures.ts
// 说明：反汇编测试的公共工具：用扩展目录里的目标与工具链组装真实的反汇编环境，提供运行操作、构建
//       程序、运行汇编链接工具的快捷函数。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-22
// 备注：真实回读校验需要扩展自带的工具链；没有时用 HAS_TOOLCHAIN 跳过相关用例。
// ------------------------------------------------------------------------

import * as path from 'node:path';
import type { DisasmEnvironment } from '../../src/contracts/disassembly/disasm-environment';
import type { BuildInput, BuildReporter } from '../../src/contracts/build/build-service';
import type { ProcessResult, ToolName, Toolchain } from '../../src/contracts/toolchain/toolchain';
import { NodeFileSystem } from '../../src/infra/files/node-file-system';
import { createExtensionResources } from '../../src/infra/resources/extension-resources-factory';
import { BundledToolchainLocator } from '../../src/infra/toolchain/bundled-toolchain-locator';
import { ProcessToolRunner } from '../../src/infra/toolchain/process-tool-runner';
import { CoreDisasmOperations } from '../../src/logic/disassembly/core-disasm-operations';
import { FileTargetProfileReader } from '../../src/logic/targets/file-target-profile-reader';
import { createRealBuildEnvironment, EXTENSION_DIRECTORY, HAS_TOOLCHAIN } from './build-fixtures';

export { HAS_TOOLCHAIN };

/** 扩展自带的目标定义目录。 */
export const BUILTIN_TARGETS_DIR = path.join(EXTENSION_DIRECTORY, 'targets');

/** 反汇编操作的结果：测试里按字段取值，字段由各操作约定。 */
export type OperationResult = Record<string, any>;

const resources = createExtensionResources(EXTENSION_DIRECTORY);
const fileSystem = new NodeFileSystem();
const toolchainLocator = new BundledToolchainLocator(fileSystem, resources);
const toolRunner = new ProcessToolRunner();

/** 真实的反汇编环境。 */
export const DISASM_ENVIRONMENT: DisasmEnvironment = {
  files: fileSystem,
  profiles: new FileTargetProfileReader(fileSystem, resources),
  toolchain: toolchainLocator,
  tools: toolRunner
};

/** 用真实环境运行操作的对象。 */
export const DISASM = new CoreDisasmOperations(DISASM_ENVIRONMENT);

/**
 * 运行一个反汇编操作。
 * @param name 操作名。
 * @param params 参数。
 * @returns 操作的结果。
 */
export async function runOperation(name: string, params: Record<string, unknown>): Promise<OperationResult> {
  return (await DISASM.run(name, params)) as OperationResult;
}

/**
 * 构建一个路径，产物供反汇编。
 * @param input 构建输入。
 * @param reporter 进度汇报。
 * @returns 构建结果。
 */
export async function buildPath(input: BuildInput, reporter?: BuildReporter) {
  return createRealBuildEnvironment().service.build(input, reporter);
}

/**
 * 取扩展自带的工具链。
 * @returns 工具链。
 * @throws FcdevError 工具链没有组装。
 */
export function locateTestToolchain(): Toolchain {
  return toolchainLocator.locate();
}

/**
 * 运行工具链里的一个工具。
 * @param toolchain 工具链。
 * @param tool 工具名。
 * @param args 参数。
 * @param cwd 工作目录。
 * @returns 运行结果。
 */
export function runTool(toolchain: Toolchain, tool: ToolName, args: readonly string[], cwd?: string): Promise<ProcessResult> {
  return toolRunner.run(toolchain, tool, args, cwd);
}
