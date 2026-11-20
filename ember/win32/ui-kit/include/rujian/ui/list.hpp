/*
 * ------------------------------------------------------------------------
 * 名称：list.hpp
 * 说明：分隔线列表：一组不可选择的行纵向排列，相邻两行之间有一条细分割线。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-19
 * 备注：对应组件库的 List（只做 divided 变体）与 ListRow。行是 ListRow（如 SettingRow）；
 *       放在面板内容区里时，面板让列表贴着面板边的第一行去掉上内边距、最后一行去掉下内边距（组件库样式里的同名规则）。
 * ------------------------------------------------------------------------
 */

#pragma once

#include <memory>
#include <vector>

#include "rujian/ui/widget.hpp"

namespace rujian::ui {

/** 列表里的一行：可以去掉上或下内边距。 */
class ListRow : public Widget {
 public:
  /** 去掉上内边距、下内边距（列表贴着面板边的行用）。 */
  void setTrim(bool top, bool bottom);
  [[nodiscard]] bool trimTop() const { return trimTop_; }
  [[nodiscard]] bool trimBottom() const { return trimBottom_; }
  /** 控件列的自然宽度，没有控件列的行为 0。 */
  [[nodiscard]] virtual double controlColumnNaturalWidth() { return 0.0; }
  /** 列表统一的控件列宽度（各行取最宽的），让各行的控件左边对齐；0 表示各行自己算。 */
  virtual void setSharedControlColumnWidth(double width) { (void)width; }

 private:
  bool trimTop_{false};
  bool trimBottom_{false};
};

/** 分隔线列表。 */
class List : public Widget {
 public:
  /** 加一行，返回它的指针。 */
  ListRow* addRow(std::unique_ptr<ListRow> row);
  /** 第一行去掉上内边距、最后一行去掉下内边距。 */
  void setTrimEnds(bool start, bool end);

  [[nodiscard]] Size measure(Size available) override;
  void arrange(const Rect& bounds) override;
  void paint(Canvas& canvas) override;

 private:
  [[nodiscard]] double dividerWidth() const;
  /** 可见的行。 */
  [[nodiscard]] std::vector<ListRow*> rows() const;
  void applyTrim();
  /** 取各行控件列的最宽值，告诉每一行。 */
  void shareControlColumnWidth();

  bool trimStart_{false};
  bool trimEnd_{false};
};

}  // namespace rujian::ui
