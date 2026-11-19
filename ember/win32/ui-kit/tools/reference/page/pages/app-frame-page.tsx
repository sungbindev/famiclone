/*
 * ------------------------------------------------------------------------
 * 名称：app-frame-page.tsx
 * 说明：窗口框架参考页：用 rujian-ui-kit 的真实窗口框架（标题栏、页面区、底部面板、大尺寸状态栏）画出与 gallery 的 app-frame 页一样的内容。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-19
 * 备注：样本依次是框架、标题栏、页面区、底部面板、状态栏与状态栏的两个状态项；
 *       样本顺序与内容必须与 gallery/pages.cpp 的 buildAppFramePage 一致，改一边要同步另一边。
 * ------------------------------------------------------------------------
 */

import { AppFrame, StatusBar, StatusBarItem, TitleBar } from '@rujianjs/ui-kit';

/** 窗口框架页。 */
export function AppFramePage() {
  return (
    <AppFrame
      titleBar={<TitleBar title="Ember" subtitle="模拟器" />}
      bottomPanel={<div />}
      bottomPanelOpen
      bottomPanelHeight={60}
      bottomPanelMinHeight={60}
      bottomPanelResizable={false}
      statusBarSize="large"
      statusBar={<StatusBar start={<StatusBarItem text="就绪" />} end={<StatusBarItem text="60 fps" />} />}
    >
      <div />
    </AppFrame>
  );
}
