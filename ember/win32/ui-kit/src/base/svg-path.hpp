/*
 * ------------------------------------------------------------------------
 * 名称：svg-path.hpp
 * 说明：SVG 路径解析：把路径的 d 属性解析成只含绝对坐标的段列表。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-19
 * 备注：支持 M L H V C S Q T A Z 及其小写（相对）写法；H、V 转成直线，S 转成三次曲线，T 转成二次曲线；
 *       弧的两个标志位可以紧贴后面的数字（如 "a1 1 0 011 1"）。
 * ------------------------------------------------------------------------
 */

#pragma once

#include <optional>
#include <string_view>
#include <vector>

#include "rujian/ui/geometry.hpp"

namespace rujian::ui {

/** 路径段，坐标都是绝对坐标。 */
struct PathSegment {
  /** 段的种类。 */
  enum class Kind { Move, Line, Cubic, Quad, Arc, Close };

  Kind kind{Kind::Move};
  /** 三次曲线的第一控制点；二次曲线的控制点。 */
  Point control1{};
  /** 三次曲线的第二控制点。 */
  Point control2{};
  /** 终点（Close 不用）。 */
  Point end{};
  /** 弧的 x 半径。 */
  double radiusX{0};
  /** 弧的 y 半径。 */
  double radiusY{0};
  /** 弧的 x 轴旋转角度（度）。 */
  double rotation{0};
  /** 弧的大弧标志。 */
  bool largeArc{false};
  /** 弧的顺时针标志。 */
  bool sweep{false};
};

/**
 * 解析 SVG 路径。
 * @param data 路径的 d 属性。
 * @return 段列表；路径有语法错误（未知命令、参数不足、数字格式错误、不以 M 开头）时返回空值。
 */
[[nodiscard]] std::optional<std::vector<PathSegment>> parseSvgPath(std::string_view data);

}  // namespace rujian::ui
