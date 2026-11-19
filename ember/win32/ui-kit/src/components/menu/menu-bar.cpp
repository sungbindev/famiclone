/*
 * ------------------------------------------------------------------------
 * 名称：menu-bar.cpp
 * 说明：菜单栏的实现：菜单名按钮的排版与绘制、打开与切换菜单、键盘导航，并把弹层的选中结果转成回调。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-19
 * 备注：与组件库 MenuBar 的行为一致：点击打开时高亮第一项，悬停切换时不高亮，方向键打开时高亮第一项（上方向键为最后一项）；
 *       可勾选项选中后菜单保持打开，其余选中后关闭并把焦点还给菜单栏。
 * ------------------------------------------------------------------------
 */

#include <algorithm>

#include "base/theme-tokens.hpp"
#include "rujian/ui/menu.hpp"

namespace rujian::ui {

namespace {

constexpr int kVirtualKeyReturn = 0x0D;
constexpr int kVirtualKeySpace = 0x20;
constexpr int kVirtualKeyLeft = 0x25;
constexpr int kVirtualKeyUp = 0x26;
constexpr int kVirtualKeyRight = 0x27;
constexpr int kVirtualKeyDown = 0x28;

TextStyle barTextStyle(Color color) {
  TextStyle style;
  style.fontSize = tokens::font_size_chrome;
  style.fontWeight = 400;
  style.lineHeight = 0;
  style.color = color;
  return style;
}

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

MenuBar::MenuBar() = default;

MenuBar::~MenuBar() {
  if (popup_ != nullptr && host() != nullptr) {
    popup_->closeSub();
    host()->hideLayer(popup_.get());
  }
}

void MenuBar::setMenus(std::vector<MenuBarMenu> menus) {
  menus_ = std::move(menus);
  focusIndex_ = std::min(focusIndex_, std::max(0, static_cast<int>(menus_.size()) - 1));
  if (popup_ == nullptr) {
    invalidate();
    return;
  }
  // 打开着时按新内容重建弹层，保留高亮项
  const std::string id = openId_;
  const int active = popup_->active();
  closeMenu(false);
  openMenu(id, active);
}

void MenuBar::setOnSelect(SelectHandler handler) { onSelect_ = std::move(handler); }

void MenuBar::setOnOpenChange(OpenHandler handler) { onOpenChange_ = std::move(handler); }

double MenuBar::buttonWidth(std::size_t index) const {
  return host()->measureText(menuItemText(MenuItem{.label = menus_[index].label, .mnemonic = menus_[index].mnemonic}), barTextStyle(kTransparent), 0).width + 2 * tokens::menubar_button_padding;
}

Rect MenuBar::buttonRect(std::size_t index) const {
  double x = bounds().x;
  for (std::size_t i = 0; i < index; ++i) x += buttonWidth(i);
  return Rect{x, bounds().y + (bounds().height - tokens::menubar_button_height) / 2.0, buttonWidth(index), tokens::menubar_button_height};
}

int MenuBar::indexAt(Point point) const {
  for (std::size_t i = 0; i < menus_.size(); ++i) {
    const Rect rect = buttonRect(i);
    if (point.x >= rect.x && point.x < rect.right() && point.y >= rect.y && point.y < rect.bottom()) return static_cast<int>(i);
  }
  return -1;
}

Size MenuBar::measure(Size /*available*/) {
  double width = 0.0;
  for (std::size_t i = 0; i < menus_.size(); ++i) width += buttonWidth(i);
  return Size{width, tokens::menubar_button_height};
}

void MenuBar::paint(Canvas& canvas) {
  const auto& colors = themeTokens(host()->theme());
  for (std::size_t i = 0; i < menus_.size(); ++i) {
    const Rect rect = buttonRect(i);
    const bool open = popup_ != nullptr && menus_[i].id == openId_;
    const bool lit = open || static_cast<int>(i) == pressedIndex_ || static_cast<int>(i) == hoverIndex_;
    if (lit) canvas.drawBox(rect, BoxStyle{.fill = colors.menubar_selection_bg, .radius = tokens::menubar_button_radius});
    const std::string label = menuItemText(MenuItem{.label = menus_[i].label, .mnemonic = menus_[i].mnemonic});
    canvas.drawText(label, Rect{rect.x + tokens::menubar_button_padding, rect.y, rect.width, rect.height}, barTextStyle(lit ? colors.chrome_text_strong : colors.chrome_text), VerticalAlign::Center);
  }
}

void MenuBar::openAt(std::size_t index, int itemIndex) {
  if (index >= menus_.size() || menus_[index].items.empty() || host() == nullptr) return;
  focusIndex_ = static_cast<int>(index);
  if (popup_ != nullptr && openId_ == menus_[index].id) {
    popup_->closeSub();
    popup_->setActive(itemIndex);
    return;
  }
  if (!focused()) host()->setFocus(this, false);
  if (popup_ != nullptr) {
    popup_->closeSub();
    host()->hideLayer(popup_.get());
    popup_.reset();
  }
  openId_ = menus_[index].id;
  popup_ = std::make_unique<MenuPopup>(*this, menus_[index].items);
  popup_->setMinWidth(buttonRect(index).width);
  host()->showLayer(popup_.get(), [this](Widget* hit) { menuOutsidePress(hit); });
  popup_->placeBelow(buttonRect(index));
  popup_->setActive(itemIndex);
  invalidate();
  if (onOpenChange_) onOpenChange_(openId_);
}

void MenuBar::openMenu(const std::string& menuId, int itemIndex) {
  for (std::size_t i = 0; i < menus_.size(); ++i) {
    if (menus_[i].id == menuId) {
      openAt(i, itemIndex);
      return;
    }
  }
}

void MenuBar::closeMenu(bool returnFocus) {
  if (popup_ == nullptr) return;
  popup_->closeSub();
  if (host() != nullptr) host()->hideLayer(popup_.get());
  popup_.reset();
  openId_.clear();
  invalidate();
  if (returnFocus && host() != nullptr && !focused()) host()->setFocus(this, true);
  if (onOpenChange_) onOpenChange_(openId_);
}

bool MenuBar::onMouseDown(const MouseEvent& event) {
  if (event.button != MouseButton::Left) return false;
  pressedIndex_ = indexAt(event.position);
  invalidate();
  return pressedIndex_ >= 0;
}

bool MenuBar::onMouseUp(const MouseEvent& event) {
  if (event.button != MouseButton::Left) return false;
  const int down = pressedIndex_;
  pressedIndex_ = -1;
  invalidate();
  if (down < 0 || down != indexAt(event.position)) return down >= 0;
  if (popup_ != nullptr && menus_[static_cast<std::size_t>(down)].id == openId_) closeMenu(true);
  // 鼠标点击打开时不高亮任何项（键盘打开时才高亮第一项）
  else openAt(static_cast<std::size_t>(down), -1);
  return true;
}

bool MenuBar::onMouseMove(const MouseEvent& event) {
  const int index = indexAt(event.position);
  if (index != hoverIndex_) {
    hoverIndex_ = index;
    invalidate();
  }
  // 已经打开着菜单时，移到别的菜单名上就切换过去（不高亮菜单项）
  if (index >= 0 && popup_ != nullptr && menus_[static_cast<std::size_t>(index)].id != openId_) openAt(static_cast<std::size_t>(index), -1);
  return index >= 0;
}

void MenuBar::onStateChanged() {
  Widget::onStateChanged();
  if (!hovered() && hoverIndex_ != -1) hoverIndex_ = -1;
  if (!hovered() && !pressed()) pressedIndex_ = -1;
  // 焦点离开菜单栏时关闭
  if (!focused() && popup_ != nullptr) closeMenu(false);
}

bool MenuBar::onKeyDown(const KeyEvent& event) {
  if (popup_ != nullptr) return popup_->handleKey(event);
  const int count = static_cast<int>(menus_.size());
  if (count == 0) return false;
  switch (event.virtualKey) {
    case kVirtualKeyRight: focusIndex_ = (focusIndex_ + 1) % count; invalidate(); return true;
    case kVirtualKeyLeft: focusIndex_ = (focusIndex_ - 1 + count) % count; invalidate(); return true;
    case kVirtualKeyDown:
    case kVirtualKeyReturn:
    case kVirtualKeySpace: openAt(static_cast<std::size_t>(focusIndex_), 0); return true;
    case kVirtualKeyUp: openAt(static_cast<std::size_t>(focusIndex_), static_cast<int>(menus_[static_cast<std::size_t>(focusIndex_)].items.size()) - 1); return true;
    default: return false;
  }
}

bool MenuBar::onChar(char32_t codePoint) { return popup_ != nullptr && popup_->handleChar(codePoint); }

void MenuBar::menuActivated(const std::vector<std::size_t>& path) {
  const auto found = std::ranges::find(menus_, openId_, &MenuBarMenu::id);
  if (found == menus_.end()) return;
  const MenuItem* item = resolveItem(found->items, path);
  if (item == nullptr || item->disabled) return;
  const std::string menuId = found->id;
  const std::string itemId = item->id;
  if (item->checked.has_value() && !item->radio) {
    // 可勾选项：切换后菜单保持打开，使用方更新菜单内容
    if (onSelect_) onSelect_(menuId, itemId, !*item->checked);
    return;
  }
  closeMenu(true);
  if (onSelect_) onSelect_(menuId, itemId, std::nullopt);
}

void MenuBar::menuClosed(bool returnFocus) { closeMenu(returnFocus); }

void MenuBar::menuSwitch(int delta) {
  const auto found = std::ranges::find(menus_, openId_, &MenuBarMenu::id);
  if (found == menus_.end()) return;
  const int count = static_cast<int>(menus_.size());
  const int current = static_cast<int>(found - menus_.begin());
  openAt(static_cast<std::size_t>((current + delta + count) % count), 0);
}

void MenuBar::menuOutsidePress(Widget* hit) {
  if (popup_ == nullptr) return;
  // 按在菜单栏自己身上由菜单栏处理（点击打开着的菜单名会关闭它）
  if (hit == this || (hit != nullptr && popup_->containsTree(hit))) return;
  closeMenu(false);
}

}  // namespace rujian::ui
