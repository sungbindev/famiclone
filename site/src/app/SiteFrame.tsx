/*
 * ------------------------------------------------------------------------
 * 名称：SiteFrame.tsx
 * 说明：站点框架：顶部标题栏（组件库的 TitleBar，右侧是顶部链接），下面左右分栏：左边侧边栏，右边是首页或帮助页，底部是页脚。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-23
 * 备注：标题栏与侧边栏都用组件库的令牌与样式；窄屏时侧边栏改到正文上方。
 * ------------------------------------------------------------------------
 */

import { TitleBar } from '@rujianjs/ui-kit';
import { DocPage } from './DocPage';
import { HomePage } from './HomePage';
import { Sidebar } from './Sidebar';
import type { PageView } from './types';

/** 标题栏的应用名与副名。 */
const BRAND = { title: 'Ember 与 fcdev', subtitle: '6502 模拟器与开发工具' };

/**
 * 站点框架。
 * @param props 视图。
 */
export function SiteFrame({ view }: { view: PageView }) {
  const actions = (
    <nav className="site-top-links" aria-label="站点链接">
      {view.topLinks.map((link) => (
        <a key={link.href} className="ui-button ui-button--ghost" href={link.href}>
          <span>{link.label}</span>
        </a>
      ))}
      <a className="ui-button ui-button--ghost" href={view.repository} target="_blank" rel="noopener">
        <span>GitHub</span>
      </a>
    </nav>
  );
  return (
    <div className="site-frame">
      <TitleBar logo={<img src={view.logo} alt="" draggable={false} />} title={BRAND.title} subtitle={BRAND.subtitle} actions={actions} />
      <div className="site-body">
        <Sidebar nav={view.nav} home={view.home} active={view.kind === 'home'} />
        <main className="site-main">
          {view.kind === 'home' && view.landing ? <HomePage view={view} landing={view.landing} /> : null}
          {view.kind === 'doc' && view.doc ? <DocPage view={view} doc={view.doc} /> : null}
        </main>
      </div>
      <footer className="site-footer">
        Ember 以 GPLv3 发布，fcdev 扩展以 Apache-2.0 发布。源码与问题反馈见 <a href={view.repository} target="_blank" rel="noopener">GitHub 仓库</a>。
      </footer>
    </div>
  );
}
