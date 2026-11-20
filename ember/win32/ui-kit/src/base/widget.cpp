/*
 * ------------------------------------------------------------------------
 * 名称：widget.cpp
 * 说明：控件基类的实现。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-19
 * 备注：控件销毁或离开控件树时，要通知宿主清理焦点、悬停、捕获对它的引用。
 * ------------------------------------------------------------------------
 */

#include "rujian/ui/widget.hpp"

#include <algorithm>

namespace rujian::ui {

Widget::~Widget() {
  if (host_ != nullptr) host_->widgetLeft(this);
}

Widget* Widget::addChild(std::unique_ptr<Widget> child) {
  Widget* raw = child.get();
  raw->parent_ = this;
  children_.push_back(std::move(child));
  if (host_ != nullptr) raw->attach(host_);
  invalidate();
  return raw;
}

std::unique_ptr<Widget> Widget::removeChild(Widget* child) {
  const auto found = std::ranges::find_if(children_, [child](const std::unique_ptr<Widget>& item) { return item.get() == child; });
  if (found == children_.end()) return nullptr;
  std::unique_ptr<Widget> owned = std::move(*found);
  children_.erase(found);
  Host* host = host_;
  owned->detach();
  owned->parent_ = nullptr;
  if (host != nullptr) host->invalidate();
  return owned;
}

void Widget::attach(Host* host) {
  host_ = host;
  for (const auto& child : children_) child->attach(host);
}

void Widget::detach() {
  for (const auto& child : children_) child->detach();
  if (host_ != nullptr) host_->widgetLeft(this);
  host_ = nullptr;
  hovered_ = false;
  pressed_ = false;
  focused_ = false;
}

Size Widget::measure(Size /*available*/) { return Size{}; }

Size Widget::minimumSize() { return Size{}; }

void Widget::arrange(const Rect& bounds) {
  bounds_ = bounds;
  for (const auto& child : children_) child->arrange(bounds);
}

void Widget::setVisible(bool visible) {
  if (visible_ == visible) return;
  visible_ = visible;
  invalidate();
}

void Widget::setEnabled(bool enabled) {
  if (enabled_ == enabled) return;
  enabled_ = enabled;
  if (!enabled_ && host_ != nullptr && host_->focusedWidget() == this) host_->setFocus(nullptr, false);
  onStateChanged();
}

void Widget::invalidate() {
  if (host_ != nullptr) host_->invalidate();
}

void Widget::requestLayout() {
  if (host_ != nullptr) host_->requestLayout();
}

void Widget::setTooltip(std::string text) { tooltip_ = TooltipContent{.text = std::move(text)}; }

void Widget::setRichTooltip(TooltipContent content) {
  content.rich = true;
  tooltip_ = std::move(content);
}

void Widget::paintTree(Canvas& canvas) {
  if (!visible_) return;
  paint(canvas);
  paintChildren(canvas);
}

void Widget::paint(Canvas& /*canvas*/) {}

void Widget::paintChildren(Canvas& canvas) {
  for (const auto& child : children_) child->paintTree(canvas);
}

Widget* Widget::hitTest(Point point) {
  if (!visible_ || !bounds_.contains(point)) return nullptr;
  for (auto it = children_.rbegin(); it != children_.rend(); ++it) {
    if (Widget* hit = (*it)->hitTest(point)) return hit;
  }
  return this;
}

void Widget::layerRects(std::vector<Rect>& out) const {
  if (visible_ && bounds_.width > 0 && bounds_.height > 0) out.push_back(bounds_);
}

bool Widget::onMouseDown(const MouseEvent& /*event*/) { return false; }
bool Widget::onMouseUp(const MouseEvent& /*event*/) { return false; }
bool Widget::onMouseMove(const MouseEvent& /*event*/) { return false; }
bool Widget::onMouseWheel(const MouseEvent& /*event*/) { return false; }
bool Widget::onKeyDown(const KeyEvent& /*event*/) { return false; }
bool Widget::onKeyUp(const KeyEvent& /*event*/) { return false; }
bool Widget::onChar(char32_t /*codePoint*/) { return false; }
void Widget::onImeComposition(const ImeComposition& /*composition*/) {}
void Widget::onImeCommit(std::u16string_view /*text*/) {}
void Widget::onImeEnd() {}

void Widget::onStateChanged() { invalidate(); }

void Widget::onTimer(unsigned /*id*/) {}

void Widget::onDescendantFocused(Widget* /*descendant*/) {}

}  // namespace rujian::ui
