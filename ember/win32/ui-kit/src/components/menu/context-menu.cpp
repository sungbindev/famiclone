/*
 * ------------------------------------------------------------------------
 * 名称：context-menu.cpp
 * 说明：右键菜单的实现：打开时占用键盘焦点并显示弹层，选中后关闭并回调，关闭后把焦点还给打开前的控件。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-25
 * 备注：无。
 * ------------------------------------------------------------------------
 */

#include "rujian/ui/context-menu.hpp"

namespace rujian::ui {

namespace {

/** 弹层的最小宽度（含边框）。 */
constexpr double kMinimumWidth = 160.0;

/** 按路径找菜单项，路径无效时为空。 */
const MenuItem* resolveItem(const std::vector<MenuItem>& items, const std::vector<std::size_t>& path) {
  const std::vector<MenuItem>* level = &items;
  const MenuItem* item = nullptr;
  for (const std::size_t index : path) {
    if (index >= level->size()) return nullptr;
    item = &(*level)[index];
    level = &item->items;
  }
  return item;
}

}  // namespace

ContextMenu::ContextMenu() = default;

ContextMenu::~ContextMenu() {
  if (popup_ != nullptr && host() != nullptr) {
    popup_->closeSub();
    host()->hideLayer(popup_.get());
  }
}

void ContextMenu::open(Point position, std::vector<MenuItem> items, Handler onChoose, bool highlightFirst) {
  if (host() == nullptr || items.empty()) return;
  close();
  Widget* focused = host()->focusedWidget();
  previous_ = focused == this ? nullptr : focused;
  items_ = std::move(items);
  handler_ = std::move(onChoose);
  host()->setFocus(this, true);
  popup_ = std::make_unique<MenuPopup>(*this, items_);
  popup_->setMinWidth(kMinimumWidth);
  host()->showLayer(popup_.get(), [this](Widget* hit) { menuOutsidePress(hit); });
  popup_->placeBelow(Rect{position.x, position.y, 0, 0});
  popup_->setActive(highlightFirst ? 0 : -1);
}

void ContextMenu::close() {
  if (popup_ == nullptr) return;
  popup_->closeSub();
  if (host() != nullptr) host()->hideLayer(popup_.get());
  popup_.reset();
  Widget* target = previous_;
  previous_ = nullptr;
  // 焦点还在本控件上时才还给原来的控件；已经被别处抢走（如点了别的控件）就不动
  if (host() != nullptr && target != nullptr && host()->focusedWidget() == this) host()->setFocus(target, true);
}

bool ContextMenu::onKeyDown(const KeyEvent& event) { return popup_ != nullptr && popup_->handleKey(event); }

bool ContextMenu::onChar(char32_t codePoint) { return popup_ != nullptr && popup_->handleChar(codePoint); }

void ContextMenu::onStateChanged() {
  Widget::onStateChanged();
  // 焦点被别处抢走时关闭
  if (!focused() && popup_ != nullptr) close();
}

void ContextMenu::menuActivated(const std::vector<std::size_t>& path) {
  const MenuItem* item = resolveItem(items_, path);
  if (item == nullptr || item->disabled) return;
  const std::string id = item->id;
  const Handler handler = handler_;
  close();
  if (handler) handler(id);
}

void ContextMenu::menuClosed(bool /*returnFocus*/) { close(); }

void ContextMenu::menuSwitch(int /*delta*/) {}

void ContextMenu::menuOutsidePress(Widget* hit) {
  if (popup_ == nullptr) return;
  if (hit != nullptr && popup_->containsTree(hit)) return;
  close();
}

}  // namespace rujian::ui
