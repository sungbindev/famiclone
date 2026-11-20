// ------------------------------------------------------------------------
// 名称：fcdev-module-runner.ts
// 说明：运行 fcdev 某个模块的操作：从起点目录向上找已安装的 fcdev，动态加载它的模块，按操作名调用。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-21
// 备注：不经命令行、不把 fcdev 打进扩展，用的就是项目锁定的版本；
//       加载后按路径缓存（同一个包里的模块实例只有一个，模拟器的会话因此跨调用保持），
//       接口版本不符时给出升级说明；
//       fcdev 自己抛出的可预期错误（FcdevError）转成带建议的 UserFacingError，
//       其他错误是缺陷，原样抛出。
// ------------------------------------------------------------------------

import { existsSync, readFileSync } from 'node:fs';
import * as path from 'node:path';
import { pathToFileURL } from 'node:url';
import type { Logger } from '../../platform/logging/logger';
import { UserFacingError } from '../../shared/errors/user-facing-error';

/** fcdev 包相对 node_modules 所在目录的路径。 */
const FCDEV_PACKAGE = path.join('node_modules', 'fcdev');

/** 一个 fcdev 模块的描述：在哪里、导出什么、扩展支持哪个接口版本。 */
export interface FcdevModuleSpec {
  /** 说明里用的名称，如“反汇编”“模拟器”。 */
  readonly label: string;
  /** 模块文件相对 fcdev 包的路径。 */
  readonly modulePath: string;
  /** 导出的接口版本的名字。 */
  readonly versionExport: string;
  /** 导出的运行函数的名字，签名是（操作名，参数）。 */
  readonly runExport: string;
  /** 本扩展支持的接口版本。 */
  readonly supportedVersion: number;
}

/** 加载后的 fcdev 模块：导出的名字由描述指定。 */
type FcdevModule = Record<string, unknown>;

/**
 * 从起点目录向上查找已安装的 fcdev 包。
 * @param startDirectory 起点目录。
 * @returns fcdev 包的目录；没有安装时为 undefined。
 */
export function locateFcdevPackage(startDirectory: string): string | undefined {
  let directory = path.resolve(startDirectory);
  for (;;) {
    const candidate = path.join(directory, FCDEV_PACKAGE);
    if (existsSync(path.join(candidate, 'package.json'))) return candidate;
    const parent = path.dirname(directory);
    if (parent === directory) return undefined;
    directory = parent;
  }
}

/** 读出 fcdev 包的版本号，读不到时为“未知”。 */
function readVersion(packageDirectory: string): string {
  try {
    const parsed = JSON.parse(readFileSync(path.join(packageDirectory, 'package.json'), 'utf8')) as { version?: unknown };
    return typeof parsed.version === 'string' ? parsed.version : '未知';
  } catch {
    return '未知';
  }
}

/** 是不是 fcdev 抛出的可预期错误。 */
function isFcdevError(error: unknown): error is Error & { hint?: string } {
  return error instanceof Error && error.name === 'FcdevError';
}

/** 动态加载工作区里的 fcdev 模块来运行操作。 */
export class FcdevModuleRunner {
  private readonly modules = new Map<string, Promise<FcdevModule>>();

  /**
   * @param logger 日志，记录加载的 fcdev 位置与版本。
   * @param spec 要加载的模块。
   */
  constructor(
    private readonly logger: Logger,
    private readonly spec: FcdevModuleSpec
  ) {}

  /**
   * 运行一个操作。
   * @param startDirectory 查找 node_modules/fcdev 的起点目录。
   * @param operation 操作名。
   * @param params 操作的参数。
   * @returns 操作的结果。
   * @throws UserFacingError 没有安装 fcdev、版本不符，或操作本身失败。
   */
  async run(startDirectory: string, operation: string, params: Record<string, unknown>): Promise<unknown> {
    const packageDirectory = locateFcdevPackage(startDirectory);
    if (packageDirectory === undefined) {
      throw new UserFacingError('项目里没有安装 fcdev（找不到 node_modules/fcdev），请先运行 npm install fcdev');
    }
    return this.invoke(await this.load(packageDirectory), operation, params);
  }

  /**
   * 只在模块已经加载过时运行操作：用于收尾，没加载过就什么也不做。
   * @param operation 操作名。
   * @param params 操作的参数。
   * @returns 已加载的模块各自运行的结果；一个也没加载时为空。
   */
  protected async runLoaded(operation: string, params: Record<string, unknown>): Promise<unknown[]> {
    const results: unknown[] = [];
    for (const loading of this.modules.values()) {
      const module = await loading.catch(() => undefined);
      if (module !== undefined) results.push(await this.invoke(module, operation, params));
    }
    return results;
  }

  /** 调用模块的运行函数，把 fcdev 的可预期错误转成带建议的错误。 */
  private async invoke(module: FcdevModule, operation: string, params: Record<string, unknown>): Promise<unknown> {
    const runOperation = module[this.spec.runExport];
    if (typeof runOperation !== 'function') throw new UserFacingError(`fcdev 的${this.spec.label}模块缺少 ${this.spec.runExport}，请升级 fcdev`);
    try {
      return await (runOperation as (name: string, input: Record<string, unknown>) => Promise<unknown>)(operation, params);
    } catch (error) {
      if (isFcdevError(error)) {
        throw new UserFacingError(error.hint === undefined || error.hint === '' ? error.message : `${error.message}\n建议：${error.hint}`, error);
      }
      throw error;
    }
  }

  /** 加载并检查 fcdev 的模块，同一个包只加载一次。 */
  private load(packageDirectory: string): Promise<FcdevModule> {
    let loaded = this.modules.get(packageDirectory);
    if (loaded === undefined) {
      loaded = this.import(packageDirectory);
      this.modules.set(packageDirectory, loaded);
      loaded.catch(() => this.modules.delete(packageDirectory));
    }
    return loaded;
  }

  /** 真正加载，并核对接口版本。 */
  private async import(packageDirectory: string): Promise<FcdevModule> {
    const { label, modulePath, versionExport, supportedVersion } = this.spec;
    const version = readVersion(packageDirectory);
    const file = path.join(packageDirectory, modulePath);
    if (!existsSync(file)) throw new UserFacingError(`fcdev ${version} 没有${label}模块，请升级 fcdev`);
    let loaded: FcdevModule;
    try {
      loaded = (await import(pathToFileURL(file).href)) as FcdevModule;
    } catch (error) {
      throw new UserFacingError(`加载 fcdev ${version} 的${label}模块失败：${error instanceof Error ? error.message : String(error)}`, error);
    }
    const apiVersion = loaded[versionExport];
    if (typeof apiVersion !== 'number') throw new UserFacingError(`fcdev ${version} 还没有${label}工具接口，请升级 fcdev`);
    if (apiVersion < supportedVersion) throw new UserFacingError(`fcdev ${version} 的${label}接口过旧，请升级 fcdev`);
    if (apiVersion > supportedVersion) throw new UserFacingError(`fcdev ${version} 的${label}接口比本扩展新，请升级 fcdev 扩展`);
    this.logger.info(`已加载 fcdev ${version} 的${label}模块（${packageDirectory}）`);
    return loaded;
  }
}
