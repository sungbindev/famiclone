// ------------------------------------------------------------------------
// 名称：emulator-locator.mjs
// 说明：定位 Ember（模拟器）的可执行文件：配置项、环境变量、随包的 Ember（用户级副本），按顺序查找，不去 PATH 里猜。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-10
// 备注：随包的 Ember 来自 fcdev-ember 包，装在用户级目录里全机共用，见 emulator-install.mjs；非 Windows 平台没有随包的 Ember。
// ------------------------------------------------------------------------
import { existsSync } from 'node:fs';
import path from 'node:path';
import { FcdevError } from '../shared/index.mjs';
import { installBundledEmber } from './emulator-install.mjs';

/** 环境变量：Ember 可执行文件的路径 */
export const EMULATOR_ENV_NAME = 'FCDEV_EMULATOR';

/**
 * 按顺序定位 Ember：配置项 emulator、环境变量、随包的 Ember。
 * @param {{configured?: string, env?: NodeJS.ProcessEnv, platform?: string}} [options] configured 为配置里的绝对路径
 * @returns {{executable: string, source: 'config'|'env'|'installed'|'package'}} Ember 可执行文件与来源
 * @throws {FcdevError} 都找不到
 */
export function locateEmulator({ configured, env = process.env, platform = process.platform } = {}) {
  const explicit = configured ? { executable: configured, source: 'config' } : env[EMULATOR_ENV_NAME] ? { executable: path.resolve(env[EMULATOR_ENV_NAME]), source: 'env' } : null;
  if (explicit) {
    if (!existsSync(explicit.executable)) {
      throw new FcdevError(`找不到 Ember：${explicit.executable}`, { code: 'EMULATOR_NOT_FOUND' });
    }
    return explicit;
  }
  const bundled = installBundledEmber({ env, platform });
  if (bundled) return { executable: bundled.executable, source: bundled.source };
  throw new FcdevError('找不到 Ember', {
    code: 'EMULATOR_NOT_FOUND',
    hint: platform === 'win32'
      ? `随包的模拟器（fcdev-ember）没有安装：重新运行 npm install（安装时跳过了可选依赖就会缺少）；也可以设置环境变量 ${EMULATOR_ENV_NAME} 或在 fcdev.json 里用 emulator 指向自备的 Ember`
      : `当前平台（${platform}）没有随包的 Ember，它目前只有 Windows 版；可以设置环境变量 ${EMULATOR_ENV_NAME} 或在 fcdev.json 里用 emulator 指向自备的 Ember`,
  });
}
