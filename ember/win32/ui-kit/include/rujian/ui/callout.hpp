/*
 * ------------------------------------------------------------------------
 * 名称：callout.hpp
 * 说明：提示块：浅色底加同色细边框的一段说明，说明文字下方可以再放别的控件（如链接按钮）。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-19
 * 备注：对应组件库的 Callout，只做 Ember 用到的部分（说明文字加后续内容，普通与错误两种）；加粗标题与标题图标暂未支持。
 * ------------------------------------------------------------------------
 */

#pragma once

#include <memory>
#include <string>

#include "rujian/ui/widget.hpp"

namespace rujian::ui {

class Label;

/** 提示块。 */
class Callout : public Widget {
 public:
  Callout();

  /** 说明文字。 */
  void setText(std::string text);
  /** 是否是错误样式（默认是提示样式，颜色不同）。 */
  void setError(bool error);
  /** 往说明文字后面加一项内容，返回它的指针。 */
  Widget* addContent(std::unique_ptr<Widget> content);

  [[nodiscard]] Size measure(Size available) override;
  void arrange(const Rect& bounds) override;
  void paint(Canvas& canvas) override;

 private:
  [[nodiscard]] double borderWidth() const;

  Label* text_{nullptr};
  bool error_{false};
};

}  // namespace rujian::ui
