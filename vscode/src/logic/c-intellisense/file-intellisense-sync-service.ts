// ------------------------------------------------------------------------
// 名称：file-intellisense-sync-service.ts
// 说明：C 智能感知配置同步服务的实现：读取项目描述，写 .vscode/c_cpp_properties.json
//       与 compile_flags.txt，同一项目的同步依次进行。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-22
// 备注：头文件目录在扩展目录里，路径随扩展版本变化；每次同步都按当前路径改写，
//       内容没变时不写盘。可预期的错误（配置有误、找不到目标）转成失败结果；
//       其他异常原样抛出。
// ------------------------------------------------------------------------

import * as path from 'node:path';
import type { IntelliSenseSyncService, SyncResult } from '../../contracts/c-intellisense/intellisense-sync-service';
import type { FileSystem } from '../../contracts/files/file-system';
import type { ProjectDescriber } from '../../contracts/project/project-describer';
import type { FcdevProjectLocator } from '../../contracts/project/project-locator';
import { buildCompileFlags, isGeneratedCompileFlags, mergeCppProperties } from '../../shared/c-intellisense/cpp-properties';
import { UserFacingError } from '../../shared/errors/user-facing-error';

/** 配置文件相对项目根目录的路径。 */
const PROPERTIES_FILE = path.join('.vscode', 'c_cpp_properties.json');

/** clangd 的编译参数文件名。 */
const COMPILE_FLAGS_FILE = 'compile_flags.txt';

/** 直接读取项目描述并写配置文件的同步服务。 */
export class FileIntelliSenseSyncService implements IntelliSenseSyncService {
  private readonly queues = new Map<string, Promise<unknown>>();

  /**
   * 创建服务。
   * @param fileSystem 文件系统。
   * @param describer 项目描述器。
   * @param locator 查找工作区里 FC Dev 项目的能力。
   */
  constructor(
    private readonly fileSystem: FileSystem,
    private readonly describer: ProjectDescriber,
    private readonly locator: FcdevProjectLocator
  ) {}

  /** @inheritdoc */
  syncProject(root: string): Promise<SyncResult> {
    const previous = this.queues.get(root) ?? Promise.resolve();
    const next = previous.then(() => this.runSync(root));
    // 队列只关心先后顺序，上一次的失败不影响下一次。
    this.queues.set(root, next.catch(() => undefined));
    return next;
  }

  /** @inheritdoc */
  async syncWorkspace(): Promise<readonly SyncResult[]> {
    const roots = await this.locator.findProjectRoots();
    return Promise.all(roots.map((root) => this.syncProject(root)));
  }

  /** 同步一次，把结果整理成同步结果。 */
  private runSync(root: string): SyncResult {
    try {
      const description = this.describer.describe({ cwd: root });
      const written = [this.writeIfChanged(path.join(root, PROPERTIES_FILE), (existing, file) => mergeCppProperties(existing, file, description))];
      const flagsFile = path.join(root, COMPILE_FLAGS_FILE);
      const existingFlags = this.fileSystem.kindOf(flagsFile) === 'file' ? this.fileSystem.readText(flagsFile) : undefined;
      if (existingFlags !== undefined && !isGeneratedCompileFlags(existingFlags)) {
        return { root, ok: true, message: `已生成：\n  ${written.join('\n  ')}\ncompile_flags.txt 已存在且不是扩展生成的，没有覆盖` };
      }
      written.push(this.writeIfChanged(flagsFile, () => buildCompileFlags(description)));
      return { root, ok: true, message: `已生成：\n  ${written.join('\n  ')}` };
    } catch (error) {
      if (error instanceof UserFacingError) return { root, ok: false, message: error.message };
      throw error;
    }
  }

  /**
   * 生成新内容，与已有内容不同才写盘。
   * @param file 文件路径。
   * @param build 由已有内容（不存在时为 undefined）与文件路径生成新内容。
   * @returns 文件路径。
   */
  private writeIfChanged(file: string, build: (existing: string | undefined, file: string) => string): string {
    const existing = this.fileSystem.kindOf(file) === 'file' ? this.fileSystem.readText(file) : undefined;
    const text = build(existing, file);
    if (text !== existing) {
      this.fileSystem.ensureDirectory(path.dirname(file));
      this.fileSystem.writeText(file, text);
    }
    return file;
  }
}
