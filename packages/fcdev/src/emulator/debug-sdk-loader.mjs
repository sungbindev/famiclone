// ------------------------------------------------------------------------
// 名称：debug-sdk-loader.mjs
// 说明：加载调试 SDK 的 Node 入口：只在需要连接 Ember 时才加载，没有构建时给出建议。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-21
// 备注：SDK 在 fcdev 包内的 dist/debug，用 npm 安装的 fcdev 自带；在仓库里要先运行 npm run debug-sdk:build。
// ------------------------------------------------------------------------
import { FcdevError } from '../shared/index.mjs';

/**
 * 加载调试 SDK 的 Node 入口。
 * @returns {Promise<typeof import('../../dist/debug/types/debug-sdk/src/node.js')>} SDK 的 Node 入口
 * @throws {FcdevError} SDK 没有构建
 */
export async function loadDebugSdk() {
  try {
    return await import('../../dist/debug/node.js');
  } catch (error) {
    if (error?.code === 'ERR_MODULE_NOT_FOUND') {
      throw new FcdevError('找不到调试 SDK 的构建产物', {
        code: 'DEBUG_SDK_MISSING',
        hint: '在本仓库的 packages/fcdev 目录里先运行 npm run debug-sdk:build；用 npm 安装的 fcdev 会自带',
        cause: error,
      });
    }
    throw error;
  }
}
