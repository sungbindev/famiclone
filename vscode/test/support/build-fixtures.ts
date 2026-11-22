// ------------------------------------------------------------------------
// 名称：build-fixtures.ts
// 说明：构建相关测试的公共工具：临时目录、写文件、组装真实的构建服务、探测本机工具链。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-22
// 备注：真实构建用扩展目录里的工具链与目标；工具链没有组装时，集成用例整体跳过。
// ------------------------------------------------------------------------

import { mkdirSync, mkdtempSync, rmSync, writeFileSync } from 'node:fs';
import { tmpdir } from 'node:os';
import * as path from 'node:path';
import type { BuildReporter } from '../../src/contracts/build/build-service';
import type { ExtensionResources } from '../../src/contracts/resources/extension-resources';
import { NodeFileSystem } from '../../src/infra/files/node-file-system';
import { createExtensionResources } from '../../src/infra/resources/extension-resources-factory';
import { BundledToolchainLocator } from '../../src/infra/toolchain/bundled-toolchain-locator';
import { ProcessToolRunner } from '../../src/infra/toolchain/process-tool-runner';
import { DefaultBuildService } from '../../src/logic/build/default-build-service';
import { FileProjectConfigReader } from '../../src/logic/project/file-project-config-reader';
import { FileTargetRepository } from '../../src/logic/targets/file-target-repository';

/** 扩展目录（含 package.json、targets、toolchain）。 */
export const EXTENSION_DIRECTORY = path.resolve(__dirname, '..', '..');

/** 临时目录与清理函数。 */
export interface TempDir {
  /** 目录的绝对路径。 */
  readonly dir: string;
  /** 删除目录。 */
  cleanup(): void;
}

/**
 * 创建临时目录。
 * @returns 目录与清理函数。
 */
export function createTempDir(): TempDir {
  const dir = mkdtempSync(path.join(tmpdir(), 'fcdev-test-'));
  return { dir, cleanup: () => rmSync(dir, { recursive: true, force: true }) };
}

/**
 * 在目录下写文件，自动创建上级目录。
 * @param root 根目录。
 * @param relativePath 相对路径。
 * @param content 内容。
 * @returns 文件的绝对路径。
 */
export function writeTestFile(root: string, relativePath: string, content: string | Uint8Array): string {
  const full = path.join(root, relativePath);
  mkdirSync(path.dirname(full), { recursive: true });
  writeFileSync(full, content);
  return full;
}

/**
 * 用一个目录作为全部资源目录的假资源位置，需要某一项时用 overrides 覆盖。
 * @param root 目录。
 * @param overrides 要覆盖的字段。
 * @returns 资源位置。
 */
export function fakeResources(root: string, overrides: Partial<ExtensionResources> = {}): ExtensionResources {
  return {
    version: '0.0.0',
    extensionDirectory: root,
    targetsDirectory: root,
    toolchainDirectory: root,
    templatesDirectory: root,
    docsDirectory: root,
    ...overrides
  };
}

/** 组装好的真实构建环境。 */
export interface RealBuildEnvironment {
  /** 构建服务。 */
  readonly service: DefaultBuildService;
  /** 文件系统。 */
  readonly fileSystem: NodeFileSystem;
  /** 扩展资源位置。 */
  readonly resources: ExtensionResources;
}

/**
 * 用扩展目录里的目标与工具链组装构建服务。
 * @param resources 资源位置；不给时用扩展目录。
 * @returns 构建环境。
 */
export function createRealBuildEnvironment(resources: ExtensionResources = createExtensionResources(EXTENSION_DIRECTORY)): RealBuildEnvironment {
  const fileSystem = new NodeFileSystem();
  const service = new DefaultBuildService({
    fileSystem,
    configReader: new FileProjectConfigReader(fileSystem),
    targets: new FileTargetRepository(fileSystem, resources),
    toolchainLocator: new BundledToolchainLocator(fileSystem, resources),
    toolRunner: new ProcessToolRunner(),
    resources
  });
  return { service, fileSystem, resources };
}

/** 本机是否已组装好扩展自带的 cc65 工具链。 */
export const HAS_TOOLCHAIN = (() => {
  try {
    new BundledToolchainLocator(new NodeFileSystem(), createExtensionResources(EXTENSION_DIRECTORY)).locate();
    return true;
  } catch {
    return false;
  }
})();

/**
 * 记录工具输出的汇报器。
 * @returns 汇报器与已记录的输出。
 */
export function createRecordingReporter(): { reporter: BuildReporter; output: () => string } {
  const chunks: string[] = [];
  return { reporter: { log: () => undefined, write: (text) => chunks.push(text) }, output: () => chunks.join('') };
}
