// ------------------------------------------------------------------------
// 名称：user-facing-error.ts
// 说明：可预期的错误：说明文字直接展示给用户，不当作程序缺陷写入错误日志。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-10
// 备注：非预期的错误（代码缺陷）不用这个类，由入口层记录日志并给出通用提示。
// ------------------------------------------------------------------------

/** 可预期的错误，如项目里没有安装 fcdev、找不到 node。 */
export class UserFacingError extends Error {
  /**
   * 创建错误。
   * @param message 展示给用户的说明文字。
   * @param cause 引起本错误的底层错误。
   */
  constructor(message: string, cause?: unknown) {
    super(message, cause === undefined ? undefined : { cause });
    this.name = 'UserFacingError';
  }
}
