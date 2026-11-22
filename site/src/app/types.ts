/*
 * ------------------------------------------------------------------------
 * 名称：types.ts
 * 说明：站点页面的视图类型：构建脚本算好的链接、导航、正文与首页内容，交给组件渲染。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-23
 * 备注：所有链接都是相对当前页面的路径，组件不自己拼接路径。
 * ------------------------------------------------------------------------
 */

/** 侧边栏的一个页面项。 */
export interface NavItem {
  label: string;
  href: string;
  active: boolean;
}

/** 侧边栏的一组页面（分组标题为空时直接列出）。 */
export interface NavGroup {
  title: string | null;
  items: NavItem[];
}

/** 侧边栏的一个分区（可以折叠）。 */
export interface NavSection {
  title: string;
  href: string;
  open: boolean;
  groups: NavGroup[];
}

/** 截图：图片地址、替代文字与图注。 */
export interface ShotView {
  src: string;
  alt: string;
  caption: string;
}

/** 页内目录的一项。 */
export interface TocItem {
  level: number;
  id: string;
  text: string;
}

/** 上一页或下一页。 */
export interface PagerItem {
  title: string;
  href: string;
  section: string;
}

/** 帮助页的内容。 */
export interface DocView {
  sectionTitle: string;
  sectionHref: string;
  label: string;
  heading: string;
  bodyHtml: string;
  toc: TocItem[];
  prev?: PagerItem;
  next?: PagerItem;
}

/** 首页的内容：截图、页数与各处的链接。 */
export interface LandingView {
  hero: ShotView;
  emberShots: ShotView[];
  fcdevShots: ShotView[];
  counts: { manual: number; plugin: number; debug: number; fcdev: number };
  links: Record<string, string>;
}

/** 一页的视图。 */
export interface PageView {
  kind: 'home' | 'doc';
  title: string;
  description: string;
  css: string;
  icon: string;
  logo: string;
  home: string;
  repository: string;
  nav: NavSection[];
  doc?: DocView;
  landing?: LandingView;
}
