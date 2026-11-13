/*
 * ------------------------------------------------------------------------
 * 名称：sender-check.ts
 * 说明：IPC 发送方检查：只接受来自应用自身页面的消息，其他来源一律拒绝。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-13
 * ------------------------------------------------------------------------
 */

import type { IpcMainEvent, IpcMainInvokeEvent } from 'electron';

/** 判断 IPC 消息是否来自应用自身的页面。 */
export type SenderCheck = (event: IpcMainEvent | IpcMainInvokeEvent) => boolean;

/**
 * 创建发送方检查。
 * @param isAppUrl 判断地址是否属于应用自身。
 * @returns 发送方检查。
 */
export function createSenderCheck(isAppUrl: (url: string) => boolean): SenderCheck {
  return (event) => {
    const url = event.senderFrame?.url;
    return typeof url === 'string' && isAppUrl(url);
  };
}
