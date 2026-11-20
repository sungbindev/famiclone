/*
 * ------------------------------------------------------------------------
 * 名称：setting-row.hpp
 * 说明：设置行：左边是名称与说明，右边是控件（按钮、下拉框、输入框等），控件下方可以带错误文字。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-19
 * 备注：对应组件库的 SettingRow（放在分隔线列表里的行）；只实现“控件”这种用法，只读的值行（value）Ember 没用到。
 *       默认窗口宽度不超过 720 逻辑像素时左右两部分上下排列（与网页版的媒体查询一致）；
 *       并排模式（setSideBySide）不跟网页版：左右两部分永远并排，控件列靠右、列宽取同一列表里各行控件的最大自然宽度，控件在列里左对齐、不换行；名称与说明占满余下的宽度，说明可以折行（最小宽度按 160 算），控件不换行。
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
  /** 并排模式：左右两部分永远并排（不随窗口变窄上下排列），控件列靠右、控件在列里左对齐、不换行；默认关。 */
  void setSideBySide(bool sideBySide);

  [[nodiscard]] Size measure(Size available) override;
  /** 并排模式下一行不折行所需的最小宽度（名称与说明列至少 160，名称更宽取名称）；默认模式没有要求。 */
  [[nodiscard]] Size minimumSize() override;
  void arrange(const Rect& bounds) override;
  [[nodiscard]] double controlColumnNaturalWidth() override;
  void setSharedControlColumnWidth(double width) override;

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
  /** 并排模式的最小宽度。 */
  [[nodiscard]] double sideBySideMinimumWidth();
  /** 行的上、下内边距（列表贴着面板边时去掉）。 */
  [[nodiscard]] double paddingTop() const;
  [[nodiscard]] double paddingBottom() const;

  Label* label_{nullptr};
  Label* description_{nullptr};
  Label* error_{nullptr};
  Widget* control_{nullptr};
  bool sideBySide_{false};
  double sharedControlWidth_{0};
};

}  // namespace rujian::ui
