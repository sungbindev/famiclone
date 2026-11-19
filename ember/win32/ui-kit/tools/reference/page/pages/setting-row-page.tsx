/*
 * ------------------------------------------------------------------------
 * 名称：setting-row-page.tsx
 * 说明：设置行参考页：用 rujian-ui-kit 的真实 SettingRow 画出与 gallery 的 setting-row 页一样的内容；宽、窄两个页面共用，宽窄由页面（窗口）宽度决定。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-19
 * 备注：三行：名称加说明、只有名称、控件下方带错误文字；控件是没有文字的滑块；文字必须与 gallery/pages.cpp 一致。
 * ------------------------------------------------------------------------
 */

import { SettingRow, Slider, Stack } from '@rujianjs/ui-kit';

/** 设置行页。 */
export function SettingRowPage() {
  return (
    <Stack className="reference-page">
      <SettingRow label="名称" description="说明文字" control={<Slider aria-label="一" defaultValue={60} />} />
      <SettingRow label="名称" control={<Slider aria-label="二" defaultValue={60} />} />
      <SettingRow label="名称" control={<Slider aria-label="三" defaultValue={60} />} error="错误文字" />
    </Stack>
  );
}
