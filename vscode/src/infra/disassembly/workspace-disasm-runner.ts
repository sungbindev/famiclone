// ------------------------------------------------------------------------
// 名称：workspace-disasm-runner.ts
// 说明：运行反汇编操作：动态加载工作区 fcdev 的反汇编模块（src/disasm/index.mjs），按操作名调用。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-21
// 备注：查找、加载、版本检查与错误转换都在 FcdevModuleRunner 里，这里只给出反汇编模块的描述。
// ------------------------------------------------------------------------

import * as path from 'node:path';
import type { DisasmRunner } from '../../contracts/disassembly/disasm-runner';
import type { Logger } from '../../platform/logging/logger';
import { FcdevModuleRunner, locateFcdevPackage } from './fcdev-module-runner';

export { locateFcdevPackage };

/** 本扩展支持的操作接口版本，与 fcdev 的 API_VERSION 对应。 */
export const SUPPORTED_API_VERSION = 1;

/** 动态加载工作区里的 fcdev 来运行反汇编操作。 */
export class WorkspaceDisasmRunner extends FcdevModuleRunner implements DisasmRunner {
  /** @param logger 日志，记录加载的 fcdev 位置与版本。 */
  constructor(logger: Logger) {
    super(logger, {
      label: '反汇编',
      modulePath: path.join('src', 'disasm', 'index.mjs'),
      versionExport: 'API_VERSION',
      runExport: 'runOperation',
      supportedVersion: SUPPORTED_API_VERSION
    });
  }
}
