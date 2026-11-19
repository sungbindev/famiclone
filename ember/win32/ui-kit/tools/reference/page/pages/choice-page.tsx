/*
 * ------------------------------------------------------------------------
 * 名称：choice-page.tsx
 * 说明：选择控件参考页：用 rujian-ui-kit 的真实复选框与分段单选组画出与 gallery 的 choice 页一样的内容。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-19
 * 备注：组件库的复选框与单选组不接收 data 属性，样本由截图脚本按“复选框、单选组”的选择器找出；
 *       样本顺序必须与 gallery/pages.cpp 的 buildChoicePage 一致，改一边要同步另一边。
 * ------------------------------------------------------------------------
 */

import { Checkbox, RadioGroup, Stack } from '@rujianjs/ui-kit';

/** 分段单选组的选项，中间一项的文字与是否禁用由参数决定。 */
function options(second: string, secondDisabled: boolean) {
  return [
    { value: 'all', label: '全部' },
    { value: 'second', label: second, disabled: secondDisabled },
    { value: 'third', label: '收藏' }
  ];
}

/** 选择控件页。 */
export function ChoicePage() {
  return (
    <Stack className="reference-page">
      <Checkbox label="启用自动保存" />
      <Checkbox label="已勾选的选项" defaultChecked />
      <Checkbox label="禁用的选项" disabled />
      <Checkbox label="禁用且已勾选" defaultChecked disabled />
      <RadioGroup variant="segmented" options={options('最近', false)} defaultValue="second" />
      <RadioGroup variant="segmented" options={options('最近', false)} defaultValue="all" />
      <RadioGroup variant="segmented" options={options('已停用', true)} defaultValue="all" />
      <RadioGroup variant="segmented" options={options('最近', false)} defaultValue="second" disabled />
    </Stack>
  );
}
