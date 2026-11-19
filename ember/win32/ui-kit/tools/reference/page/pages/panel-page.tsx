/*
 * ------------------------------------------------------------------------
 * 名称：panel-page.tsx
 * 说明：面板参考页：用 rujian-ui-kit 的真实面板画出与 gallery 的 panel 页一样的内容。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-19
 * 备注：五个面板：标题加 aside 加两块内容、紧凑标题栏、平面、只有 aside、只有标题（内容区为空）；
 *       内容块高 24、底色取强调色，必须与 gallery/pages.cpp 的 Block 一致，改一边要同步另一边。
 * ------------------------------------------------------------------------
 */

import { Panel, Slider, Stack } from '@rujianjs/ui-kit';

/** 内容块的高度（像素）。 */
const BLOCK_HEIGHT = 24;

/** 没有文字的内容块。 */
function Block() {
  return <div style={{ height: BLOCK_HEIGHT, background: 'var(--accent)' }} />;
}

/** 面板页。 */
export function PanelPage() {
  return (
    <Stack className="reference-page">
      <Panel title="面板" aside={<Slider aria-label="进度" defaultValue={35} />}>
        <Block />
        <Block />
      </Panel>
      <Panel title="紧凑标题" compactHead>
        <Block />
      </Panel>
      <Panel title="平面" plain>
        <Block />
      </Panel>
      <Panel aside={<Slider aria-label="进度" defaultValue={60} />}>
        <Block />
      </Panel>
      <Panel title="仅标题" />
    </Stack>
  );
}
