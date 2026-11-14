/*
 * ------------------------------------------------------------------------
 * 名称：RuntimePage.tsx
 * 说明：运行页：画面区占满页面，没有开机、断电、暂停时在画面上覆盖提示；画面区可聚焦，页面显示时自动聚焦，键盘事件采集后经输入端口送给核心。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-14
 * 备注：由外壳的页面区挂载（登记在 runtime-module.tsx），机器的信息与控制在边栏（RuntimeSideBar）；画面比例与缩放方式先用组件的默认值，首选项页面做好后再接。
 *       必须放在组件库根组件 UiProvider 之内（通知需要它）。
 * ------------------------------------------------------------------------
 */

import { useEffect, useRef } from 'react';
import type { ShellPageProps } from '../shell/shell-registry';
import type { RuntimeController } from './runtime-controller';
import { RuntimeScreen } from './RuntimeScreen';
import { ScreenOverlay } from './ScreenOverlay';
import './ember-run.css';

/** 运行页的属性。 */
export interface RuntimePageProps extends ShellPageProps {
  readonly controller: RuntimeController;
}

/**
 * 运行页。
 * @param props 属性。
 */
export function RuntimePage({ bridge, controller }: RuntimePageProps) {
  const screenRef = useRef<HTMLDivElement>(null);
  // 边栏上的操作结束后，控制器经它把焦点还给画面，键盘才能继续控制游戏。
  useEffect(() => {
    controller.setScreenFocus(() => screenRef.current?.focus({ preventScroll: true }));
    return () => controller.setScreenFocus(undefined);
  }, [controller]);
  return (
    <div className="ember-run">
      <RuntimeScreen bridge={bridge} screenRef={screenRef} />
      <ScreenOverlay controller={controller} />
    </div>
  );
}
