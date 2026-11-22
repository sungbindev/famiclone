/*
 * ------------------------------------------------------------------------
 * 名称：Sidebar.tsx
 * 说明：侧边栏：首页链接，然后是各分区的折叠列表；当前页所在的分区默认展开，当前页高亮。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-23
 * 备注：折叠用原生的 details，不需要脚本；分区的第一个页面就是分区首页。
 * ------------------------------------------------------------------------
 */

import type { NavSection } from './types';

/**
 * 侧边栏。
 * @param props 分区、首页链接与首页是否为当前页。
 */
export function Sidebar({ nav, home, active }: { nav: NavSection[]; home: string; active: boolean }) {
  return (
    <aside className="site-sidebar">
      <nav aria-label="帮助文档导航">
        <a className={active ? 'site-nav__home site-nav__link--active' : 'site-nav__home'} href={home} aria-current={active ? 'page' : undefined}>
          首页
        </a>
        {nav.map((section) => (
          <details key={section.title} className="site-nav__section" open={section.open}>
            <summary>{section.title}</summary>
            {section.groups.map((group, index) => (
              <div key={`${section.title}-${index}`} className="site-nav__group">
                {group.title ? <p className="site-nav__group-title">{group.title}</p> : null}
                <ul className="site-nav__list">
                  {group.items.map((item) => (
                    <li key={item.href}>
                      <a className={item.active ? 'site-nav__link site-nav__link--active' : 'site-nav__link'} href={item.href} aria-current={item.active ? 'page' : undefined}>
                        {item.label}
                      </a>
                    </li>
                  ))}
                </ul>
              </div>
            ))}
          </details>
        ))}
      </nav>
    </aside>
  );
}
