/*
 * ------------------------------------------------------------------------
 * 名称：open-help.ts
 * 说明：system.openHelp 的处理：请应用内的帮助文档窗口打开（已经打开时提到最前），打不开时返回带说明的错误。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-17
 * 备注：文档的位置由主进程决定，请求没有参数；实际的原因写进说明，界面用通知显示。
 * ------------------------------------------------------------------------
 */

import type { MainResult } from '../../../contracts/core-gateway/generated/core-api';
import { CoreRequestError } from '../../../contracts/core-gateway/core-messages';
import type { HelpWindow } from '../../../contracts/shell/help-window';
import type { MainRequestHandler } from '../core/main-request-ipc';

/** 错误码。 */
const HELP_UNAVAILABLE = 'system.helpUnavailable';

/** 用户可见的文字。 */
const TEXT = {
  unavailable: (reason: string) => `无法打开帮助文档：${reason}`
} as const;

/** 打开帮助文档处理所需的依赖。 */
export interface OpenHelpOptions {
  readonly helpWindow: Pick<HelpWindow, 'open'>;
}

/**
 * 创建 system.openHelp 的处理函数。
 * @param options 依赖。
 * @returns 处理函数。
 */
export function createOpenHelpHandler(options: OpenHelpOptions): MainRequestHandler {
  return async (): Promise<MainResult<'system.openHelp'>> => {
    try {
      await options.helpWindow.open();
    } catch (error) {
      throw new CoreRequestError({ code: HELP_UNAVAILABLE, message: TEXT.unavailable(error instanceof Error ? error.message : String(error)) });
    }
    return {};
  };
}
