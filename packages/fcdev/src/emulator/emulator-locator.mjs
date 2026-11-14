// ------------------------------------------------------------------------
// 名称：emulator-locator.mjs
// 说明：定位模拟器程序：配置项、环境变量，按顺序查找。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-10
// 备注：无
// ------------------------------------------------------------------------
import { existsSync } from 'node:fs';
import path from 'node:path';
import { FcdevError } from '../shared/index.mjs';

/** 环境变量：模拟器的路径 */
export const EMULATOR_ENV_NAME = 'FCDEV_EMULATOR';

/**
 * 按顺序定位模拟器：配置项 emulator、环境变量。
 * @param {{configured?: string, env?: NodeJS.ProcessEnv}} [options] configured 为配置里的绝对路径
 * @returns {{executable: string, source: string}} 模拟器程序与来源
 * @throws {FcdevError} 都找不到
 */
export function locateEmulator({ configured, env = process.env } = {}) {
  const explicit = configured ? { executable: configured, source: 'config' } : env[EMULATOR_ENV_NAME] ? { executable: path.resolve(env[EMULATOR_ENV_NAME]), source: 'env' } : null;
  if (explicit) {
    if (!existsSync(explicit.executable)) {
      throw new FcdevError(`找不到模拟器：${explicit.executable}`, { code: 'EMULATOR_NOT_FOUND' });
    }
    return explicit;
  }
  throw new FcdevError('找不到模拟器', {
    code: 'EMULATOR_NOT_FOUND',
    hint: `任选一种：设置环境变量 ${EMULATOR_ENV_NAME} 指向模拟器程序；在 fcdev.json 里用 emulator 指定`,
  });
}
