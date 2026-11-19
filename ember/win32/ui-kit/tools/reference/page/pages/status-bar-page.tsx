/*
 * ------------------------------------------------------------------------
 * 名称：status-bar-page.tsx
 * 说明：状态栏参考页：用 rujian-ui-kit 的真实状态栏画出与 gallery 的 status-bar、status-bar-large 页一样的内容。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-19
 * 备注：样本依次是状态栏与它的各个状态项；大尺寸是根元素上的类（定义在 AppFrame 的样式里，所以这里单独引入），由入口按页名加类；
 *       样本顺序与内容必须与 gallery/pages.cpp 的 buildStatusBarPage 一致，改一边要同步另一边。
 * ------------------------------------------------------------------------
 */

import '@kit-source/components/app-frame/ui-app-frame.css';
import { iconBell, iconDeviceFloppy, iconInfoCircle, StatusBar, StatusBarItem } from '@rujianjs/ui-kit';

const noop = () => undefined;

/** 状态栏页。 */
export function StatusBarPage() {
  return (
    <StatusBar
      start={
        <>
          <StatusBarItem text="运行中" dot="success" />
          <StatusBarItem text="已暂停" tone="warning" />
          <StatusBarItem text="调试模式" tone="success" onClick={noop} />
          <StatusBarItem text="这是一个很长很长很长很长很长很长的文件名称.nes" />
        </>
      }
      end={
        <>
          <StatusBarItem text="软驱" lamp={{ lit: true, label: '灯' }} />
          <StatusBarItem text="磁盘" icon={iconDeviceFloppy} lamp={{ lit: true, label: '读写灯', inIcon: { x: 50, y: 60 } }} />
          <StatusBarItem text="通知" icon={iconBell} iconOnly pressed onClick={noop} />
          <StatusBarItem text="提示" icon={iconInfoCircle} iconOnly onClick={noop} />
        </>
      }
    />
  );
}
