// ------------------------------------------------------------------------
// 名称：fcdev-project-info-provider.ts
// 说明：项目信息提供者的实现：找到文件所属项目，运行 `fcdev info --json` 并缓存结果。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-10
// 备注：读取失败（没装 fcdev、找不到工具链）时返回 undefined，并在 30 秒后允许重试，
//       避免每次悬停都启动一次命令行；fcdev.json 变化时由入口层调用 invalidate。
// ------------------------------------------------------------------------

import { existsSync } from 'node:fs';
import * as path from 'node:path';
import type { FcdevCommandRunner } from '../../contracts/project/fcdev-command-runner';
import type { ProjectInfo, ProjectInfoProvider } from '../../contracts/project/project-info';
import type { Logger } from '../../platform/logging/logger';
import { UserFacingError } from '../../shared/errors/user-facing-error';
import { parseProjectInfo } from '../../shared/project/project-info';

/** 项目配置文件名。 */
const CONFIG_FILE_NAME = 'fcdev.json';

/** 读取失败后允许重试的间隔，单位毫秒。 */
const FAILURE_RETRY_MS = 30_000;

/**
 * 从文件所在目录向上查找 fcdev.json。
 * @param filePath 文件的绝对路径。
 * @returns 项目根目录；没有找到时为 undefined。
 */
export function findProjectRoot(filePath: string): string | undefined {
  let directory = path.dirname(filePath);
  for (;;) {
    if (existsSync(path.join(directory, CONFIG_FILE_NAME))) return directory;
    const parent = path.dirname(directory);
    if (parent === directory) return undefined;
    directory = parent;
  }
}

/** 经命令行读取并缓存项目信息。 */
export class FcdevProjectInfoProvider implements ProjectInfoProvider {
  private readonly cache = new Map<string, Promise<ProjectInfo | undefined>>();

  /**
   * 创建提供者。
   * @param runner 运行 fcdev 命令的能力。
   * @param logger 日志。
   */
  constructor(
    private readonly runner: FcdevCommandRunner,
    private readonly logger: Logger
  ) {}

  /** @inheritdoc */
  get(filePath: string): Promise<ProjectInfo | undefined> {
    const root = findProjectRoot(filePath);
    if (root === undefined) return Promise.resolve(undefined);
    let info = this.cache.get(root);
    if (info === undefined) {
      info = this.load(root);
      this.cache.set(root, info);
    }
    return info;
  }

  /** @inheritdoc */
  invalidate(root?: string): void {
    if (root === undefined) this.cache.clear();
    else this.cache.delete(root);
  }

  /** 运行命令读取项目信息；失败时记日志，并在一段时间后允许重试。 */
  private async load(root: string): Promise<ProjectInfo | undefined> {
    try {
      const result = await this.runner.run(root, ['info', '--json']);
      const info = result.exitCode === 0 ? parseProjectInfo(result.stdout) : undefined;
      if (info === undefined) this.logger.info(`读取项目信息失败（${root}）：${result.stderr.trim() || '输出格式不对'}`);
      else return info;
    } catch (error) {
      if (error instanceof UserFacingError) this.logger.info(`读取项目信息失败（${root}）：${error.message}`);
      else this.logger.error(`读取项目信息出错（${root}）`, error);
    }
    this.scheduleRetry(root);
    return undefined;
  }

  /** 一段时间后丢弃失败的缓存，下次读取时重试。 */
  private scheduleRetry(root: string): void {
    setTimeout(() => this.cache.delete(root), FAILURE_RETRY_MS).unref();
  }
}
