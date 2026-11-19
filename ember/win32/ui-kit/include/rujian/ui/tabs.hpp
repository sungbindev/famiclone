/*
 * ------------------------------------------------------------------------
 * 名称：tabs.hpp
 * 说明：页签：一条页签栏加各页签的内容，同一时间只显示选中页签的内容。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-19
 * 备注：对应组件库的 Tabs 基础下划线样式（steps 变体、页签后的数量 Ember 没用到，不做）。
 *       页签栏在网页版里与内容是并列的元素，所以放在纵向堆叠里时，页签栏与内容之间是堆叠的间距（10）；这里同样留 10。
 *       键盘：整条页签栏只有一个 Tab 停靠点（选中的页签），左右方向键切换（首尾循环），Home 与 End 跳到首尾，切换时同时选中。
 * ------------------------------------------------------------------------
 */

#pragma once

#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "rujian/ui/widget.hpp"

namespace rujian::ui {

class TabBar;

/** 页签。 */
class Tabs : public Widget {
 public:
  /** 选中页签变化时调用，参数是新选中的页签标识。 */
  using SelectHandler = std::function<void(const std::string&)>;

  Tabs();

  /**
   * 加一个页签。
   * @param id 页签标识。
   * @param label 页签文字。
   * @param content 页签的内容，可以没有；返回它的指针（没有内容时为空）。
   */
  Widget* addTab(std::string id, std::string label, std::unique_ptr<Widget> content = nullptr);
  /** 选中指定页签（不调用回调）；标识不存在时不变。 */
  void select(const std::string& id);
  /** 当前选中页签的标识，没有页签时为空串。 */
  [[nodiscard]] std::string selectedId() const;
  void setOnSelect(SelectHandler handler);
  /** 页签栏（给测试与对照用，它是可获得键盘焦点的控件）。 */
  [[nodiscard]] Widget* tabBar() const;

  [[nodiscard]] Size measure(Size available) override;
  void arrange(const Rect& bounds) override;

 private:
  friend class TabBar;

  /** 一个页签。 */
  struct Tab {
    std::string id;
    std::string label;
    Widget* content;
  };

  /** 用户操作选中第 index 个页签：变化时更新内容并调用回调。 */
  void choose(std::size_t index);
  void showSelectedContent();

  TabBar* bar_{nullptr};
  std::vector<Tab> tabs_;
  std::size_t selected_{0};
  SelectHandler onSelect_;
};

}  // namespace rujian::ui
