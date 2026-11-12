// ------------------------------------------------------------------------
// 名称：errors.mjs
// 说明：create-fcdev 的统一错误类型：带错误码与下一步建议。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-10
// 备注：创建包是独立发布的，不引用 fcdev 主包，所以有自己的错误类型。
// ------------------------------------------------------------------------

/** 可预期的使用错误（参数有误、目录不为空、没有这个模板等） */
export class CreateError extends Error {
  /**
   * @param {string} message 说明发生了什么
   * @param {{code?: string, hint?: string, cause?: unknown}} [options] code 为错误码；hint 为下一步怎么做；cause 为原始原因
   */
  constructor(message, { code = 'CREATE_ERROR', hint = '', cause } = {}) {
    super(message, cause === undefined ? undefined : { cause });
    this.name = 'CreateError';
    this.code = code;
    this.hint = hint;
  }
}
