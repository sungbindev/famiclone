// ------------------------------------------------------------------------
// 名称：fcdev-error.ts
// 说明：构建、目标、打包等核心功能的可预期错误：带错误码与下一步建议。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-22
// 备注：属于可预期错误，入口层只展示说明与建议，不当作程序缺陷记日志。
// ------------------------------------------------------------------------

import { UserFacingError } from './user-facing-error';

/** 创建 FcdevError 时的可选项。 */
export interface FcdevErrorOptions {
  /** 错误码，供调用者区分原因。 */
  readonly code?: string;
  /** 下一步怎么做。 */
  readonly hint?: string;
  /** 引起本错误的底层错误。 */
  readonly cause?: unknown;
}

/** 配置有误、找不到工具、构建失败等可预期的使用错误。 */
export class FcdevError extends UserFacingError {
  /** 错误码。 */
  readonly code: string;

  /** 下一步怎么做；没有建议时为空字符串。 */
  readonly hint: string;

  /**
   * 创建错误。
   * @param message 说明发生了什么。
   * @param options 错误码、建议与底层原因。
   */
  constructor(message: string, options: FcdevErrorOptions = {}) {
    super(message, options.cause);
    this.name = 'FcdevError';
    this.code = options.code ?? 'FCDEV_ERROR';
    this.hint = options.hint ?? '';
  }
}
