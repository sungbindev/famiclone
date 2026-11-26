// ------------------------------------------------------------------------
// 名称：extension.mjs
// 说明：仓库里的开发脚本（fbasic 工具、机型探测脚本等）使用 VS Code 扩展能力的公共入口：定位工具链并运行工具、取指令表、启动 Ember 并连接调试服务、把程序放进软盘镜像。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-22
// 备注：直接引用扩展的编译产物 vscode/out，使用前要先在 vscode 目录运行 npm run build；Ember 的位置取环境变量 EMBER_EXE（脚本不在 VS Code 里运行，读不到扩展设置 fcdev.emulator.path）；
//       只结束自己启动的 Ember，不按进程名结束任何进程。
// ------------------------------------------------------------------------

import path from 'node:path';
import { fileURLToPath } from 'node:url';
import { NodeEmulatorProcessLauncher } from '../vscode/out/infra/emulator/node-process-launcher.js';
import { FileEndpointResolver } from '../vscode/out/infra/debug-client/endpoint-file.js';
import { openWebSocket } from '../vscode/out/infra/debug-client/web-socket-transport.js';
import { NodeFileSystem } from '../vscode/out/infra/files/node-file-system.js';
import { createExtensionResources } from '../vscode/out/infra/resources/extension-resources-factory.js';
import { BundledToolchainLocator } from '../vscode/out/infra/toolchain/bundled-toolchain-locator.js';
import { ProcessToolRunner } from '../vscode/out/infra/toolchain/process-tool-runner.js';
import { DefaultBuildService } from '../vscode/out/logic/build/default-build-service.js';
import { EmberConnector } from '../vscode/out/logic/debug-client/ember-connector.js';
import { launchEmulator as launchWithEnvironment } from '../vscode/out/logic/emulator/emulator-launcher.js';
import { FileProjectConfigReader } from '../vscode/out/logic/project/file-project-config-reader.js';
import { FileTargetRepository } from '../vscode/out/logic/targets/file-target-repository.js';
import { enterProgram, runFrames, tapKey } from '../vscode/out/logic/emulator/machine-session.js';
import { packageDisk } from '../vscode/out/shared/packaging/program-packager.js';

export { OPCODE_TABLE } from '../vscode/out/shared/disassembly/cpu/opcodes.js';
export { enterProgram, packageDisk, runFrames, tapKey };

/** 扩展目录。 */
const EXTENSION_DIRECTORY = path.resolve(path.dirname(fileURLToPath(import.meta.url)), '..', 'vscode');

const files = new NodeFileSystem();
const processes = new NodeEmulatorProcessLauncher();
const connector = new EmberConnector(openWebSocket, new FileEndpointResolver());
const toolRunner = new ProcessToolRunner();

/**
 * 定位扩展自带的工具链。
 * @returns 工具链。
 * @throws {Error} 工具链没有组装（先运行扩展的 npm run build:toolchain）。
 */
export function locateToolchain() {
  return new BundledToolchainLocator(files, createExtensionResources(EXTENSION_DIRECTORY)).locate();
}

/**
 * 运行工具链里的一个工具。
 * @param {object} toolchain 工具链。
 * @param {string} tool 工具名：cc65、ca65、ld65、ar65。
 * @param {readonly string[]} args 参数。
 * @param {string} [cwd] 工作目录；省略时用当前进程的。
 * @returns {Promise<{code: number, stdout: string, stderr: string}>} 运行结果。
 */
export function runTool(toolchain, tool, args, cwd) {
  return toolRunner.run(toolchain, tool, args, cwd);
}

/**
 * 取 Ember 可执行文件的路径：环境变量 EMBER_EXE。
 * @returns {string} 路径。
 * @throws {Error} 没有设置。
 */
export function emberExecutable() {
  const executable = process.env.EMBER_EXE;
  if (executable === undefined || executable === '') throw new Error('没有设置环境变量 EMBER_EXE（Ember 可执行文件的路径）');
  return executable;
}

/**
 * 调试模式下 Ember 使用的用户数据目录（每个端口一个）。
 * @param {number} port 调试端口。
 * @returns {string} 目录。
 */
export function debugUserDataDirectory(port) {
  return processes.userDataDirectory(port);
}

/**
 * 启动 Ember，参数同扩展里的 launchEmulator（executable、romFile、sb2000、debug、port）。
 * @param {object} input 启动参数。
 * @returns {Promise<{pid: number|undefined, client: object|null}>} 进程号；调试模式下还有已连接的调试会话，调用者负责关闭。
 */
export function launchEmulator(input) {
  return launchWithEnvironment({ files, connector, processes }, input);
}

/**
 * 连接已运行的 Ember（找端点文件）。
 * @param {object} [options] 连接选项：userDataDirectory、url、token、clientName 等。
 * @returns {Promise<object>} 调试会话，调用者负责关闭。
 */
export function connectToEmber(options = {}) {
  return connector.connectToEmber(options);
}

/**
 * 构建一个项目目录或源文件（与任务“构建项目”同一套逻辑），工具输出原样写到标准输出。
 * @param {string} pathArg 项目的源目录或源文件，相对 cwd。
 * @param {string} cwd 起始目录。
 * @returns {Promise<{programName: string, outputFile: string, size: number, compiled: number, reused: number}>} 构建结果。
 * @throws {Error} 路径不存在、没有目标或编译链接失败。
 */
export async function buildProject(pathArg, cwd) {
  const resources = createExtensionResources(EXTENSION_DIRECTORY);
  const service = new DefaultBuildService({
    fileSystem: files,
    configReader: new FileProjectConfigReader(files),
    targets: new FileTargetRepository(files, resources),
    toolchainLocator: new BundledToolchainLocator(files, resources),
    toolRunner,
    resources
  });
  const outcome = await service.build({ pathArg, cwd }, { log: (message) => console.log(message), write: (text) => process.stdout.write(text) });
  return outcome.result;
}
