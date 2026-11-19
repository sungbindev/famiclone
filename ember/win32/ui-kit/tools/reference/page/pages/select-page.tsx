/*
 * ------------------------------------------------------------------------
 * 名称：select-page.tsx
 * 说明：下拉框参考页：用 rujian-ui-kit 的真实下拉框画出与 gallery 的 select 页一样的内容。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-19
 * 备注：组件库的下拉框不接收 data 属性，样本由截图脚本按“下拉框外层元素”的选择器找出；
 *       展开后弹层里的各个选项也会按顺序排在样本之后（与 gallery 输出的选项位置一致）；
 *       样本顺序必须与 gallery/pages.cpp 的 buildSelectPage 一致，改一边要同步另一边。
 * ------------------------------------------------------------------------
 */

import { Row, Select } from '@rujianjs/ui-kit';

const OPTIONS = ['选项一', '选项二', '选项三', '选项四'];

/** 下拉框页。 */
export function SelectPage() {
  return (
    <Row className="reference-page">
      <Select options={OPTIONS.map((label, index) => ({ value: 'abcd'[index] as string, label }))} defaultValue="b" allowEmpty={false} />
      <Select options={OPTIONS.map((label, index) => ({ value: 'abcd'[index] as string, label }))} allowEmpty={false} />
      <Select options={OPTIONS.map((label, index) => ({ value: 'abcd'[index] as string, label }))} defaultValue="c" allowEmpty={false} disabled />
    </Row>
  );
}
