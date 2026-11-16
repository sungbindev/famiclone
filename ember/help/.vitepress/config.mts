/*
 * ------------------------------------------------------------------------
 * 名称：config.mts
 * 说明：Ember 帮助文档站点的 VitePress 配置：中文界面、两个分类（使用说明、调试接口手册）的导航与侧边栏、离线搜索，构建输出到仓库根目录的 build/ember/help。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-17
 * 备注：help/api 由 npm run help:build 先生成（调试接口的各模块页，不提交），侧边栏里的模块按 help/api/index.md 里的顺序列出；
 *       站点由主进程经自定义协议 ember-help:// 提供，只能用 .html 结尾的链接（cleanUrls 保持关闭）；外观跟随系统，不提供切换按钮。
 * ------------------------------------------------------------------------
 */

import { existsSync, readFileSync } from 'node:fs';
import { resolve } from 'node:path';
import { fileURLToPath } from 'node:url';
import { defineConfig } from 'vitepress';
import { HELP_BUILD_DIRECTORY } from '../../tools/build/build-paths.mjs';
import { uiKitDirectory } from '../../ui-kit-source.mts';

/** 站点源码目录（ember/help）。 */
const HELP_ROOT = fileURLToPath(new URL('..', import.meta.url));

/** 给 html 写组件库的主题类（与主窗口一样跟随系统深浅色），同时同步 VitePress 的 dark 类，在页面绘制前执行，避免先显示错误的颜色。 */
const THEME_CLASS_SCRIPT =
  "(function(){var q=matchMedia('(prefers-color-scheme: dark)'),r=document.documentElement;function a(){r.classList.remove('theme-dark','theme-light');r.classList.add(q.matches?'theme-dark':'theme-light');r.classList.toggle('dark',q.matches)}a();q.addEventListener('change',a)})()";

/** 用户可见的文字。 */
const TEXT = {
  title: 'Ember 帮助文档',
  description: 'Ember（FC 与学习机模拟器）的使用说明与调试接口手册',
  manual: '使用说明',
  debug: '调试接口手册',
  missingApi: '找不到 help/api：先运行 npm run help:build 生成调试接口的各模块页'
} as const;

/** 调试接口手册里各模块页的侧边栏项，顺序取自总览页里的模块列表。 */
function apiSidebarItems(): { text: string; link: string }[] {
  const overview = `${HELP_ROOT}api/index.md`;
  if (!existsSync(overview)) throw new Error(TEXT.missingApi);
  const modules = [...readFileSync(overview, 'utf8').matchAll(/^### \[([a-z0-9-]+)\]\(\1\.md\)$/gm)].map((match) => match[1] as string);
  return modules.map((module) => ({ text: module, link: `/api/${module}` }));
}

/** 使用说明的侧边栏。 */
const MANUAL_SIDEBAR = [
  {
    text: TEXT.manual,
    items: [
      { text: '简介', link: '/manual/' },
      { text: '快速开始', link: '/manual/start' },
      { text: '窗口、菜单与快捷键', link: '/manual/window' },
      { text: '机型与 BIOS', link: '/manual/machines' },
      { text: '卡带、软盘、磁带与打印机', link: '/manual/media' },
      { text: '键盘、手柄与鼠标', link: '/manual/input' },
      { text: '画面与声音', link: '/manual/display-sound' },
      { text: 'mapper 插件', link: '/manual/plugins' },
      { text: '首选项', link: '/manual/preferences' },
      { text: '常见问题', link: '/manual/faq' }
    ]
  }
];

/** 调试接口手册的侧边栏。 */
function debugSidebar() {
  return [
    {
      text: '调试服务',
      items: [
        { text: '概述', link: '/debug/' },
        { text: '连接与报文', link: '/debug/protocol' },
        { text: '运行控制与典型流程', link: '/debug/control' },
        { text: '客户端工具', link: '/debug/clients' }
      ]
    },
    { text: '请求与事件', items: [{ text: '总览', link: '/api/' }, ...apiSidebarItems()] }
  ];
}

export default defineConfig({
  lang: 'zh-CN',
  title: TEXT.title,
  description: TEXT.description,
  outDir: HELP_BUILD_DIRECTORY,
  appearance: 'force-auto',
  head: [['script', {}, THEME_CLASS_SCRIPT]],
  // 主题里直接引用组件库源码的令牌、主题与标题栏样式，位置与界面打包用的一致（见 ui-kit-source.mts）
  vite: { resolve: { alias: [{ find: /^@ember-ui-kit\//, replacement: `${resolve(uiKitDirectory(), 'src').replaceAll('\\', '/')}/` }] } },
  markdown: { theme: { light: 'github-light', dark: 'github-dark' } },
  themeConfig: {
    siteTitle: false,
    nav: [
      { text: TEXT.manual, link: '/manual/', activeMatch: '^/manual/' },
      { text: TEXT.debug, link: '/debug/', activeMatch: '^/(debug|api)/' }
    ],
    sidebar: { '/manual/': MANUAL_SIDEBAR, '/debug/': debugSidebar(), '/api/': debugSidebar() },
    outline: { level: [2, 3], label: '本页内容' },
    docFooter: { prev: '上一页', next: '下一页' },
    returnToTopLabel: '回到顶部',
    sidebarMenuLabel: '菜单',
    search: {
      provider: 'local',
      options: {
        translations: {
          button: { buttonText: '搜索', buttonAriaLabel: '搜索' },
          modal: {
            displayDetails: '显示详细列表',
            resetButtonTitle: '清除查询条件',
            backButtonTitle: '关闭搜索',
            noResultsText: '没有找到结果',
            footer: { selectText: '选择', selectKeyAriaLabel: '回车', navigateText: '切换', navigateUpKeyAriaLabel: '上箭头', navigateDownKeyAriaLabel: '下箭头', closeText: '关闭', closeKeyAriaLabel: 'Esc' }
          }
        }
      }
    }
  }
});
