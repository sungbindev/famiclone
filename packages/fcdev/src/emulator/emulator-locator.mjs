// ------------------------------------------------------------------------
// 名称：emulator-locator.mjs
// 说明：定位 Ember（模拟器）的可执行文件：配置项、环境变量，按顺序查找，不去 PATH 里猜。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-10
// 备注：无
// ------------------------------------------------------------------------
import { existsSync } from 'node:fs';
import path from 'node:path';
import { FcdevError } from '../shared/index.mjs';

/** 环境变量：Ember 可执行文件的路径 */
export const EMULATOR_ENV_NAME = 'FCDEV_EMULATOR';

/**
 * 按顺序定位 Ember：配置项 emulator、环境变量。
 * @param {{configured?: string, env?: NodeJS.ProcessEnv}} [options] configured 为配置里的绝对路径
 * @returns {{executable: string, source: string}} Ember 可执行文件与来源
 * @throws {FcdevError} 都找不到
 */
export function locateEmulator({ configured, env = process.env } = {}) {
  const explicit = configured ? { executable: configured, source: 'config' } : env[EMULATOR_ENV_NAME] ? { executable: path.resolve(env[EMULATOR_ENV_NAME]), source: 'env' } : null;
  if (explicit) {
    if (!existsSync(explicit.executable)) {
      throw new FcdevError(`找不到 Ember：${explicit.executable}`, { code: 'EMULATOR_NOT_FOUND' });
    }
    return explicit;
  }
  throw new FcdevError('找不到 Ember', {
    code: 'EMULATOR_NOT_FOUND',
    hint: `任选一种：设置环境变量 ${EMULATOR_ENV_NAME} 指向 Ember 的可执行文件；在 fcdev.json 里用 emulator 指定`,
  });
}
