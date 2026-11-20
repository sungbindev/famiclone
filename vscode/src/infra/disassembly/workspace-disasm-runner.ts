// ------------------------------------------------------------------------
// 名称：workspace-disasm-runner.ts
// 说明：运行反汇编操作：从工程目录向上找已安装的 fcdev，动态加载它的反汇编模块，按操作名调用。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-21
// 备注：不经命令行、不把 fcdev 打进扩展，用的就是项目锁定的版本；
//       加载后按路径缓存，API_VERSION 不符时给出升级说明；
//       fcdev 自己抛出的可预期错误（FcdevError）转成带建议的 UserFacingError，
//       其他错误是缺陷，原样抛出。
// ------------------------------------------------------------------------

import { existsSync, readFileSync } from 'node:fs';
import * as path from 'node:path';
import { pathToFileURL } from 'node:url';
import type { DisasmRunner } from '../../contracts/disassembly/disasm-runner';
import type { Logger } from '../../platform/logging/logger';
import { UserFacingError } from '../../shared/errors/user-facing-error';

/** fcdev 包相对 node_modules 所在目录的路径。 */
const FCDEV_PACKAGE = path.join('node_modules', 'fcdev');

/** 反汇编模块相对 fcdev 包的路径。 */
const DISASM_MODULE = path.join('src', 'disasm', 'index.mjs');

/** 本扩展支持的操作接口版本，与 fcdev 的 API_VERSION 对应。 */
export const SUPPORTED_API_VERSION = 1;

/** fcdev 反汇编模块里扩展用到的部分。 */
interface FcdevDisasmModule {
  readonly API_VERSION?: unknown;
  readonly runOperation?: (operation: string, params: Record<string, unknown>) => Promise<unknown>;
}

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

/** 动态加载工作区里的 fcdev 来运行反汇编操作。 */
export class WorkspaceDisasmRunner implements DisasmRunner {
  private readonly modules = new Map<string, Promise<FcdevDisasmModule>>();

  /** @param logger 日志，记录加载的 fcdev 位置与版本。 */
  constructor(private readonly logger: Logger) {}

  /** @inheritdoc */
  async run(startDirectory: string, operation: string, params: Record<string, unknown>): Promise<unknown> {
    const packageDirectory = locateFcdevPackage(startDirectory);
    if (packageDirectory === undefined) {
      throw new UserFacingError('项目里没有安装 fcdev（找不到 node_modules/fcdev），请先运行 npm install fcdev');
    }
    const disasm = await this.load(packageDirectory);
    const runOperation = disasm.runOperation;
    if (typeof runOperation !== 'function') throw new UserFacingError('fcdev 的反汇编模块缺少 runOperation，请升级 fcdev');
    try {
      return await runOperation(operation, params);
    } catch (error) {
      if (isFcdevError(error)) {
        throw new UserFacingError(error.hint === undefined || error.hint === '' ? error.message : `${error.message}\n建议：${error.hint}`, error);
      }
      throw error;
    }
  }

  /** 加载并检查 fcdev 的反汇编模块，同一个包只加载一次。 */
  private load(packageDirectory: string): Promise<FcdevDisasmModule> {
    let loaded = this.modules.get(packageDirectory);
    if (loaded === undefined) {
      loaded = this.import(packageDirectory);
      this.modules.set(packageDirectory, loaded);
      loaded.catch(() => this.modules.delete(packageDirectory));
    }
    return loaded;
  }

  /** 真正加载，并核对接口版本。 */
  private async import(packageDirectory: string): Promise<FcdevDisasmModule> {
    const version = readVersion(packageDirectory);
    const file = path.join(packageDirectory, DISASM_MODULE);
    if (!existsSync(file)) throw new UserFacingError(`fcdev ${version} 没有反汇编模块，请升级 fcdev`);
    let disasm: FcdevDisasmModule;
    try {
      disasm = (await import(pathToFileURL(file).href)) as FcdevDisasmModule;
    } catch (error) {
      throw new UserFacingError(`加载 fcdev ${version} 的反汇编模块失败：${error instanceof Error ? error.message : String(error)}`, error);
    }
    if (typeof disasm.API_VERSION !== 'number') throw new UserFacingError(`fcdev ${version} 还没有反汇编工具接口，请升级 fcdev`);
    if (disasm.API_VERSION < SUPPORTED_API_VERSION) throw new UserFacingError(`fcdev ${version} 的反汇编接口过旧，请升级 fcdev`);
    if (disasm.API_VERSION > SUPPORTED_API_VERSION) throw new UserFacingError(`fcdev ${version} 的反汇编接口比本扩展新，请升级 fcdev 扩展`);
    this.logger.info(`已加载 fcdev ${version} 的反汇编模块（${packageDirectory}）`);
    return disasm;
  }
}
