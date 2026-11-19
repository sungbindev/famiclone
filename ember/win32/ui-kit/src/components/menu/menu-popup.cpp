/*
 * ------------------------------------------------------------------------
 * 名称：menu-popup.cpp
 * 说明：菜单弹层的实现：外框（阴影、底色、边框）、各项的排版与绘制（勾选列、图标、文字、快捷键或提示、子菜单箭头、分隔线）、定位、鼠标与键盘操作、子菜单。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-19
 * 备注：项高 24，左右各留 4；勾选列宽 26；右侧留 26 的内边距，快捷键与子菜单箭头前再留 52 的间距；分隔线上下各 5。
 *       键盘高亮与鼠标悬停同色（菜单选中色），展开了子菜单的项保持这个颜色。
 * ------------------------------------------------------------------------
 */

#include <algorithm>
#include <cctype>

#include "base/theme-tokens.hpp"
#include "rujian/ui/geometry.hpp"
#include "rujian/ui/menu.hpp"

namespace rujian::ui {

namespace {

constexpr double kBorderWidth = 1.0;
constexpr double kPopupGap = 2.0;
constexpr double kViewportMargin = 8.0;
constexpr double kIconTextGap = tokens::icon_text_gap;
constexpr double kPaddingY = tokens::menu_popup_padding[0];
constexpr double kShortcutOpacity = 0.7;
constexpr int kVirtualKeyReturn = 0x0D;
constexpr int kVirtualKeyEscape = 0x1B;
constexpr int kVirtualKeySpace = 0x20;
constexpr int kVirtualKeyEnd = 0x23;
constexpr int kVirtualKeyHome = 0x24;
constexpr int kVirtualKeyLeft = 0x25;
constexpr int kVirtualKeyUp = 0x26;
constexpr int kVirtualKeyRight = 0x27;
constexpr int kVirtualKeyDown = 0x28;
constexpr int kVirtualKeyTab = 0x09;

TextStyle textStyle(Color color) {
  TextStyle style;
  style.fontSize = tokens::menu_font_size;
  style.lineHeight = 0;
  style.color = color;
  return style;
}

bool sameLetter(const std::string& mnemonic, char32_t codePoint) {
  if (mnemonic.empty() || codePoint > 0x7F) return false;
  return std::toupper(static_cast<unsigned char>(mnemonic[0])) == std::toupper(static_cast<int>(codePoint));
}

}  // namespace

std::string menuItemText(const MenuItem& item) {
  if (item.mnemonic.empty()) return item.label;
  std::string letter = item.mnemonic;
  for (char& c : letter) c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
  return item.label + "(" + letter + ")";
}

MenuPopup::MenuPopup(MenuPopupOwner& owner, std::vector<MenuItem> items, std::vector<std::size_t> path, MenuPopup* parent)
    : owner_(owner), parent_(parent), items_(std::move(items)), path_(std::move(path)) {}

MenuPopup::~MenuPopup() {
  if (host() != nullptr) closeSub();
}

void MenuPopup::setMinWidth(double minWidth) { minWidth_ = minWidth; }

double MenuPopup::border() const { return snapBorderWidth(kBorderWidth, host() != nullptr ? host()->scale() : 1.0); }

std::vector<MenuPopup::Row> MenuPopup::rows() const {
  std::vector<Row> result;
  const double hairline = snapBorderWidth(1.0, host() != nullptr ? host()->scale() : 1.0);
  double y = kPaddingY;
  for (std::size_t i = 0; i < items_.size(); ++i) {
    if (items_[i].separatorBefore && i > 0) y += tokens::menu_separator_margin_y * 2 + hairline;
    result.push_back(Row{y, tokens::menu_item_height});
    y += tokens::menu_item_height;
  }
  return result;
}

double MenuPopup::naturalHeight() const {
  const std::vector<Row> laid = rows();
  const double content = laid.empty() ? 0.0 : laid.back().top + laid.back().height;
  return 2 * border() + content + kPaddingY;
}

double MenuPopup::itemWidth(const MenuItem& item) {
  double width = tokens::menu_mark_column_width + host()->measureText(menuItemText(item), textStyle(kTransparent), 0).width;
  if (item.icon) width += tokens::icon_size + kIconTextGap;
  if (!item.items.empty()) width += tokens::menu_shortcut_gap + tokens::icon_size;
  else if (!item.shortcut.empty()) width += tokens::menu_shortcut_gap + host()->measureText(item.shortcut, textStyle(kTransparent), 0).width;
  else if (!item.hint.empty()) width += tokens::menu_shortcut_gap + host()->measureText(item.hint, textStyle(kTransparent), 0).width;
  return width + tokens::menu_item_padding_right;
}

double MenuPopup::naturalWidth() {
  double widest = 0.0;
  for (const MenuItem& item : items_) widest = std::max(widest, itemWidth(item));
  return std::max(minWidth_, widest + 2 * tokens::menu_item_margin_x + 2 * border());
}

Size MenuPopup::measure(Size /*available*/) { return Size{naturalWidth(), naturalHeight()}; }

void MenuPopup::placeBelow(const Rect& anchor) {
  const Size viewport = host()->viewportSize();
  const Size size = measure(Size{});
  const double below = viewport.height - anchor.bottom() - kViewportMargin;
  const double above = anchor.y - kViewportMargin;
  const bool placeBelow = below >= size.height || below >= above;
  const double height = std::min(size.height, std::max(placeBelow ? below : above, 0.0));
  const double left = std::max(kViewportMargin, std::min(anchor.x, viewport.width - kViewportMargin - size.width));
  arrange(Rect{left, placeBelow ? anchor.bottom() + kPopupGap : anchor.y - kPopupGap - height, size.width, height});
}

void MenuPopup::placeBeside(const Rect& parentItem) {
  const Size viewport = host()->viewportSize();
  const Size size = measure(Size{});
  const double height = std::min(size.height, std::max(viewport.height - 2 * kViewportMargin, 0.0));
  const bool fitsRight = parentItem.right() + size.width <= viewport.width - kViewportMargin;
  const bool fitsLeft = parentItem.x - size.width >= kViewportMargin;
  double left = viewport.width - kViewportMargin - size.width;
  if (fitsRight) left = parentItem.right();
  else if (fitsLeft) left = parentItem.x - size.width;
  const double top = std::max(kViewportMargin, std::min(parentItem.y, viewport.height - kViewportMargin - height));
  arrange(Rect{std::max(kViewportMargin, left), top, size.width, height});
}

Rect MenuPopup::itemRect(std::size_t index) const {
  const std::vector<Row> laid = rows();
  const double inner = border();
  return Rect{bounds().x + inner + tokens::menu_item_margin_x, bounds().y + inner + laid[index].top, bounds().width - 2 * inner - 2 * tokens::menu_item_margin_x, laid[index].height};
}

int MenuPopup::indexAt(Point point) const {
  for (std::size_t i = 0; i < items_.size(); ++i) {
    const Rect row = itemRect(i);
    if (point.y >= row.y && point.y < row.bottom()) return static_cast<int>(i);
  }
  return -1;
}

bool MenuPopup::containsTree(const Widget* widget) const { return widget == this || (sub_ != nullptr && sub_->containsTree(widget)); }

void MenuPopup::setActive(int index) {
  active_ = index;
  invalidate();
}

void MenuPopup::closeSub() {
  if (sub_ == nullptr) return;
  sub_->closeSub();
  if (host() != nullptr) host()->hideLayer(sub_.get());
  sub_.reset();
  subIndex_ = -1;
  invalidate();
}

void MenuPopup::openSub(std::size_t index, bool focusFirst) {
  if (subIndex_ == static_cast<int>(index) && sub_ != nullptr) {
    if (focusFirst) sub_->setActive(0);
    return;
  }
  closeSub();
  subIndex_ = static_cast<int>(index);
  std::vector<std::size_t> childPath = path_;
  childPath.push_back(index);
  sub_ = std::make_unique<MenuPopup>(owner_, items_[index].items, std::move(childPath), this);
  host()->showLayer(sub_.get(), [this](Widget* hit) { owner_.menuOutsidePress(hit); });
  sub_->placeBeside(itemRect(index));
  if (focusFirst) sub_->setActive(0);
  invalidate();
}

void MenuPopup::activate(std::size_t index) {
  if (index >= items_.size() || items_[index].disabled) return;
  if (!items_[index].items.empty()) {
    openSub(index, true);
    return;
  }
  std::vector<std::size_t> path = path_;
  path.push_back(index);
  owner_.menuActivated(path);
}

void MenuPopup::move(int delta) {
  const int count = static_cast<int>(items_.size());
  if (count == 0) return;
  if (active_ < 0) setActive(delta > 0 ? 0 : count - 1);
  else setActive((active_ + delta + count) % count);
}

MenuPopup* MenuPopup::keyboardTarget() {
  if (sub_ != nullptr && sub_->active_ >= 0) return sub_->keyboardTarget();
  return this;
}

bool MenuPopup::handleKey(const KeyEvent& event) { return keyboardTarget()->keyHere(event); }

bool MenuPopup::handleChar(char32_t codePoint) { return keyboardTarget()->charHere(codePoint); }

bool MenuPopup::keyHere(const KeyEvent& event) {
  const bool hasSub = active_ >= 0 && !items_[static_cast<std::size_t>(active_)].items.empty();
  switch (event.virtualKey) {
    case kVirtualKeyRight:
      if (hasSub) activate(static_cast<std::size_t>(active_));
      else owner_.menuSwitch(1);
      return true;
    case kVirtualKeyLeft:
      // 子菜单里回到父项；顶层弹层里切到左边的菜单
      if (parent_ != nullptr) {
        MenuPopup* parent = parent_;
        parent->active_ = parent->subIndex_;
        parent->closeSub();
      } else {
        owner_.menuSwitch(-1);
      }
      return true;
    case kVirtualKeyDown: move(1); return true;
    case kVirtualKeyUp: move(-1); return true;
    case kVirtualKeyHome: setActive(0); return true;
    case kVirtualKeyEnd: setActive(static_cast<int>(items_.size()) - 1); return true;
    case kVirtualKeyReturn:
    case kVirtualKeySpace:
      if (active_ >= 0) activate(static_cast<std::size_t>(active_));
      return true;
    case kVirtualKeyEscape:
      // 只关掉这一层：子菜单回到父项，顶层关掉整个菜单
      if (parent_ != nullptr) {
        MenuPopup* parent = parent_;
        parent->active_ = parent->subIndex_;
        parent->closeSub();
      } else {
        owner_.menuClosed(true);
      }
      return true;
    case kVirtualKeyTab:
      owner_.menuClosed(false);
      return false;
    default:
      return false;
  }
}

bool MenuPopup::charHere(char32_t codePoint) {
  for (std::size_t i = 0; i < items_.size(); ++i) {
    if (sameLetter(items_[i].mnemonic, codePoint)) {
      activate(i);
      return true;
    }
  }
  return false;
}

bool MenuPopup::onMouseDown(const MouseEvent& event) {
  if (event.button != MouseButton::Left) return false;
  downIndex_ = indexAt(event.position);
  return true;
}

bool MenuPopup::onMouseUp(const MouseEvent& event) {
  if (event.button != MouseButton::Left) return false;
  const int index = indexAt(event.position);
  const int down = downIndex_;
  downIndex_ = -1;
  if (index >= 0 && index == down) activate(static_cast<std::size_t>(index));
  return true;
}

bool MenuPopup::onMouseMove(const MouseEvent& event) {
  const int index = indexAt(event.position);
  if (index < 0) return true;
  if (index != active_) setActive(index);
  if (subIndex_ == index) return true;
  const MenuItem& item = items_[static_cast<std::size_t>(index)];
  if (!item.items.empty() && !item.disabled) openSub(static_cast<std::size_t>(index), false);
  else closeSub();
  return true;
}

void MenuPopup::onStateChanged() {
  Widget::onStateChanged();
  // 鼠标移出弹层后去掉高亮
  if (!hovered() && active_ >= 0 && sub_ == nullptr) active_ = -1;
  else if (!hovered() && active_ >= 0 && sub_ != nullptr && sub_->active_ < 0 && !sub_->hovered()) active_ = subIndex_;
}

void MenuPopup::paint(Canvas& canvas) {
  const auto& colors = themeTokens(host()->theme());
  canvas.drawShadow(bounds(), tokens::menu_popup_radius, colors.menu_shadow);
  canvas.drawBox(bounds(), BoxStyle{.fill = colors.menu_background, .radius = tokens::menu_popup_radius, .borderWidth = kBorderWidth, .borderColor = colors.menu_border});
  const double inner = border();
  canvas.pushClip(bounds().inset(Insets::all(inner)), std::max(0.0, tokens::menu_popup_radius - inner));
  const std::vector<Row> laid = rows();
  const double hairline = snapBorderWidth(1.0, host()->scale());
  for (std::size_t i = 0; i < items_.size(); ++i) {
    const MenuItem& item = items_[i];
    const Rect row = itemRect(i);
    if (item.separatorBefore && i > 0) {
      canvas.fillRect(Rect{bounds().x + inner, row.y - tokens::menu_separator_margin_y - hairline, bounds().width - 2 * inner, hairline}, colors.menu_border);
    }
    const bool highlighted = !item.disabled && (static_cast<int>(i) == active_ || (static_cast<int>(i) == subIndex_ && sub_ != nullptr));
    if (highlighted) canvas.drawBox(row, BoxStyle{.fill = colors.menu_selection_background, .radius = tokens::menu_item_radius});
    const Color color = item.disabled ? colors.disabled_text : (highlighted ? colors.menu_selection_foreground : colors.menu_foreground);
    // 勾选标记列
    if (item.checked.value_or(false)) {
      canvas.drawIcon(IconId::Check, Rect{row.x + (tokens::menu_mark_column_width - tokens::menu_mark_size) / 2.0, row.y + (row.height - tokens::menu_mark_size) / 2.0, tokens::menu_mark_size, tokens::menu_mark_size}, color, tokens::icon_stroke_width);
    }
    double labelX = row.x + tokens::menu_mark_column_width;
    if (item.icon) {
      canvas.drawIcon(*item.icon, Rect{labelX, row.y + (row.height - tokens::icon_size) / 2.0, tokens::icon_size, tokens::icon_size}, color, tokens::icon_stroke_width);
      labelX += tokens::icon_size + kIconTextGap;
    }
    const double rightEdge = row.right() - tokens::menu_item_padding_right;
    const std::string label = menuItemText(item);
    canvas.drawText(label, Rect{labelX, row.y, std::max(0.0, rightEdge - labelX), row.height}, textStyle(color), VerticalAlign::Center);
    if (!item.items.empty()) {
      canvas.drawIcon(IconId::ChevronRight, Rect{rightEdge - tokens::icon_size, row.y + (row.height - tokens::icon_size) / 2.0, tokens::icon_size, tokens::icon_size}, color, tokens::icon_stroke_width);
    } else if (const std::string& trailing = !item.shortcut.empty() ? item.shortcut : item.hint; !trailing.empty()) {
      const double width = host()->measureText(trailing, textStyle(kTransparent), 0).width;
      canvas.drawText(trailing, Rect{rightEdge - width, row.y, width + 1, row.height}, textStyle(color.withOpacity(kShortcutOpacity)), VerticalAlign::Center);
    }
  }
  canvas.popClip();
}

}  // namespace rujian::ui
