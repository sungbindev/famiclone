/*
 * ------------------------------------------------------------------------
 * 名称：context-menu.hpp
 * 说明：右键菜单：在指定位置弹出菜单弹层，键盘与鼠标操作同菜单栏的菜单；选中叶子项后关闭并把项的标识交给回调。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-25
 * 备注：Ember 自己的控件，网页版没有。本控件是个看不见的、不在 Tab 顺序里的控件，放进控件树后才有宿主；
 *       菜单打开时键盘焦点移到它上面（按键与助记字母由它转给弹层），关闭后焦点回到打开前的控件；
 *       焦点被别处抢走、窗口大小变化或在弹层之外按下鼠标时关闭。菜单项与菜单栏的菜单共用 MenuItem（禁用项置灰，不能选中）。
 * ------------------------------------------------------------------------
 */

#pragma once

#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "rujian/ui/menu.hpp"
#include "rujian/ui/widget.hpp"

namespace rujian::ui {

/** 右键菜单。 */
class ContextMenu final : public Widget, public MenuPopupOwner {
 public:
  /** 选中一个叶子项时调用，参数是项的标识。 */
  using Handler = std::function<void(const std::string& itemId)>;

  ContextMenu();
  ~ContextMenu() override;

  /**
   * 在窗口坐标 position 弹出菜单（独立小窗口），弹层可用范围内放不下时翻到上方或左移。
   * @param highlightFirst 是否高亮第一项（用键盘打开时为真，鼠标右键打开时为假）。
   */
  void open(Point position, std::vector<MenuItem> items, Handler onChoose, bool highlightFirst = false);
  /** 关闭菜单；焦点还在本控件上时回到打开前的控件。 */
  void close();
  [[nodiscard]] bool isOpen() const { return popup_ != nullptr; }
  /** 打开着的弹层（测试用）。 */
  [[nodiscard]] MenuPopup* popup() const { return popup_.get(); }

  [[nodiscard]] Size measure(Size /*available*/) override { return Size{}; }
  [[nodiscard]] Widget* hitTest(Point /*point*/) override { return nullptr; }
  [[nodiscard]] bool focusable() const override { return true; }
  [[nodiscard]] bool tabStop() const override { return false; }
  bool onKeyDown(const KeyEvent& event) override;
  bool onChar(char32_t codePoint) override;
  void onStateChanged() override;

  void menuActivated(const std::vector<std::size_t>& path) override;
  void menuClosed(bool returnFocus) override;
  void menuSwitch(int delta) override;
  void menuOutsidePress(Widget* hit) override;

 private:
  std::vector<MenuItem> items_;
  std::unique_ptr<MenuPopup> popup_;
  Handler handler_;
  /** 打开前有键盘焦点的控件；关闭时把焦点还给它。 */
  Widget* previous_{nullptr};
};

}  // namespace rujian::ui
