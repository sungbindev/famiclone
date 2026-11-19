/*
 * ------------------------------------------------------------------------
 * 名称：token-types.hpp
 * 说明：生成的令牌文件用到的类型：阴影与阴影列表。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-19
 * 备注：阴影的偏移与模糊半径都是逻辑像素；一个令牌最多三条阴影。
 * ------------------------------------------------------------------------
 */

#pragma once

#include <array>

#include "rujian/ui/color.hpp"

namespace rujian::ui {

/** 一条阴影（对应 CSS 的 box-shadow 一项）。 */
struct Shadow {
  /** 水平偏移。 */
  double offsetX;
  /** 垂直偏移。 */
  double offsetY;
  /** 模糊半径。 */
  double blur;
  /** 扩展半径。 */
  double spread;
  /** 颜色。 */
  Color color;
  /** 是否是内阴影。 */
  bool inset;
};

/** 阴影列表；count 为 0 表示没有阴影。 */
struct ShadowList {
  /** 有效的阴影条数。 */
  int count;
  /** 阴影，前 count 条有效。 */
  std::array<Shadow, 3> items;
};

}  // namespace rujian::ui
