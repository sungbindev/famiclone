/*
 * ------------------------------------------------------------------------
 * 名称：facts-page.tsx
 * 说明：键值清单参考页：用 rujian-ui-kit 的真实 Facts 画出与 gallery 的 facts 页一样的内容；宽、窄两个页面共用，宽窄由页面（窗口）宽度决定。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-19
 * 备注：三条，最后一条的内容分两行（每行一个 div）；文字必须与 gallery/pages.cpp 一致，改一边要同步另一边。
 * ------------------------------------------------------------------------
 */

import { Facts, Stack } from '@rujianjs/ui-kit';

/** 键值清单页。 */
export function FactsPage() {
  return (
    <Stack className="reference-page">
      <Facts
        items={[
          { label: '构建时间', value: '2026-11-19 10:00' },
          { label: '作者', value: '某某' },
          { label: '授权', value: ['第一行', '第二行'].map((line) => <div key={line}>{line}</div>) }
        ]}
      />
    </Stack>
  );
}
