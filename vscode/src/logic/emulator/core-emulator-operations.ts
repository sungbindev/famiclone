// ------------------------------------------------------------------------
// 名称：core-emulator-operations.ts
// 说明：模拟器操作的实现：按操作名调用连接、程序、检查类操作，Ember 与调试连接的错误统一成带建议的
//       FcdevError；扩展停用时关闭会话并结束自己启动的 Ember。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-22
// 备注：会话保存在对象里，所以到 Ember 的连接在多次调用之间保持；EMULATOR_API_VERSION 在操作的名
//       字、参数或结果的含义发生不兼容变化时加 1。
// ------------------------------------------------------------------------

import type { EmulatorEnvironment } from '../../contracts/emulator/emulator-environment';
import type { EmulatorOperations } from '../../contracts/emulator/emulator-operations';
import { EmberConnectionError, EmberRequestError } from '../../shared/debug-client/errors';
import { FcdevError } from '../../shared/errors/fcdev-error';
import { events, profile } from './analysis-operations';
import { connect, launch, status } from './connection-operations';
import { EmulatorSessionStore } from './emulator-session';
import { inspect, input, record, screenshot } from './inspect-operations';
import type { OperationContext } from './operation-context';
import { breakpoint, load, run, step } from './program-operations';

/** 操作接口的版本。 */
export const EMULATOR_API_VERSION = 1;

/** 一个操作：上下文与参数进，结果出。 */
type Operation = (context: OperationContext, params: Record<string, unknown>) => Promise<Record<string, unknown>>;

/** 操作名到实现。 */
const OPERATIONS: Readonly<Record<string, Operation>> = Object.freeze({ connect, launch, status, load, run, breakpoint, step, inspect, screenshot, input, record, profile, events });

/** 全部操作名。 */
export const EMULATOR_OPERATION_NAMES: readonly string[] = Object.freeze(Object.keys(OPERATIONS));

/**
 * 把调试连接的错误换成带建议的 FcdevError；其他错误原样返回。
 * @param error 捕获的错误。
 * @returns 转换后的错误。
 */
function convertDebugError(error: unknown): unknown {
  if (error instanceof EmberRequestError) {
    return new FcdevError(`Ember 拒绝了请求：${error.message}（${error.code}）`, {
      code: 'EMULATOR_REQUEST_FAILED',
      hint: '检查参数与机器状态（有没有载入程序、是否已通电）',
      cause: error
    });
  }
  if (error instanceof EmberConnectionError) {
    return new FcdevError(`与 Ember 的连接出错：${error.message}`, {
      code: 'EMULATOR_CONNECTION_LOST',
      hint: 'Ember 可能已退出；用 emu_connect 重新连接，或用 emu_launch 重新启动',
      cause: error
    });
  }
  return error;
}

/** 模拟器操作的实现。 */
export class CoreEmulatorOperations implements EmulatorOperations {
  /** @inheritdoc */
  readonly operationNames = EMULATOR_OPERATION_NAMES;

  private readonly store: EmulatorSessionStore;

  /** @param env 外部能力。 */
  constructor(private readonly env: EmulatorEnvironment) {
    this.store = new EmulatorSessionStore((pid) => env.processes.terminate(pid));
  }

  /** @inheritdoc */
  async run(operation: string, params: Record<string, unknown>): Promise<Record<string, unknown>> {
    const implementation = Object.hasOwn(OPERATIONS, operation) ? OPERATIONS[operation] : undefined;
    if (implementation === undefined) {
      throw new FcdevError(`未知的操作：${operation}`, { code: 'EMULATOR_OPERATION_UNKNOWN', hint: `可用的操作：${EMULATOR_OPERATION_NAMES.join('、')}` });
    }
    if (params === null || typeof params !== 'object' || Array.isArray(params)) {
      throw new FcdevError('参数必须是对象', { code: 'EMULATOR_PARAM_INVALID' });
    }
    try {
      return await implementation({ env: this.env, store: this.store }, params);
    } catch (error) {
      throw convertDebugError(error);
    }
  }

  /** @inheritdoc */
  shutdown(): Promise<void> {
    this.store.closeSession({ terminate: true });
    return Promise.resolve();
  }
}
