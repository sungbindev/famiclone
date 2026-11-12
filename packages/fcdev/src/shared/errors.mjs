// ------------------------------------------------------------------------
// 名称：errors.mjs
// 说明：fcdev 的统一错误类型：带错误码与下一步建议，命令行据此给用户输出可操作的提示。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-10
// 备注：无
// ------------------------------------------------------------------------

/**
 * 可预期的使用错误（配置有误、找不到工具、构建失败等）。
 * 程序自身的缺陷不用这个类，直接抛出原始异常。
 */
export class FcdevError extends Error {
  /**
   * @param {string} message 说明发生了什么
   * @param {{code?: string, hint?: string, cause?: unknown}} [options] code 为错误码；hint 为下一步怎么做；cause 为原始原因
   */
  constructor(message, { code = 'FCDEV_ERROR', hint = '', cause } = {}) {
    super(message, cause === undefined ? undefined : { cause });
    this.name = 'FcdevError';
    this.code = code;
    this.hint = hint;
  }
}
