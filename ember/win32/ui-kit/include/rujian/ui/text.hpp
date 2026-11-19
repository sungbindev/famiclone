/*
 * ------------------------------------------------------------------------
 * 名称：text.hpp
 * 说明：文字的样式与度量类型。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-19
 * 备注：字号、行高等长度都是逻辑像素；行高按 CSS 的 line-height 倍数计算，文字在行高里上下居中（半行距）。
 * ------------------------------------------------------------------------
 */

#pragma once

#include "rujian/ui/color.hpp"

namespace rujian::ui {

/** 水平对齐。 */
enum class TextAlign { Start, Center, End };

/** 一行放不下时的处理。 */
enum class TextOverflow {
  /** 直接裁掉超出的部分。 */
  Clip,
  /** 用省略号结尾（对应 CSS 的 text-overflow: ellipsis）。 */
  Ellipsis
};

/** 字体：正文字体，或标题栏应用名用的品牌字体（Quicksand，只含拉丁字母，其余字符仍按正文字体）。 */
enum class TextFace { Body, Brand };

/** 文字样式。 */
struct TextStyle {
  /** 字号。 */
  double fontSize{12};
  /** 字重，400 为常规，600 为半粗，700 为粗体。 */
  int fontWeight{400};
  /** 文字颜色。 */
  Color color{kCurrentColor};
  /** 行高倍数；0 表示 CSS 的 normal，用字体自带的行距（上升 + 下降 + 行间距）。 */
  double lineHeight{1.45};
  /** 水平对齐。 */
  TextAlign align{TextAlign::Start};
  /** 超出宽度时的处理；只在不换行时有效。 */
  TextOverflow overflow{TextOverflow::Clip};
  /** 是否在宽度不够时自动换行。 */
  bool wrap{false};
  /** 是否加下划线。 */
  bool underline{false};
  /** 字体。 */
  TextFace face{TextFace::Body};
  /** 字间距，占字号的倍数（CSS 的 letter-spacing，单位 em）；每个字符后面加一份，包括最后一个；只用于不换行、靠左的文字。 */
  double letterSpacing{0};
};

/** 文字度量结果。 */
struct TextMetrics {
  /** 排版后的宽度。 */
  double width{0};
  /** 排版后的高度（行数乘行高）。 */
  double height{0};
};

}  // namespace rujian::ui
