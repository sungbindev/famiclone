/*
 * ------------------------------------------------------------------------
 * 名称：slider-page.tsx
 * 说明：滑块参考页：用 rujian-ui-kit 的真实滑块画出与 gallery 的 slider 页一样的内容。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-19
 * 备注：组件库的滑块不接收 data 属性，样本由截图脚本按“滑块外层元素”的选择器找出；
 *       样本顺序必须与 gallery/pages.cpp 的 buildSliderPage 一致，改一边要同步另一边。
 * ------------------------------------------------------------------------
 */

import { Row, Slider } from '@rujianjs/ui-kit';

/** 滑块页。 */
export function SliderPage() {
  return (
    <Row className="reference-page">
      <Slider aria-label="最左" defaultValue={0} />
      <Slider aria-label="靠左" defaultValue={35} />
      <Slider aria-label="最右" defaultValue={100} />
      <Slider aria-label="禁用" defaultValue={60} disabled />
    </Row>
  );
}
