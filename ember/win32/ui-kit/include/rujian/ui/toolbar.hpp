/*
 * ------------------------------------------------------------------------
 * 名称：toolbar.hpp
 * 说明：工具栏：左边是前导控件（如搜索框）与一行说明，右边是操作，整条下方有一条分割线；宽度不够时操作换到下一行。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-19
 * 备注：对应组件库的 Toolbar，只做 Ember 用到的部分（前导、说明、操作；标题没用到，不做）。
 * ------------------------------------------------------------------------
 */

#pragma once

#include <memory>
#include <string>

#include "rujian/ui/widget.hpp"

namespace rujian::ui {

class Label;

/** 工具栏。 */
class Toolbar : public Widget {
 public:
  Toolbar();

  /** 左边的前导控件。 */
  void setLeading(std::unique_ptr<Widget> leading);
  /** 前导控件后面的一行说明（如数量），空表示没有。 */
  void setDescription(std::string description);
  /** 右边的操作（一般是 Actions）。 */
  void setActions(std::unique_ptr<Widget> actions);

  [[nodiscard]] Size measure(Size available) override;
  void arrange(const Rect& bounds) override;
  void paint(Canvas& canvas) override;

 private:
  /** 按可用宽度算出的布局。 */
  struct Layout {
    bool wrapped{false};
    double leadingWidth{0};
    double leadingHeight{0};
    double descriptionWidth{0};
    double descriptionHeight{0};
    double actionsWidth{0};
    double actionsHeight{0};
    double startHeight{0};
    /** 一行放下全部内容需要的宽度。 */
    double naturalWidth{0};
    double height{0};
  };

  [[nodiscard]] Layout layout(double width);
  [[nodiscard]] double borderWidth() const;

  Widget* leading_{nullptr};
  Label* description_{nullptr};
  Widget* actions_{nullptr};
};

}  // namespace rujian::ui
