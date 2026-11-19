/*
 * ------------------------------------------------------------------------
 * 名称：tooltip.hpp
 * 说明：悬停提示控件，对应网页版组件库的悬停提示（tooltip-watcher）：鼠标停在带提示的控件上一会儿后，在控件旁弹出纯文字或版式提示，带指向控件的角标。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-19
 * 备注：本控件实现 Root 的 TooltipPresenter，由应用创建一个、与窗口的 Root 一起使用；提示是弹层，不拦截鼠标。
 *       定位规则同网页版：小控件（宽高都不超过 80）对齐控件，大控件对齐鼠标位置；默认在锚点右侧垂直居中，右侧放不下时放左侧，上下不超出窗口；
 *       角标指向锚点中心。延迟（400 毫秒）、已有提示显示时立即弹出下一个、点击与按键后收起等逻辑在 Root 里。
 * ------------------------------------------------------------------------
 */

#pragma once

#include <vector>

#include "rujian/ui/root.hpp"
#include "rujian/ui/widget.hpp"

namespace rujian::ui {

/** 悬停提示。 */
class Tooltip final : public Widget, public TooltipPresenter {
 public:
  /** 提示相对锚点的位置。 */
  enum class Side { Right, Left };

  /** @param root 窗口的 Root，必须比本控件活得久；构造时把自己登记为 root 的提示显示者。 */
  explicit Tooltip(Root& root);
  ~Tooltip() override;

  void showTooltip(const Widget& target, const TooltipContent& content, Point pointer) override;
  void hideTooltip() override;

  [[nodiscard]] bool shown() const { return shown_; }
  [[nodiscard]] Side side() const { return side_; }
  /** 角标尖端的纵向位置（窗口坐标）。 */
  [[nodiscard]] double arrowY() const { return arrowY_; }

  [[nodiscard]] Widget* hitTest(Point /*point*/) override { return nullptr; }
  void paint(Canvas& canvas) override;

 private:
  /** 版式提示各部分的尺寸。 */
  struct Metrics {
    /** 提示整体（含边框）的尺寸。 */
    Size box;
    /** 内容区宽度（版式）或文字宽度（纯文字）。 */
    double innerWidth{0};
    /** 标题行高度（含下边距与分隔线），没有标题行时为 0。 */
    double headHeight{0};
    /** 各条说明的高度。 */
    std::vector<double> itemHeights;
    /** 各条说明的小标题高度。 */
    std::vector<double> itemTitleHeights;
  };

  [[nodiscard]] Metrics compute() const;
  [[nodiscard]] double borderWidth() const;
  void paintArrow(Canvas& canvas) const;

  Root& root_;
  TooltipContent content_;
  bool shown_{false};
  Side side_{Side::Right};
  double arrowY_{0};
};

}  // namespace rujian::ui
