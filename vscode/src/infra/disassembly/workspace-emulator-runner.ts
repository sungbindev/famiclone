// ------------------------------------------------------------------------
// 名称：workspace-emulator-runner.ts
// 说明：运行模拟器操作：动态加载工作区 fcdev 的模拟器模块（src/emulator/index.mjs），按操作名调用；
//       扩展停用时关闭会话并结束自己启动的 Ember。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-21
// 备注：查找、加载、版本检查与错误转换都在 FcdevModuleRunner 里；
//       模块按路径只加载一次，所以到 Ember 的连接在多次工具调用之间保持。
// ------------------------------------------------------------------------

import * as path from 'node:path';
import type { EmulatorRunner } from '../../contracts/disassembly/emulator-runner';
import type { Logger } from '../../platform/logging/logger';
import { FcdevModuleRunner } from './fcdev-module-runner';

/** 本扩展支持的模拟器操作接口版本，与 fcdev 的 EMULATOR_API_VERSION 对应。 */
export const SUPPORTED_EMULATOR_API_VERSION = 1;

/** 动态加载工作区里的 fcdev 来运行模拟器操作。 */
export class WorkspaceEmulatorRunner extends FcdevModuleRunner implements EmulatorRunner {
  /** @param logger 日志，记录加载的 fcdev 位置与版本。 */
  constructor(private readonly log: Logger) {
    super(log, {
      label: '模拟器',
      modulePath: path.join('src', 'emulator', 'index.mjs'),
      versionExport: 'EMULATOR_API_VERSION',
      runExport: 'runEmulatorOperation',
      supportedVersion: SUPPORTED_EMULATOR_API_VERSION
    });
  }

  /** @inheritdoc */
  async shutdown(): Promise<void> {
    try {
      await this.runLoaded('status', { close: true, terminate: true });
    } catch (error) {
      this.log.error('关闭模拟器会话出错', error);
    }
  }
}
