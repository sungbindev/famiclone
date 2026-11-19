/*
 * ------------------------------------------------------------------------
 * 名称：setting-row.hpp
 * 说明：设置行：左边是名称与说明，右边是控件（按钮、下拉框、输入框等），控件下方可以带错误文字。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-19
 * 备注：对应组件库的 SettingRow（放在分隔线列表里的行）；只实现“控件”这种用法，只读的值行（value）Ember 没用到。
 *       窗口宽度不超过 720 逻辑像素时左右两部分上下排列（与网页版的媒体查询一致）。
 * ------------------------------------------------------------------------
 */

#pragma once

#include <memory>
#include <string>

#include "rujian/ui/list.hpp"

namespace rujian::ui {

class Label;

/** 设置行。 */
class SettingRow : public ListRow {
 public:
  SettingRow();

  /** 名称。 */
  void setLabel(std::string label);
  /** 说明，换行符按换行处理，空表示没有说明。 */
  void setDescription(std::string description);
  /** 右边的控件。 */
  void setControl(std::unique_ptr<Widget> control);
  /** 控件下方的错误文字，空表示没有错误。 */
  void setError(std::string error);

  [[nodiscard]] Size measure(Size available) override;
  void arrange(const Rect& bounds) override;

 private:
  /** 按可用宽度算出的布局。 */
  struct Layout {
    bool stacked{false};
    double textWidth{0};
    double controlWidth{0};
    double textHeight{0};
    double labelHeight{0};
    double descriptionHeight{0};
    double controlHeight{0};
    double errorHeight{0};
    double controlColumnHeight{0};
    double height{0};
  };

  [[nodiscard]] Layout layout(double width);
  /** 行的上、下内边距（列表贴着面板边时去掉）。 */
  [[nodiscard]] double paddingTop() const;
  [[nodiscard]] double paddingBottom() const;

  Label* label_{nullptr};
  Label* description_{nullptr};
  Label* error_{nullptr};
  Widget* control_{nullptr};
};

}  // namespace rujian::ui
