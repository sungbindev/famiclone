// ------------------------------------------------------------------------
// 名称：output-channel-logger.ts
// 说明：把日志写到 VS Code“输出”面板里名为“FC Dev”的输出通道，每行带时间。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-10
// 备注：通道在扩展停用时由 dispose 释放。
// ------------------------------------------------------------------------

import * as vscode from 'vscode';
import type { Logger } from './logger';

/** 输出通道的名称。 */
const CHANNEL_NAME = 'FC Dev';

/** 提取错误的描述文字：有调用栈时带上调用栈。 */
function describeError(error: unknown): string {
  if (error instanceof Error) return error.stack ?? error.message;
  return String(error);
}

/** 写到“输出”面板的日志。 */
export class OutputChannelLogger implements Logger, vscode.Disposable {
  private readonly channel = vscode.window.createOutputChannel(CHANNEL_NAME);

  /** @inheritdoc */
  info(message: string): void {
    this.channel.appendLine(`${new Date().toLocaleTimeString()}  ${message}`);
  }

  /** @inheritdoc */
  error(message: string, error?: unknown): void {
    const detail = error === undefined ? '' : `\n${describeError(error)}`;
    this.channel.appendLine(`${new Date().toLocaleTimeString()}  [错误] ${message}${detail}`);
  }

  /** 释放输出通道。 */
  dispose(): void {
    this.channel.dispose();
  }
}
