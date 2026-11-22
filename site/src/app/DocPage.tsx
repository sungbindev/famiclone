/*
 * ------------------------------------------------------------------------
 * 名称：DocPage.tsx
 * 说明：帮助页：面包屑、标题、页内目录（标题较多时）、正文，最后是上一页与下一页。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-23
 * 备注：正文是构建时由 Markdown 渲染好的 HTML，原样放进页面；页内目录只列二、三级标题。
 * ------------------------------------------------------------------------
 */

import { Panel, Title } from '@rujianjs/ui-kit';
import type { DocView, PageView } from './types';

/** 页内目录至少有这么多二级标题才显示。 */
const TOC_MIN_SECTIONS = 3;

/**
 * 帮助页。
 * @param props 视图与帮助页内容。
 */
export function DocPage({ view, doc }: { view: PageView; doc: DocView }) {
  const sectionCount = doc.toc.filter((item) => item.level === 2).length;
  return (
    <div className="doc-page">
      <nav className="site-crumbs" aria-label="当前位置">
        <a href={view.home}>首页</a>
        <span aria-hidden="true">›</span>
        <a href={doc.sectionHref}>{doc.sectionTitle}</a>
        <span aria-hidden="true">›</span>
        <span aria-current="page">{doc.label}</span>
      </nav>
      <header className="doc-head">
        <p className="kicker">{doc.sectionTitle}</p>
        <Title as="h1" className="doc-title">
          {doc.heading}
        </Title>
      </header>
      {sectionCount >= TOC_MIN_SECTIONS ? (
        <Panel as="nav" title="本页目录" className="doc-toc" aria-label="本页目录">
          <ul>
            {doc.toc.map((item) => (
              <li key={item.id} className={item.level === 3 ? 'toc-sub' : undefined}>
                <a href={`#${item.id}`}>{item.text}</a>
              </li>
            ))}
          </ul>
        </Panel>
      ) : null}
      <article className="doc" dangerouslySetInnerHTML={{ __html: doc.bodyHtml }} />
      <nav className="doc-pager" aria-label="上一页与下一页">
        {doc.prev ? (
          <a href={doc.prev.href} className="doc-pager__prev">
            <span>上一页 · {doc.prev.section}</span>
            {doc.prev.title}
          </a>
        ) : (
          <span />
        )}
        {doc.next ? (
          <a href={doc.next.href} className="doc-pager__next">
            <span>下一页 · {doc.next.section}</span>
            {doc.next.title}
          </a>
        ) : null}
      </nav>
    </div>
  );
}
