// ------------------------------------------------------------------------
// 名称：operation-context.ts
// 说明：模拟器操作共用的上下文：外部能力与当前会话的存储，以及取 Ember 可执行文件位置的函数。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-22
// 备注：Ember 的位置只来自扩展设置 fcdev.emulator.path，没有设置时报错并说明去哪里设置。
// ------------------------------------------------------------------------

import type { EmulatorEnvironment } from '../../contracts/emulator/emulator-environment';
import { FcdevError } from '../../shared/errors/fcdev-error';
import type { EmulatorSessionStore } from './emulator-session';

/** 设置项的名字，用在提示里。 */
export const EMULATOR_PATH_SETTING = 'fcdev.emulator.path';

/** 一次操作的上下文。 */
export interface OperationContext {
  /** 外部能力。 */
  readonly env: EmulatorEnvironment;
  /** 当前会话的存储。 */
  readonly store: EmulatorSessionStore;
}

/**
 * 取 Ember 可执行文件的路径：来自设置，文件必须存在。
 * @param env 外部能力。
 * @returns 可执行文件的路径。
 * @throws FcdevError 没有设置，或设置的文件不存在。
 */
export function requireEmulatorExecutable(env: EmulatorEnvironment): string {
  const executable = env.settings.executablePath();
  if (executable === undefined || executable === '') {
    throw new FcdevError('还没有设置 Ember 的位置', {
      code: 'EMULATOR_PATH_NOT_SET',
      hint: `在 VS Code 设置里把 ${EMULATOR_PATH_SETTING} 设为 Ember 可执行文件（Ember.exe）的路径`
    });
  }
  if (env.files.kindOf(executable) !== 'file') {
    throw new FcdevError(`找不到 Ember：${executable}`, {
      code: 'EMULATOR_NOT_FOUND',
      hint: `检查设置 ${EMULATOR_PATH_SETTING} 指向的文件是否存在`
    });
  }
  return executable;
}
