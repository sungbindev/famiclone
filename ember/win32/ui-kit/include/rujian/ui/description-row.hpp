/*
 * ------------------------------------------------------------------------
 * 名称：description-row.hpp
 * 说明：说明行：左边是一段说明文字（字段说明的字号与颜色），右边是一个操作控件（如“清除”按钮），两者垂直居中；宽度不够时说明文字换行。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-20
 * 备注：手柄图下方的说明行，它不在网页版组件库里，没有网页基线，由单元测试覆盖；两部分之间留 8（space-md）。
 * ------------------------------------------------------------------------
 */

#pragma once

#include <memory>
#include <string>

#include "rujian/ui/widget.hpp"

namespace rujian::ui {

class Label;

/** 说明行。 */
class DescriptionRow : public Widget {
 public:
  DescriptionRow();

  /** 说明文字。 */
  void setText(std::string text);
  [[nodiscard]] const std::string& text() const;
  /** 说明文字的水平对齐，默认靠左。 */
  void setTextAlign(TextAlign align);
  /** 右边的操作控件。 */
  void setAction(std::unique_ptr<Widget> action);

  [[nodiscard]] Size measure(Size available) override;
  void arrange(const Rect& bounds) override;

 private:
  Label* text_{nullptr};
  Widget* action_{nullptr};
};

}  // namespace rujian::ui
