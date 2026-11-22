/*
 * ------------------------------------------------------------------------
 * 名称：icon-data.hpp
 * 说明：图标路径数据的内部类型：一个图标由若干条 SVG 路径组成，路径可带填充、描边与透明度。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-19
 * 备注：数据由 tools/gen-icons.mjs 生成在 src/generated；路径的坐标在 24×24 的画布里，显示时取中间 20×20（viewBox 2 2 20 20）。
 * ------------------------------------------------------------------------
 */

#pragma once

#include <cstddef>
#include <string_view>

#include "rujian/ui/icon-id.hpp"

namespace rujian::ui {

/** 图标里的一条路径。 */
struct IconPath {
  /** SVG 路径的 d 属性。 */
  std::string_view d;
  /** 填充：空表示不填充，currentColor 表示用图标颜色填充，accent 表示用调用方给的强调色填充（没有给强调色时不填充）。 */
  std::string_view fill;
  /** 描边：空表示按默认描边图标颜色，none 表示不描边。 */
  std::string_view stroke;
  /** 不透明度，0 到 1。 */
  double opacity;
};

/** 一个图标的全部路径。 */
struct IconData {
  const IconPath* paths;
  std::size_t count;
};

/**
 * 取图标的路径数据。
 * @param id 图标编号，必须小于 IconId::Count。
 */
const IconData& iconData(IconId id);

}  // namespace rujian::ui
