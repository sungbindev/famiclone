// ------------------------------------------------------------------------
// 名称：logger.ts
// 说明：日志接口：逻辑层与入口层只依赖它，不关心日志写到哪里。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-10
// 备注：实现见 output-channel-logger.ts。
// ------------------------------------------------------------------------

/** 日志。 */
export interface Logger {
  /**
   * 记录一条普通信息。
   * @param message 信息。
   */
  info(message: string): void;

  /**
   * 记录一条错误。
   * @param message 发生了什么。
   * @param error 捕获到的错误，可以没有。
   */
  error(message: string, error?: unknown): void;
}
