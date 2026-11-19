/*
 * ------------------------------------------------------------------------
 * 名称：list-page.tsx
 * 说明：分隔线列表参考页：用 rujian-ui-kit 的真实 Panel、List、SettingRow 画出与 gallery 的 list 页一样的内容。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-19
 * 备注：面板里一个分隔线列表，三个设置行（控件是没有文字的滑块）；必须与 gallery/pages.cpp 一致，改一边要同步另一边。
 * ------------------------------------------------------------------------
 */

import { List, Panel, SettingRow, Slider, Stack } from '@rujianjs/ui-kit';

/** 列表页。 */
export function ListPage() {
  return (
    <Stack className="reference-page">
      <Panel>
        <List variant="divided">
          {[20, 50, 80].map((value) => (
            <SettingRow key={value} label="名称" control={<Slider aria-label={`滑块${value}`} defaultValue={value} />} />
          ))}
        </List>
      </Panel>
    </Stack>
  );
}
