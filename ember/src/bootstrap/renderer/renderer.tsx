/*
 * ------------------------------------------------------------------------
 * 名称：renderer.tsx
 * 说明：界面入口：挂载组件库根组件与外壳，主题跟随系统；对话框用独立的系统窗口显示（组件库的 dialogDisplay="window"），主题类随主窗口同步到对话框窗口。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-13
 * 备注：组件库的令牌、主题与基础样式必须经 @rujianjs/ui-kit/styles.css 引入，打包时不会随组件一起保留；
 *       主题由 html 元素上的 theme-dark、theme-light 类决定，首选项里的主题设置做好后改由设置决定。
 * ------------------------------------------------------------------------
 */

import { StrictMode } from 'react';
import { createRoot } from 'react-dom/client';
import { THEME_NAMES, themeClassName, UiProvider, type ThemeName } from '@rujianjs/ui-kit';
import '@rujianjs/ui-kit/styles.css';
import { getBridge } from '../../ui/shell/bridge';
import { Shell } from '../../ui/shell/Shell';
import { RENDERER_MODULES } from './module-list';

/** 给 html 元素设置主题类。 */
function applyTheme(theme: ThemeName): void {
  const root = document.documentElement;
  for (const name of THEME_NAMES) root.classList.remove(themeClassName(name));
  root.classList.add(themeClassName(theme));
}

const darkQuery = window.matchMedia('(prefers-color-scheme: dark)');
applyTheme(darkQuery.matches ? 'dark' : 'light');
darkQuery.addEventListener('change', (event) => applyTheme(event.matches ? 'dark' : 'light'));

const container = document.getElementById('root');
if (container === null) throw new Error('界面根元素不存在');

createRoot(container).render(
  <StrictMode>
    <UiProvider dialogDisplay="window">
      <Shell bridge={getBridge()} modules={RENDERER_MODULES} />
    </UiProvider>
  </StrictMode>
);
