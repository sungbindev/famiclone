/*
 * ------------------------------------------------------------------------
 * 名称：label.hpp
 * 说明：文字控件：一段可换行的文字，字号、字重、行高与颜色可设；Heading 是组件库的 Heading（小节标题）。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-19
 * 备注：网页版里这些文字是 p、h3 等普通元素，原生控件库用它统一承载；颜色用“按主题取色”的函数给出，绘制时现取。
 *       说明文字里成对的 ** 加粗强调暂不支持（Ember 的界面还没有用到）。
 * ------------------------------------------------------------------------
 */

#pragma once

#include <functional>
#include <string>

#include "rujian/ui/widget.hpp"

namespace rujian::ui {

/** 文字控件。 */
class Label : public Widget {
 public:
  /** 按主题给出文字颜色。 */
  using ColorProvider = std::function<Color(ThemeName)>;

  explicit Label(std::string text = {});

  /** 设置文字（UTF-8），换行符按换行处理。 */
  void setText(std::string text);
  [[nodiscard]] const std::string& text() const { return text_; }
  /** 字号，默认正文字号 12。 */
  void setFontSize(double fontSize);
  /** 字重，默认 400。 */
  void setFontWeight(int fontWeight);
  /** 行高倍数，0 表示 normal，默认 0。 */
  void setLineHeight(double lineHeight);
  /** 放不下时是否换行，默认是；不换行时超出的部分用省略号。 */
  void setWrap(bool wrap);
  /** 水平对齐，默认靠左；控件比文字宽时才看得出来。 */
  void setAlign(TextAlign align);
  /** 文字颜色，默认是正文文字色。 */
  void setColorProvider(ColorProvider provider);

  [[nodiscard]] Size measure(Size available) override;
  void paint(Canvas& canvas) override;

 private:
  [[nodiscard]] TextStyle style(Color color) const;

  std::string text_;
  double fontSize_;
  int fontWeight_{400};
  double lineHeight_{0};
  bool wrap_{true};
  TextAlign align_{TextAlign::Start};
  ColorProvider color_;
};

/** 小节标题（组件库的 Heading）：字号 12、字重 600。 */
class Heading : public Label {
 public:
  explicit Heading(std::string text = {});
};

}  // namespace rujian::ui
