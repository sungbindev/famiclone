// ------------------------------------------------------------------------
// 名称：debug-cli.mjs
// 说明：debug 命令的底层：找到调试 SDK 随包提供的终端客户端 ember-debug，并决定它找 Ember 调试服务的环境变量。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-15
// 备注：fcdev 自己不实现任何调试命令，全部交给 ember-debug；没有显式指定地址、令牌、端点文件或用户数据目录时，
//       按调试端口（环境变量 FCDEV_DEBUG_PORT，默认 5699）找到 fcdev 用 run --debug 启动的那个 Ember。
// ------------------------------------------------------------------------
import { existsSync } from 'node:fs';
import { fileURLToPath } from 'node:url';
import { FcdevError } from '../shared/index.mjs';
import { DEFAULT_DEBUG_PORT, debugUserDataDirectory } from './emulator-launcher.mjs';

/** 用户显式指定调试服务位置的环境变量，设置了任何一个就不再按端口推算 */
const EXPLICIT_ENV_NAMES = ['EMBER_DEBUG_URL', 'EMBER_DEBUG_TOKEN', 'EMBER_DEBUG_ENDPOINT', 'EMBER_USER_DATA'];

/**
 * 调试 SDK 的终端客户端（ember-debug）的入口文件。
 * @returns {string} 入口文件的绝对路径
 * @throws {FcdevError} SDK 没有安装或没有构建
 */
export function resolveDebugCli() {
  let file = null;
  try {
    file = fileURLToPath(new URL('./cli.js', import.meta.resolve('ember-debug-sdk')));
  } catch {
    // 下面统一报错
  }
  if (file === null || !existsSync(file)) {
    throw new FcdevError('找不到调试 SDK（ember-debug-sdk）的终端客户端', {
      code: 'DEBUG_SDK_MISSING',
      hint: '在本仓库里先运行 npm run build -w packages/ember-debug-sdk；用 npm 安装的 fcdev 会自带',
    });
  }
  return file;
}

/**
 * 运行终端客户端时用的环境变量。
 * @param {NodeJS.ProcessEnv} env 当前环境变量
 * @returns {NodeJS.ProcessEnv} 新的环境变量；没有显式指定位置时加上 fcdev 启动的 Ember 的用户数据目录
 */
export function debugEnvironment(env) {
  if (EXPLICIT_ENV_NAMES.some((name) => env[name])) return env;
  const port = Number(env.FCDEV_DEBUG_PORT) || DEFAULT_DEBUG_PORT;
  return { ...env, EMBER_USER_DATA: debugUserDataDirectory(port) };
}
