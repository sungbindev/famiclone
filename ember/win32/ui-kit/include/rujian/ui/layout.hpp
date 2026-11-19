/*
 * ------------------------------------------------------------------------
 * 名称：layout.hpp
 * 说明：弹性布局：Stack（纵向堆叠）、Row（居中对齐、可换行的一行）、Wrap（可换行的一组）、Actions（操作行），对应网页版的同名布局原语。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-19
 * 备注：只实现界面用到的 flex 能力：方向、换行、间距、交叉轴对齐（居中、顶部、拉伸）、按比例分配主轴剩余空间的 grow；
 *       不可见的子控件不占位；间距取令牌（Stack 为区块间距，Row 为中间距，Wrap 为小间距，Actions 为操作间距）。
 * ------------------------------------------------------------------------
 */

#pragma once

#include <map>

#include "rujian/ui/widget.hpp"

namespace rujian::ui {

/** 主轴方向。 */
enum class FlexDirection { Row, Column };

/** 交叉轴对齐。 */
enum class FlexAlign { Start, Center, Stretch };

/** 弹性布局容器。 */
class FlexBox : public Widget {
 public:
  /**
   * @param direction 主轴方向。
   * @param wrap 主轴放不下时是否换行（只对横向有效）。
   * @param gap 子控件之间的间距（主轴与换行后的行间距相同）。
   * @param align 交叉轴对齐。
   */
  FlexBox(FlexDirection direction, bool wrap, double gap, FlexAlign align);

  /** 设置子控件占主轴剩余空间的比例（CSS 的 flex-grow），默认 0。 */
  void setGrow(const Widget* child, double grow);
  /** 设置容器的内边距。 */
  void setPadding(const Insets& padding);

  [[nodiscard]] Size measure(Size available) override;
  void arrange(const Rect& bounds) override;

 private:
  /** 一行（或纵向时整列）子控件及其尺寸。 */
  struct Line {
    std::vector<Widget*> items;
    std::vector<Size> sizes;
    double main{0};
    double cross{0};
  };

  [[nodiscard]] std::vector<Line> breakLines(double mainLimit);
  [[nodiscard]] double growOf(const Widget* child) const;

  FlexDirection direction_;
  bool wrap_;
  double gap_;
  FlexAlign align_;
  Insets padding_{};
  std::map<const Widget*, double> grow_;
};

/** 纵向堆叠，子控件拉伸到容器宽度，间距为区块间距。 */
class Stack final : public FlexBox {
 public:
  Stack();
};

/** 居中对齐、可换行的一行，间距为中间距。 */
class Row final : public FlexBox {
 public:
  Row();
};

/** 可换行的一组，间距为小间距，交叉轴拉伸（有固定高度的控件除外）。 */
class Wrap final : public FlexBox {
 public:
  Wrap();
};

/** 操作行：一组按钮，居中对齐、可换行，间距为操作间距。 */
class Actions final : public FlexBox {
 public:
  Actions();
};

}  // namespace rujian::ui
