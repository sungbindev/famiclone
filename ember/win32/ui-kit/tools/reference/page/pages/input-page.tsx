/*
 * ------------------------------------------------------------------------
 * 名称：input-page.tsx
 * 说明：输入框参考页：用 rujian-ui-kit 的真实输入框画出与 gallery 的 input 页一样的内容。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-19
 * 备注：data-sample 的顺序必须与 gallery/pages.cpp 的 buildInputPage 里 addInput 的顺序一致，改一边要同步另一边。
 * ------------------------------------------------------------------------
 */

import { Stack, TextInput } from '@rujianjs/ui-kit';

/** 输入框页。 */
export function InputPage() {
  return (
    <Stack className="reference-page">
      <TextInput data-sample placeholder="请输入" />
      <TextInput data-sample defaultValue="已填写的文字 Hello" />
      <TextInput data-sample defaultValue="不合法的值" invalid />
      <TextInput data-sample defaultValue="禁用的输入框" disabled />
      <TextInput data-sample placeholder="禁用的占位文字" disabled />
      <TextInput data-sample defaultValue="这是一段很长很长很长很长很长很长很长很长很长很长很长的文字，超出输入框宽度会被裁掉" />
      <TextInput data-sample type="search" defaultValue="搜索词" />
      <TextInput data-sample type="search" placeholder="搜索" />
    </Stack>
  );
}
