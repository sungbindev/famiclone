/*
 * ------------------------------------------------------------------------
 * 名称：root.cpp
 * 说明：根容器的实现。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-19
 * 备注：事件分发规则：从目标控件开始向父控件冒泡，不可用的控件跳过，第一个返回 true 的控件处理该事件；
 *       鼠标被捕获时，移动与抬起事件发给捕获者；按下左键时焦点移到最近的可获得焦点的祖先（含自己），没有则清除焦点。
 * ------------------------------------------------------------------------
 */

#include "rujian/ui/root.hpp"

#include <algorithm>

#include "base/graphics.hpp"

namespace rujian::ui {

namespace {

constexpr int kVirtualKeyTab = 0x09;

/** 从目标开始向上冒泡，返回第一个处理了事件的控件，没有则为空。 */
template <typename Handler>
Widget* bubble(Widget* from, Handler&& handler) {
  for (Widget* widget = from; widget != nullptr; widget = widget->parent()) {
    if (widget->enabled() && handler(*widget)) return widget;
  }
  return nullptr;
}

/** 控件自己与全部祖先都可见。 */
bool chainVisible(const Widget* widget) {
  for (; widget != nullptr; widget = widget->parent()) {
    if (!widget->visible()) return false;
  }
  return true;
}

}  // namespace

Root::Root(ThemeName theme, double scale) : theme_(theme), scale_(scale) {}

Root::~Root() = default;

void Root::setContent(std::unique_ptr<Widget> content) {
  if (content_) content_->detach();
  content_ = std::move(content);
  if (content_) content_->attach(this);
  invalidate();
}

void Root::setTheme(ThemeName theme) {
  theme_ = theme;
  invalidate();
}

void Root::setScale(double scale) {
  scale_ = scale;
  invalidate();
}

void Root::layout(Size size) {
  size_ = size;
  if (content_) content_->arrange(Rect{0, 0, size.width, size.height});
  invalidate();
}

void Root::paint(Canvas& canvas) {
  if (content_) content_->paintTree(canvas);
}

void Root::invalidate() {
  if (invalidateHandler_) invalidateHandler_();
}

TextMetrics Root::measureText(std::string_view text, const TextStyle& style, double maxWidth) { return Graphics::shared().text().measure(text, style, maxWidth); }

void Root::widgetLeft(Widget* widget) {
  if (focus_ == widget) focus_ = nullptr;
  if (hover_ == widget) hover_ = nullptr;
  if (pressedWidget_ == widget) pressedWidget_ = nullptr;
  if (capture_ == widget) capture_ = nullptr;
}

void Root::updateHover(Widget* target) {
  if (target == hover_) return;
  const auto chainOf = [](Widget* leaf) {
    std::vector<Widget*> chain;
    for (Widget* widget = leaf; widget != nullptr; widget = widget->parent()) chain.push_back(widget);
    return chain;
  };
  const std::vector<Widget*> before = chainOf(hover_);
  const std::vector<Widget*> after = chainOf(target);
  hover_ = target;
  for (Widget* widget : before) {
    if (std::ranges::find(after, widget) == after.end()) {
      widget->hovered_ = false;
      widget->onStateChanged();
    }
  }
  for (Widget* widget : after) {
    if (std::ranges::find(before, widget) == before.end()) {
      widget->hovered_ = true;
      widget->onStateChanged();
    }
  }
}

void Root::mouseMove(Point position, unsigned modifiers) {
  if (!content_) return;
  Widget* hit = content_->hitTest(position);
  updateHover(hit);
  const MouseEvent event{position, MouseButton::None, modifiers, 0};
  bubble(capture_ != nullptr ? capture_ : hit, [&](Widget& widget) { return widget.onMouseMove(event); });
}

void Root::mouseDown(Point position, MouseButton button, unsigned modifiers) {
  if (!content_) return;
  Widget* hit = capture_ != nullptr ? capture_ : content_->hitTest(position);
  updateHover(content_->hitTest(position));
  if (button == MouseButton::Left) {
    Widget* focusTarget = hit;
    while (focusTarget != nullptr && !(focusTarget->enabled() && focusTarget->focusable())) focusTarget = focusTarget->parent();
    setFocus(focusTarget, false);
  }
  const MouseEvent event{position, button, modifiers, 0};
  Widget* handled = bubble(hit, [&](Widget& widget) { return widget.onMouseDown(event); });
  if (handled != nullptr && button == MouseButton::Left) {
    pressedWidget_ = handled;
    handled->pressed_ = true;
    handled->onStateChanged();
  }
}

void Root::mouseUp(Point position, MouseButton button, unsigned modifiers) {
  if (!content_) return;
  const MouseEvent event{position, button, modifiers, 0};
  if (button == MouseButton::Left && pressedWidget_ != nullptr) {
    Widget* target = pressedWidget_;
    // 先把事件交给按下的控件（它据此判断是否算一次点击），再清除按下状态
    target->onMouseUp(event);
    if (pressedWidget_ == target) {
      pressedWidget_ = nullptr;
      target->pressed_ = false;
      target->onStateChanged();
    }
    return;
  }
  Widget* hit = capture_ != nullptr ? capture_ : content_->hitTest(position);
  bubble(hit, [&](Widget& widget) { return widget.onMouseUp(event); });
}

void Root::mouseWheel(Point position, double delta, unsigned modifiers) {
  if (!content_) return;
  Widget* hit = capture_ != nullptr ? capture_ : content_->hitTest(position);
  const MouseEvent event{position, MouseButton::None, modifiers, delta};
  bubble(hit, [&](Widget& widget) { return widget.onMouseWheel(event); });
}

void Root::mouseLeave() { updateHover(nullptr); }

void Root::setFocus(Widget* widget, bool visible) {
  if (widget != nullptr && !(widget->host_ == this && widget->enabled())) return;
  if (widget == focus_) {
    if (widget != nullptr && widget->focusVisible_ != visible) {
      widget->focusVisible_ = visible;
      widget->onStateChanged();
    }
    return;
  }
  Widget* previous = focus_;
  focus_ = widget;
  if (previous != nullptr) {
    previous->focused_ = false;
    previous->focusVisible_ = false;
    previous->onStateChanged();
  }
  if (widget != nullptr) {
    widget->focused_ = true;
    widget->focusVisible_ = visible;
    widget->onStateChanged();
  }
}

void Root::collectFocusable(Widget* widget, std::vector<Widget*>& out) {
  if (!widget->visible()) return;
  if (widget->enabled() && widget->focusable()) out.push_back(widget);
  for (const auto& child : widget->children()) collectFocusable(child.get(), out);
}

void Root::moveFocus(bool backward) {
  if (!content_) return;
  std::vector<Widget*> candidates;
  collectFocusable(content_.get(), candidates);
  if (candidates.empty()) return;
  const auto current = std::ranges::find(candidates, focus_);
  std::size_t index = 0;
  if (current == candidates.end()) {
    index = backward ? candidates.size() - 1 : 0;
  } else {
    const auto position = static_cast<std::size_t>(current - candidates.begin());
    index = backward ? (position + candidates.size() - 1) % candidates.size() : (position + 1) % candidates.size();
  }
  setFocus(candidates[index], true);
}

void Root::keyDown(int virtualKey, unsigned modifiers, bool repeat) {
  const KeyEvent event{virtualKey, modifiers, repeat};
  if (focus_ != nullptr && chainVisible(focus_) && bubble(focus_, [&](Widget& widget) { return widget.onKeyDown(event); }) != nullptr) return;
  if (virtualKey == kVirtualKeyTab && (modifiers & (kModifierCtrl | kModifierAlt)) == 0) moveFocus((modifiers & kModifierShift) != 0);
}

void Root::keyUp(int virtualKey, unsigned modifiers) {
  const KeyEvent event{virtualKey, modifiers, false};
  if (focus_ != nullptr) bubble(focus_, [&](Widget& widget) { return widget.onKeyUp(event); });
}

void Root::character(char32_t codePoint) {
  if (focus_ != nullptr) bubble(focus_, [&](Widget& widget) { return widget.onChar(codePoint); });
}

}  // namespace rujian::ui
