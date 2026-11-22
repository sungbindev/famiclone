// ------------------------------------------------------------------------
// 名称：workspace-emulator-settings.ts
// 说明：读取扩展设置里的模拟器选项：Ember 可执行文件的位置（fcdev.emulator.path）与调试端口
//       （fcdev.emulator.debugPort）。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-22
// 备注：两项设置的作用域都是 machine，工作区设置不能指定要运行的程序；每次使用时读取，设置改了立即
//       生效；端口不是 1 到 65535 的整数时按默认端口处理。
// ------------------------------------------------------------------------

import * as vscode from 'vscode';
import { DEFAULT_DEBUG_PORT, type EmulatorSettings } from '../../contracts/emulator/emulator-settings';

/** 设置分组。 */
export const EMULATOR_SECTION = 'fcdev.emulator';

/** Ember 位置的设置名（完整名是 fcdev.emulator.path）。 */
export const PATH_KEY = 'path';

/** 调试端口的设置名（完整名是 fcdev.emulator.debugPort）。 */
export const DEBUG_PORT_KEY = 'debugPort';

/** 读 VS Code 设置里的模拟器选项。 */
export class WorkspaceEmulatorSettings implements EmulatorSettings {
  /** @inheritdoc */
  executablePath(): string | undefined {
    const value = vscode.workspace.getConfiguration(EMULATOR_SECTION).get<string>(PATH_KEY);
    const trimmed = value?.trim();
    return trimmed === undefined || trimmed === '' ? undefined : trimmed;
  }

  /** @inheritdoc */
  debugPort(): number {
    const value = vscode.workspace.getConfiguration(EMULATOR_SECTION).get<number>(DEBUG_PORT_KEY);
    return typeof value === 'number' && Number.isInteger(value) && value >= 1 && value <= 65_535 ? value : DEFAULT_DEBUG_PORT;
  }
}
