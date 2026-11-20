// ------------------------------------------------------------------------
// 名称：emulator-runner.ts
// 说明：运行 fcdev 模拟器操作的外部能力接口：按操作名与参数调用工作区里安装的 fcdev 的模拟器模块，
//       并在扩展停用时收尾。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-21
// 备注：模拟器的会话保存在 fcdev 模块里，跨调用保持；入口层只经这个接口使用，不关心它怎么加载。
// ------------------------------------------------------------------------

import type { DisasmRunner } from './disasm-runner';

/** 运行 fcdev 的模拟器操作。 */
export interface EmulatorRunner extends DisasmRunner {
  /**
   * 关闭模拟器会话，并结束由工具启动的 Ember；没有用过模拟器时什么也不做。
   * 只结束自己启动的进程，不影响用户自己打开的 Ember。
   */
  shutdown(): Promise<void>;
}
