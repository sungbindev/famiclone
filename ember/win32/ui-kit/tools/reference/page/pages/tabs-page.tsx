/*
 * ------------------------------------------------------------------------
 * 名称：tabs-page.tsx
 * 说明：页签参考页：用 rujian-ui-kit 的真实 Tabs 画出与 gallery 的 tabs 页一样的内容。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-19
 * 备注：一条放在一行里的页签栏（宽度是内容宽），以及纵向堆叠里带内容的页签；
 *       文字与内容块（高 24、强调色）必须与 gallery/pages.cpp 一致，改一边要同步另一边。
 * ------------------------------------------------------------------------
 */

import { Row, Stack, Tabs } from '@rujianjs/ui-kit';

/** 页签页。 */
export function TabsPage() {
  return (
    <Stack className="reference-page">
      <Row>
        <Tabs
          aria-label="仅页签栏"
          items={[
            { id: 'a', label: '第一页' },
            { id: 'b', label: '第二页' },
            { id: 'c', label: '第三页' }
          ]}
        />
      </Row>
      <Tabs
        aria-label="带内容"
        items={[
          { id: 'a', label: '第一页', content: <div data-sample style={{ height: 24, background: 'var(--accent)' }} /> },
          { id: 'b', label: '第二页' }
        ]}
      />
    </Stack>
  );
}
