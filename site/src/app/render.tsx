/*
 * ------------------------------------------------------------------------
 * 名称：render.tsx
 * 说明：站点的渲染入口：把一页的视图渲染成 HTML 字符串，构建脚本经 Vite 的 SSR 调用它。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-23
 * 备注：渲染只在构建时发生，组件里不访问 window 与 document；页面的交互只有折叠的分区与链接，不需要脚本。
 * ------------------------------------------------------------------------
 */

import { renderToStaticMarkup } from 'react-dom/server';
import { Document } from './Document';
import type { PageView } from './types';

/**
 * 渲染一页（不含文档类型声明，由调用方加上）。
 * @param view 页面视图。
 * @returns HTML 字符串。
 */
export function renderPage(view: PageView): string {
  return renderToStaticMarkup(<Document view={view} />);
}
