/*
 * ------------------------------------------------------------------------
 * 名称：state.hpp
 * 说明：说明块：列表等内容区在没有内容或出错时显示的一段说明；出错时文字前带一个提示图标、文字用错误色。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-19
 * 备注：对应组件库的 State，只做普通与出错两种；“加载中”不做（Ember 里不显示任何内容），文字下方的按钮也不做。
 * ------------------------------------------------------------------------
 */

#pragma once

#include <string>

#include "rujian/ui/widget.hpp"

namespace rujian::ui {

class Label;

/** 说明块。 */
class State : public Widget {
 public:
  State();

  /** 说明文字。 */
  void setText(std::string text);
  /** 是否是出错样式。 */
  void setError(bool error);

  [[nodiscard]] Size measure(Size available) override;
  void arrange(const Rect& bounds) override;
  void paint(Canvas& canvas) override;

 private:
  /** 文字起点相对左边缘的偏移：出错时让出图标槽与间距。 */
  [[nodiscard]] double textOffset() const;

  Label* text_{nullptr};
  bool error_{false};
};

}  // namespace rujian::ui
