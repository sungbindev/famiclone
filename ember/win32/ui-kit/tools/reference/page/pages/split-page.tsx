/*
 * ------------------------------------------------------------------------
 * 名称：split-page.tsx
 * 说明：分栏视图参考页：用 rujian-ui-kit 的真实 SplitView 画出与 gallery 的 split 页一样的内容。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-19
 * 备注：高 160 的外框里放分栏视图；左栏是十二条交替颜色的色带（每条 24 高，会滚动），主区是铺满的底色块；
 *       左栏初始 140、最小 100、最大 200；色带与颜色必须与 gallery/pages.cpp 一致，改一边要同步另一边。
 * ------------------------------------------------------------------------
 */

import { SplitView, Stack } from '@rujianjs/ui-kit';

/** 色带的数量与高度（像素）。 */
const BANDS = 12;
const BAND_HEIGHT = 24;
/** 分栏视图外框的高度（像素）。 */
const VIEW_HEIGHT = 160;

/** 分栏视图页。 */
export function SplitPage() {
  const side = Array.from({ length: BANDS }, (_, index) => (
    <div key={index} style={{ height: BAND_HEIGHT, background: index % 2 === 0 ? 'var(--accent)' : 'var(--input-bg)' }} />
  ));
  return (
    <Stack className="reference-page">
      <div style={{ height: VIEW_HEIGHT }}>
        <SplitView side={side} sideLabel="左栏" defaultSideWidth={140} minSideWidth={100} maxSideWidth={200}>
          <div style={{ background: 'var(--input-bg)' }} />
        </SplitView>
      </div>
    </Stack>
  );
}
