/*
 * ------------------------------------------------------------------------
 * 名称：index.ts
 * 说明：帮助文档站点的主题入口：在 VitePress 默认主题上加载组件库的令牌与主题、标题栏样式和 Ember 的覆盖样式，页面顶部放与主窗口一致的标题栏。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-17
 * 备注：@ember-ui-kit 是 config.mts 里指向组件库源码目录的别名；深浅主题类（theme-dark、theme-light）由 config.mts 的 head 脚本在页面绘制前写到 html 上。
 * ------------------------------------------------------------------------
 */

import type { Theme } from 'vitepress';
import DefaultTheme from 'vitepress/theme';
import { h } from 'vue';
import '@ember-ui-kit/styles/index.css';
import '@ember-ui-kit/components/title-bar/ui-title-bar.css';
import './style.css';
import { HelpTitleBar } from './HelpTitleBar';

export default {
  extends: DefaultTheme,
  Layout: () => h(DefaultTheme.Layout, null, { 'layout-top': () => h(HelpTitleBar) })
} satisfies Theme;
