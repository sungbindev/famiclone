/*
 * ------------------------------------------------------------------------
 * 名称：field-page.tsx
 * 说明：字段参考页：用 rujian-ui-kit 的真实 Field 画出与 gallery 的 field 页一样的内容。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-19
 * 备注：两个字段：有标签与说明、没有标签只有说明；控件是没有文字的滑块；文字必须与 gallery/pages.cpp 一致。
 * ------------------------------------------------------------------------
 */

import { Field, Slider, Stack } from '@rujianjs/ui-kit';

/** 字段页。 */
export function FieldPage() {
  return (
    <Stack className="reference-page">
      <Field label="名称" description="说明文字">
        <Slider defaultValue={35} />
      </Field>
      <Field description="说明文字">
        <Slider aria-label="无标签" defaultValue={35} />
      </Field>
    </Stack>
  );
}
