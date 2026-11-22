// ------------------------------------------------------------------------
// 名称：bundled-toolchain-locator.ts
// 说明：定位随扩展发布的 cc65 工具链：检查扩展目录下的 toolchain 里有 cc65、ca65、ld65。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-22
// 备注：不使用 PATH 里的 cc65，也不接受其他位置的工具链，避免与运行库版本不一致。
// ------------------------------------------------------------------------

import * as path from 'node:path';
import type { FileSystem } from '../../contracts/files/file-system';
import type { ExtensionResources } from '../../contracts/resources/extension-resources';
import type { Toolchain, ToolchainLocator } from '../../contracts/toolchain/toolchain';
import { FcdevError } from '../../shared/errors/fcdev-error';

/** 工具链里必须存在的程序。 */
const REQUIRED_TOOLS = ['cc65', 'ca65', 'ld65'] as const;

/**
 * 工具程序的完整路径。
 * @param toolchain 工具链。
 * @param tool 工具名，如 cc65。
 * @returns 可执行文件路径。
 */
export function getToolPath(toolchain: Toolchain, tool: string): string {
  return path.join(toolchain.home, 'bin', `${tool}.exe`);
}

/** 定位随扩展发布的工具链。 */
export class BundledToolchainLocator implements ToolchainLocator {
  /**
   * 创建定位器。
   * @param fileSystem 文件系统。
   * @param resources 扩展资源位置。
   */
  constructor(
    private readonly fileSystem: FileSystem,
    private readonly resources: ExtensionResources
  ) {}

  /** @inheritdoc */
  locate(): Toolchain {
    const toolchain: Toolchain = { home: this.resources.toolchainDirectory };
    const missing = REQUIRED_TOOLS.filter((tool) => this.fileSystem.kindOf(getToolPath(toolchain, tool)) !== 'file');
    if (missing.length > 0) {
      throw new FcdevError(`扩展自带的 cc65 工具链不完整（${toolchain.home} 的 bin 下缺少 ${missing.join('、')}）`, {
        code: 'TOOLCHAIN_INCOMPLETE',
        hint: '在源码里运行 tools/build-cc65.mjs 组装工具链，或重新安装扩展'
      });
    }
    return toolchain;
  }
}
