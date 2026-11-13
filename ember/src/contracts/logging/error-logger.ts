/*
 * ------------------------------------------------------------------------
 * 名称：error-logger.ts
 * 说明：系统错误日志的接口：主进程各层经它写错误，不直接操作日志文件。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-13
 * 备注：对应核心的 platform/logging/error-log.hpp，写入同一个日志目录；写日志失败不影响业务，也不再写日志。
 * ------------------------------------------------------------------------
 */

/** 一条系统错误。 */
export interface ErrorLogEntry {
  /** 发生的进程。 */
  readonly process: 'main' | 'renderer';
  /** 所属模块，无法确定时省略。 */
  readonly module?: string;
  /** 错误码。 */
  readonly code: string;
  /** 说明。 */
  readonly message: string;
  /** 调用栈。 */
  readonly stack?: string;
  /** 排查需要的上下文。 */
  readonly context?: Readonly<Record<string, unknown>>;
}

/** 系统错误日志。 */
export interface ErrorLogger {
  /** 写入一条错误；不抛出异常。 */
  write(entry: ErrorLogEntry): void;
}
