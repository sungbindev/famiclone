/*
 * ------------------------------------------------------------------------
 * 名称：HelpTitleBar.ts
 * 说明：帮助文档窗口的标题栏：标志、应用名与副名，结构与类名同主窗口的标题栏，样式直接用组件库的 ui-title-bar。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-17
 * 备注：窗口隐藏了系统标题栏，右侧给系统窗口按钮留的位置由组件库样式按 titlebar-area-* 环境变量处理；标志与主窗口用同一张图。
 * ------------------------------------------------------------------------
 */

import { h } from 'vue';
import logoUrl from '../../../src/ui/shell/ember-logo.png';

/** 用户可见的文字。 */
const TEXT = {
  title: 'EMBER',
  subtitle: ' 帮助文档'
} as const;

/** 帮助文档窗口的标题栏。 */
export function HelpTitleBar() {
  return h('header', { class: 'ui-title-bar' }, [
    h('div', { class: 'ui-title-bar__brand' }, [
      h('span', { class: 'ui-title-bar__logo', 'aria-hidden': 'true' }, [h('img', { src: logoUrl, alt: '', draggable: false })]),
      h('span', { class: 'ui-title-bar__title' }, [TEXT.title, h('span', { class: 'ui-title-bar__subtitle' }, TEXT.subtitle)])
    ])
  ]);
}
