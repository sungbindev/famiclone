/*
 * ------------------------------------------------------------------------
 * 名称：Document.tsx
 * 说明：页面的外壳：html 与 head（标题、图标、样式、主题脚本），body 里是站点框架。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-23
 * 备注：html 默认带 theme-light，主题脚本在页面绘制前按系统的深浅色改成 theme-dark 或 theme-light，避免先显示错误的颜色。
 * ------------------------------------------------------------------------
 */

import { SiteFrame } from './SiteFrame';
import type { PageView } from './types';

/** 跟随系统深浅色写主题类（与 Ember 帮助站点一致），在页面绘制前执行。 */
const THEME_SCRIPT =
  "(function(){var q=matchMedia('(prefers-color-scheme: dark)'),r=document.documentElement;function a(){r.classList.remove('theme-dark','theme-light');r.classList.add(q.matches?'theme-dark':'theme-light');r.classList.toggle('dark',q.matches)}a();q.addEventListener('change',a)})()";

/**
 * 页面根元素。
 * @param props 视图。
 */
export function Document({ view }: { view: PageView }) {
  return (
    <html lang="zh-CN" className="theme-light">
      <head>
        <meta charSet="utf-8" />
        <meta name="viewport" content="width=device-width, initial-scale=1" />
        <meta name="description" content={view.description} />
        <title>{view.title}</title>
        <link rel="icon" href={view.icon} />
        <link rel="stylesheet" href={view.css} />
        <script dangerouslySetInnerHTML={{ __html: THEME_SCRIPT }} />
      </head>
      <body className="site">
        <SiteFrame view={view} />
      </body>
    </html>
  );
}
