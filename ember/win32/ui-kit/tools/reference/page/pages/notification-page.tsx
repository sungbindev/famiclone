/*
 * ------------------------------------------------------------------------
 * 名称：notification-page.tsx
 * 说明：通知参考页：用 rujian-ui-kit 的真实通知（弹出的通知、通知按钮、通知面板）画出与 gallery 的 notification、notification-center 页一样的内容。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-19
 * 备注：页面挂载后用 useNotify 发三条通知；面板页再把面板打开；样本依次是弹出的通知（或状态栏与通知按钮），面板与各条历史排在其后（与 gallery 输出的位置一致）；
 *       通知的内容与顺序必须与 gallery/pages.cpp 的 sendSampleNotices 一致，改一边要同步另一边。
 * ------------------------------------------------------------------------
 */

import { useEffect } from 'react';
import { NotificationCenter, NotificationCenterButton, StatusBar, useNotificationCenter, useNotify, type NotifyFunction } from '@rujianjs/ui-kit';

/** 发三条通知：短消息、会换行的警告、超过三行被截断的错误。 */
function sendSampleNotices(notify: NotifyFunction): void {
  notify('已保存设置。', 'info');
  notify('这是一条较长的警告消息，需要换行显示：卡带的存档目录无法写入，请检查目录权限后重试。', 'warning');
  notify('保存失败\n原因：磁盘已满。请释放空间后重试。' + '详情'.repeat(18), 'error');
}

/** 通知页：弹出的通知。 */
export function NotificationPage() {
  const notify = useNotify();
  useEffect(() => {
    sendSampleNotices(notify);
  }, [notify]);
  return <div />;
}

/** 通知面板页：状态栏上的通知按钮与打开的通知面板。 */
export function NotificationCenterPage() {
  const notify = useNotify();
  const center = useNotificationCenter();
  useEffect(() => {
    sendSampleNotices(notify);
    center.setOpen(true);
    // 只在挂载时执行一次
    // eslint-disable-next-line react-hooks/exhaustive-deps
  }, []);
  return (
    <>
      <StatusBar end={<NotificationCenterButton />} />
      <NotificationCenter />
    </>
  );
}
