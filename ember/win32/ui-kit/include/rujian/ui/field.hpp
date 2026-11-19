/*
 * ------------------------------------------------------------------------
 * 名称：field.hpp
 * 说明：字段：给一个控件配上标签与说明文字，纵向排列。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-19
 * 备注：对应组件库的 Field，只做 Ember 用到的部分：有标签时依次是标签、说明、控件；没有标签时是控件、说明。
 *       必填标记、校验错误与说明里的加粗强调暂未支持。
 * ------------------------------------------------------------------------
 */

#pragma once

#include <memory>
#include <string>

#include "rujian/ui/widget.hpp"

namespace rujian::ui {

class Label;

/** 字段。 */
class Field : public Widget {
 public:
  Field();

  /** 标签文字，空表示没有标签。 */
  void setLabel(std::string label);
  /** 说明文字，换行符按换行处理，空表示没有说明。 */
  void setDescription(std::string description);
  /** 设置被包装的控件。 */
  void setControl(std::unique_ptr<Widget> control);

  [[nodiscard]] Size measure(Size available) override;
  void arrange(const Rect& bounds) override;

 private:
  Label* label_{nullptr};
  Label* description_{nullptr};
  Widget* control_{nullptr};
};

}  // namespace rujian::ui
