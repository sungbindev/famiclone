// ------------------------------------------------------------------------
// 名称：disasm-runner.ts
// 说明：运行 fcdev 反汇编操作的外部能力接口：按操作名与参数调用工作区里安装的 fcdev，
//       返回可序列化的结果。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-21
// 备注：由实现层提供；入口层只经这个接口使用 fcdev，不关心它怎么加载。
// ------------------------------------------------------------------------

/** 运行 fcdev 的反汇编操作。 */
export interface DisasmRunner {
  /**
   * 运行一个操作。
   * @param startDirectory 查找 node_modules/fcdev 的起点目录，一般是工程目录。
   * @param operation 操作名，如 trace、view。
   * @param params 操作的参数，路径都已经是绝对路径。
   * @returns 操作的结果，是普通的可序列化对象。
   * @throws UserFacingError 没有安装 fcdev、版本过低，或操作本身失败（说明与建议已写进错误信息）。
   */
  run(startDirectory: string, operation: string, params: Record<string, unknown>): Promise<unknown>;
}
