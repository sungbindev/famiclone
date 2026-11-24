// ------------------------------------------------------------------------
// 名称：fakes.ts
// 说明：测试替身：内存里的汇编源文件存取与固定的项目信息提供者。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-10
// 备注：只替身系统边界（文件系统与命令行），不替身被测的逻辑。
// ------------------------------------------------------------------------

import type { AssemblySourceStore } from '../../src/contracts/assembly/assembly-source-store';
import type { ProjectInfo, ProjectInfoProvider } from '../../src/contracts/project/project-info';

/** 内存里的汇编源文件存取，并记录读取次数。 */
export class FakeSourceStore implements AssemblySourceStore {
  readonly reads: string[] = [];

  /**
   * 创建替身。
   * @param files 路径到文件内容。
   */
  constructor(readonly files: Record<string, string>) {}

  /** @inheritdoc */
  async read(path: string): Promise<string | undefined> {
    this.reads.push(path);
    return this.files[path];
  }

  /** @inheritdoc */
  async listWorkspaceFiles(): Promise<readonly string[]> {
    return Object.keys(this.files);
  }
}

/** 固定返回同一份项目信息的提供者。 */
export class FakeProjectInfoProvider implements ProjectInfoProvider {
  /**
   * 创建替身。
   * @param info 要返回的项目信息；不给时表示不在 FC Dev 项目里。
   */
  constructor(private readonly info?: ProjectInfo) {}

  /** @inheritdoc */
  async get(): Promise<ProjectInfo | undefined> {
    return this.info;
  }

  /** @inheritdoc */
  invalidate(): void {}
}
