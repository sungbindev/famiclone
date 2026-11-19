/*
 * ------------------------------------------------------------------------
 * 名称：scroll-page.tsx
 * 说明：滚动区域参考页：用 rujian-ui-kit 的真实滚动区域画出与 gallery 的 scroll 页一样的内容。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-19
 * 备注：内容是十二条交替颜色的色带；样本由截图脚本按“滚动区域外框”的选择器找出；
 *       色带的数量、高度与颜色必须与 gallery/pages.cpp 的 Bands 一致，改一边要同步另一边。
 * ------------------------------------------------------------------------
 */

import { ScrollArea, Stack } from '@rujianjs/ui-kit';

/** 色带的数量与高度（像素）。 */
const BANDS = 12;
const BAND_HEIGHT = 24;

/** 滚动区域页。 */
export function ScrollPage() {
  return (
    <Stack className="reference-page">
      <ScrollArea horizontal="hidden" style={{ maxHeight: 120 }}>
        {Array.from({ length: BANDS }, (_, index) => (
          <div key={index} style={{ height: BAND_HEIGHT, background: index % 2 === 0 ? 'var(--accent)' : 'var(--input-bg)' }} />
        ))}
      </ScrollArea>
    </Stack>
  );
}
