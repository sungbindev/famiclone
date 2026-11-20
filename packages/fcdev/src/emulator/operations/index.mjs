// ------------------------------------------------------------------------
// 名称：index.mjs
// 说明：模拟器操作的出口：Copilot 模拟器工具对应的操作，按名字调用；会话（到 Ember 的连接）保存在进程里，跨调用保持。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-21
// 备注：EMULATOR_API_VERSION 在操作的名字、参数或结果的含义发生不兼容变化时加 1；Ember 返回的请求错误与连接错误统一成 FcdevError。
// ------------------------------------------------------------------------
import { FcdevError } from '../../shared/index.mjs';
import { connect, launch, status } from './connection-operations.mjs';
import { inspect, input, record, screenshot } from './inspect-operations.mjs';
import { breakpoint, load, run, step } from './program-operations.mjs';

/** 操作接口的版本 */
export const EMULATOR_API_VERSION = 1;

/** 操作名到实现 */
export const EMULATOR_OPERATIONS = Object.freeze({ connect, launch, status, load, run, breakpoint, step, inspect, screenshot, input, record });

/** 全部操作名 */
export const EMULATOR_OPERATION_NAMES = Object.freeze(Object.keys(EMULATOR_OPERATIONS));

/**
 * 把调试 SDK 的错误换成带建议的 FcdevError；其他错误原样抛出。
 * @param {unknown} error 捕获的错误
 * @returns {unknown} 转换后的错误
 */
function convertSdkError(error) {
  if (error instanceof FcdevError || !(error instanceof Error)) return error;
  if (error.name === 'EmberRequestError') {
    return new FcdevError(`Ember 拒绝了请求：${error.message}（${error.code}）`, { code: 'EMULATOR_REQUEST_FAILED', hint: '检查参数与机器状态（有没有载入程序、是否已通电）', cause: error });
  }
  if (error.name === 'EmberConnectionError') {
    return new FcdevError(`与 Ember 的连接出错：${error.message}`, { code: 'EMULATOR_CONNECTION_LOST', hint: 'Ember 可能已退出；用 emu_connect 重新连接，或用 emu_launch 重新启动', cause: error });
  }
  return error;
}

/**
 * 按名字运行一个模拟器操作。
 * @param {string} name 操作名，见 EMULATOR_OPERATION_NAMES
 * @param {Record<string, unknown>} params 参数
 * @param {{env?: NodeJS.ProcessEnv}} [options] 运行环境
 * @returns {Promise<object>} 可序列化的结果
 * @throws {FcdevError} 操作名不认识、参数不合法、没有连接，或 Ember 报错
 */
export async function runEmulatorOperation(name, params, options = {}) {
  if (!Object.hasOwn(EMULATOR_OPERATIONS, name)) throw new FcdevError(`未知的操作：${String(name)}`, { code: 'EMULATOR_OPERATION_UNKNOWN', hint: `可用的操作：${EMULATOR_OPERATION_NAMES.join('、')}` });
  if (params === null || typeof params !== 'object' || Array.isArray(params)) throw new FcdevError('参数必须是对象', { code: 'EMULATOR_PARAM_INVALID' });
  try {
    return await EMULATOR_OPERATIONS[name](params, options);
  } catch (error) {
    throw convertSdkError(error);
  }
}
