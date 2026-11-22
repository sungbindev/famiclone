// ------------------------------------------------------------------------
// 名称：describing-project-info-provider.ts
// 说明：项目信息提供者的实现：找到文件所属项目，读取项目描述并缓存结果。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-22
// 备注：读取失败（配置有误、找不到目标）时返回 undefined，并在 30 秒后允许重试，
//       避免每次悬停都重新读取；fcdev.json 变化时由入口层调用 invalidate。
// ------------------------------------------------------------------------

import * as path from 'node:path';
import type { FileSystem } from '../../contracts/files/file-system';
import type { ProjectDescriber } from '../../contracts/project/project-describer';
import type { ProjectInfo, ProjectInfoProvider } from '../../contracts/project/project-info';
import type { Logger } from '../../platform/logging/logger';
import { UserFacingError } from '../../shared/errors/user-facing-error';
import { toProjectInfo } from '../../shared/project/project-info';
import { CONFIG_FILE_NAME } from '../../shared/project/project-config';

/** 读取失败后允许重试的间隔，单位毫秒。 */
const FAILURE_RETRY_MS = 30_000;

/** 读取并缓存项目信息。 */
export class DescribingProjectInfoProvider implements ProjectInfoProvider {
  private readonly cache = new Map<string, ProjectInfo | undefined>();

  /**
   * 创建提供者。
   * @param fileSystem 文件系统。
   * @param describer 项目描述器。
   * @param logger 日志。
   */
  constructor(
    private readonly fileSystem: FileSystem,
    private readonly describer: ProjectDescriber,
    private readonly logger: Logger
  ) {}

  /**
   * 从文件所在目录向上查找 fcdev.json。
   * @param filePath 文件的绝对路径。
   * @returns 项目根目录；没有找到时为 undefined。
   */
  findProjectRoot(filePath: string): string | undefined {
    let directory = path.dirname(filePath);
    for (;;) {
      if (this.fileSystem.kindOf(path.join(directory, CONFIG_FILE_NAME)) === 'file') return directory;
      const parent = path.dirname(directory);
      if (parent === directory) return undefined;
      directory = parent;
    }
  }

  /** @inheritdoc */
  get(filePath: string): Promise<ProjectInfo | undefined> {
    const root = this.findProjectRoot(filePath);
    if (root === undefined) return Promise.resolve(undefined);
    if (!this.cache.has(root)) this.cache.set(root, this.load(root));
    return Promise.resolve(this.cache.get(root));
  }

  /** @inheritdoc */
  invalidate(root?: string): void {
    if (root === undefined) this.cache.clear();
    else this.cache.delete(root);
  }

  /** 读取项目描述；失败时记日志，并在一段时间后允许重试。 */
  private load(root: string): ProjectInfo | undefined {
    try {
      return toProjectInfo(this.describer.describe({ cwd: root }));
    } catch (error) {
      if (error instanceof UserFacingError) this.logger.info(`读取项目信息失败（${root}）：${error.message}`);
      else this.logger.error(`读取项目信息出错（${root}）`, error);
    }
    setTimeout(() => this.cache.delete(root), FAILURE_RETRY_MS).unref();
    return undefined;
  }
}
