/*
 * ------------------------------------------------------------------------
 * 名称：RuntimePage.tsx
 * 说明：运行页：上方是画面，下方是开发期操作条；画面区可聚焦，页面显示时自动聚焦，键盘事件采集后经输入端口送给核心。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-14
 * 备注：先放进最小外壳的主区域，阶段 3a 外壳按原型重做时，操作条迁移到边栏；画面比例与缩放方式先用组件的默认值，首选项页面做好后再接。
 *       必须放在组件库根组件 UiProvider 之内（通知需要它）。
 * ------------------------------------------------------------------------
 */

import { useRef } from 'react';
import type { ShellPageProps } from '../shell/shell-registry';
import { RuntimeControls } from './RuntimeControls';
import { RuntimeScreen } from './RuntimeScreen';
import './ember-run.css';

/**
 * 运行页。
 * @param props 属性。
 */
export function RuntimePage({ bridge }: ShellPageProps) {
  const screenRef = useRef<HTMLDivElement>(null);
  return (
    <div className="ember-run">
      <RuntimeScreen bridge={bridge} screenRef={screenRef} />
      <RuntimeControls bridge={bridge} screenRef={screenRef} />
    </div>
  );
}
