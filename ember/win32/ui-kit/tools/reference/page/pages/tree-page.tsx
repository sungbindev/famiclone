/*
 * ------------------------------------------------------------------------
 * 名称：tree-page.tsx
 * 说明：树参考页：用 rujian-ui-kit 的真实 Tree 画出与 gallery 的 tree 页一样的内容。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-19
 * 备注：五个扁平条目、选中第二项；必须与 gallery/pages.cpp 一致，改一边要同步另一边。
 * ------------------------------------------------------------------------
 */

import { Stack, Tree } from '@rujianjs/ui-kit';

/** 条目。 */
const ITEMS = [
  { id: 'general', label: '通用' },
  { id: 'appearance', label: '外观' },
  { id: 'machines', label: '机型与 BIOS' },
  { id: 'input', label: '输入' },
  { id: 'paths', label: '路径' }
];

/** 树页。 */
export function TreePage() {
  return (
    <Stack className="reference-page">
      <Tree aria-label="分组" items={ITEMS} defaultSelectedId="appearance" />
    </Stack>
  );
}
