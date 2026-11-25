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
#include <cmath>

#include "base/clipboard.hpp"
#include "base/graphics.hpp"

namespace rujian::ui {

namespace {

constexpr int kVirtualKeyTab = 0x09;
/** 文字光标闪烁的半周期（毫秒），与系统默认的 500 毫秒一致。 */
constexpr unsigned kCaretBlinkMilliseconds = 500;

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

/** 悬停提示的延迟定时器的持有者：定时器到时间后调用给定的函数。 */
class TooltipTimerOwner final : public Widget {
 public:
  explicit TooltipTimerOwner(std::function<void()> fire) : fire_(std::move(fire)) {}
  void onTimer(unsigned /*id*/) override { fire_(); }

 private:
  std::function<void()> fire_;
};

}  // namespace

Root::Root(ThemeName theme, double scale) : theme_(theme), scale_(scale) {
  tooltipTimerOwner_ = std::make_unique<TooltipTimerOwner>([this] {
    tooltipTimer_ = 0;
    showTooltipNow();
  });
}

Root::~Root() {
  // 弹层由调用方拥有，它们销毁时不能再访问已经开始销毁的 Root
  const std::vector<Layer> layers = layers_;
  layers_.clear();
  for (const Layer& layer : layers) layer.widget->detach();
}

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
  const bool changed = !(size == size_);
  size_ = size;
  if (content_) content_->arrange(Rect{0, 0, size.width, size.height});
  // 窗口大小变化时弹层的位置作废，通知它们自己关闭
  if (changed) {
    const std::vector<Layer> layers = layers_;
    for (const Layer& layer : layers) layer.onOutsidePress(nullptr);
  }
  invalidate();
}

void Root::dismissLayers() {
  const std::vector<Layer> layers = layers_;
  for (const Layer& layer : layers) layer.onOutsidePress(nullptr);
}

void Root::paint(Canvas& canvas) {
  if (content_) content_->paintTree(canvas);
  for (const Layer& layer : layers_) layer.widget->paintTree(canvas);
  if (!layerRectsHandler_) return;
  std::vector<Rect> rects;
  for (const Layer& layer : layers_) layer.widget->layerRects(rects);
  if (rects == lastLayerRects_) return;
  lastLayerRects_ = rects;
  layerRectsHandler_(rects);
}

void Root::showLayer(Widget* layer, std::function<void(Widget* hit)> onOutsidePress) {
  if (layer == nullptr || std::ranges::any_of(layers_, [layer](const Layer& item) { return item.widget == layer; })) return;
  layer->attach(this);
  layers_.push_back(Layer{layer, std::move(onOutsidePress)});
  invalidate();
}

void Root::hideLayer(Widget* layer) {
  const auto found = std::ranges::find_if(layers_, [layer](const Layer& item) { return item.widget == layer; });
  if (found == layers_.end()) return;
  layers_.erase(found);
  layer->detach();
  invalidate();
}

Widget* Root::hitTarget(Point point) {
  for (auto it = layers_.rbegin(); it != layers_.rend(); ++it) {
    if (Widget* hit = it->widget->hitTest(point)) return hit;
  }
  return content_ ? content_->hitTest(point) : nullptr;
}

unsigned Root::startTimer(Widget* owner, unsigned milliseconds) {
  const unsigned id = nextTimer_++;
  timers_[id] = owner;
  if (timerStart_) timerStart_(id, milliseconds);
  return id;
}

void Root::stopTimer(unsigned id) {
  if (timers_.erase(id) > 0 && timerStop_) timerStop_(id);
}

void Root::setTimerHandlers(std::function<void(unsigned id, unsigned milliseconds)> start, std::function<void(unsigned id)> stop) {
  timerStart_ = std::move(start);
  timerStop_ = std::move(stop);
}

void Root::fireTimer(unsigned id) {
  const auto found = timers_.find(id);
  if (found == timers_.end()) return;
  Widget* owner = found->second;
  timers_.erase(found);
  if (timerStop_) timerStop_(id);
  owner->onTimer(id);
}

std::vector<unsigned> Root::pendingTimers() const {
  std::vector<unsigned> ids;
  for (const auto& [id, owner] : timers_) ids.push_back(id);
  return ids;
}

bool Root::inLayer(const Widget* widget) const {
  const Widget* top = widget;
  while (top != nullptr && top->parent() != nullptr) top = top->parent();
  return top != nullptr && std::ranges::any_of(layers_, [top](const Layer& item) { return item.widget == top; });
}

void Root::invalidate() {
  if (invalidateHandler_) invalidateHandler_();
}

TextMetrics Root::measureText(std::string_view text, const TextStyle& style, double maxWidth) {
  Graphics::shared().text().setScale(scale_);
  return Graphics::shared().text().measure(text, style, maxWidth);
}

TextMetrics Root::measureText16(std::u16string_view text, const TextStyle& style, double maxWidth) {
  Graphics::shared().text().setScale(scale_);
  return Graphics::shared().text().measure(text, style, maxWidth);
}

double Root::caretOffset(std::u16string_view text, const TextStyle& style, std::size_t index) {
  Graphics::shared().text().setScale(scale_);
  return Graphics::shared().text().caretOffset(text, style, index);
}

std::size_t Root::caretIndexAt(std::u16string_view text, const TextStyle& style, double x) {
  Graphics::shared().text().setScale(scale_);
  return Graphics::shared().text().caretIndexAt(text, style, x);
}

void Root::setClipboardHandlers(std::function<std::optional<std::string>()> read, std::function<void(std::string_view)> write) {
  clipboardRead_ = std::move(read);
  clipboardWrite_ = std::move(write);
}

std::optional<std::string> Root::clipboardText() { return clipboardRead_ ? clipboardRead_() : readClipboardText(); }

void Root::setClipboardText(std::string_view text) {
  if (clipboardWrite_) clipboardWrite_(text);
  else writeClipboardText(text);
}

void Root::restartCaretBlink() {
  caretPhase_ = true;
  invalidate();
  if (caretTimerHandler_) caretTimerHandler_(kCaretBlinkMilliseconds);
}

void Root::toggleCaret() {
  caretPhase_ = !caretPhase_;
  invalidate();
}

void Root::widgetLeft(Widget* widget) {
  if (tooltipTarget_ == widget) hideTooltip();
  if (tooltipBlocked_ == widget) tooltipBlocked_ = nullptr;
  if (focus_ == widget) focus_ = nullptr;
  if (hover_ == widget) hover_ = nullptr;
  if (pressedWidget_ == widget) pressedWidget_ = nullptr;
  if (capture_ == widget) capture_ = nullptr;
  std::erase_if(layers_, [widget](const Layer& item) { return item.widget == widget; });
  for (auto it = timers_.begin(); it != timers_.end();) {
    if (it->second != widget) {
      ++it;
      continue;
    }
    if (timerStop_) timerStop_(it->first);
    it = timers_.erase(it);
  }
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
  Widget* hit = hitTarget(position);
  updateHover(hit);
  updateTooltip(hit, position);
  const MouseEvent event{position, MouseButton::None, modifiers, 0};
  bubble(capture_ != nullptr ? capture_ : hit, [&](Widget& widget) { return widget.onMouseMove(event); });
}

void Root::mouseDown(Point position, MouseButton button, unsigned modifiers, unsigned timeMs) {
  if (!content_) return;
  dismissTooltip();
  // 连续点击：同一个按键在规定时间和范围内再次按下，次数 +1，三连击之后重新从 1 开始
  const bool repeated = timeMs != 0 && button == lastClickButton_ && timeMs - lastClickTime_ <= doubleClickMs_ && std::hypot(position.x - lastClickPosition_.x, position.y - lastClickPosition_.y) <= doubleClickSlop_;
  clickCount_ = repeated ? clickCount_ % 3 + 1 : 1;
  lastClickTime_ = timeMs;
  lastClickPosition_ = position;
  lastClickButton_ = button;
  Widget* hit = capture_ != nullptr ? capture_ : hitTarget(position);
  updateHover(hitTarget(position));
  // 在弹层之外按下：先让弹层决定是否关闭；在弹层上按下不改变键盘焦点
  const bool onLayer = inLayer(hit);
  if (!layers_.empty() && !onLayer) {
    const std::vector<Layer> layers = layers_;
    for (auto it = layers.rbegin(); it != layers.rend(); ++it) it->onOutsidePress(hit);
  }
  if ((button == MouseButton::Left || button == MouseButton::Right) && !onLayer) {
    Widget* focusTarget = hit;
    while (focusTarget != nullptr && !(focusTarget->enabled() && focusTarget->focusable())) focusTarget = focusTarget->parent();
    setFocus(focusTarget, false);
  }
  const MouseEvent event{position, button, modifiers, 0, clickCount_};
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
  Widget* hit = capture_ != nullptr ? capture_ : hitTarget(position);
  bubble(hit, [&](Widget& widget) { return widget.onMouseUp(event); });
}

void Root::mouseWheel(Point position, double delta, unsigned modifiers) {
  if (!content_) return;
  dismissTooltip();
  Widget* hit = capture_ != nullptr ? capture_ : hitTarget(position);
  const MouseEvent event{position, MouseButton::None, modifiers, delta};
  bubble(hit, [&](Widget& widget) { return widget.onMouseWheel(event); });
}

void Root::mouseLeave() {
  updateHover(nullptr);
  hideTooltip();
}

void Root::setTooltipPresenter(TooltipPresenter* presenter) {
  if (tooltipPresenter_ != nullptr && tooltipPresenter_ != presenter) hideTooltip();
  tooltipPresenter_ = presenter;
}

void Root::hideTooltip() {
  if (tooltipTimer_ != 0) stopTimer(tooltipTimer_);
  tooltipTimer_ = 0;
  if (tooltipVisible_ && tooltipPresenter_ != nullptr) tooltipPresenter_->hideTooltip();
  tooltipVisible_ = false;
  tooltipTarget_ = nullptr;
  tooltipBlocked_ = nullptr;
}

void Root::dismissTooltip() {
  Widget* target = tooltipTarget_;
  hideTooltip();
  tooltipBlocked_ = target;
}

void Root::showTooltipNow() {
  if (tooltipTarget_ == nullptr || tooltipPresenter_ == nullptr || !tooltipTarget_->tooltip()) return;
  tooltipPresenter_->showTooltip(*tooltipTarget_, *tooltipTarget_->tooltip(), tooltipPointer_);
  tooltipVisible_ = true;
}

void Root::updateTooltip(Widget* hit, Point position) {
  // 找鼠标所在控件起最近的带提示的控件；空文字的纯文字提示表示这里没有提示，盖住外层的
  Widget* element = nullptr;
  for (Widget* widget = hit; widget != nullptr; widget = widget->parent()) {
    if (widget->tooltip()) {
      element = widget;
      break;
    }
  }
  const bool blank = element != nullptr && !element->tooltip()->rich && element->tooltip()->text.find_first_not_of(" \t\r\n") == std::string::npos;
  if (element == nullptr || blank) {
    hideTooltip();
    return;
  }
  if (element == tooltipBlocked_) return;
  tooltipBlocked_ = nullptr;
  tooltipPointer_ = position;
  if (element == tooltipTarget_) return;
  // 已有提示显示时，移到下一个控件立即弹出
  const bool warm = tooltipVisible_;
  hideTooltip();
  tooltipTarget_ = element;
  if (tooltipPresenter_ == nullptr) return;
  if (warm) showTooltipNow();
  else tooltipTimer_ = startTimer(tooltipTimerOwner_.get(), kTooltipDelayMilliseconds);
}

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
    for (Widget* ancestor = widget->parent(); ancestor != nullptr; ancestor = ancestor->parent()) ancestor->onDescendantFocused(widget);
  }
  // 文字输入控件获得焦点时光标立刻显示并开始闪烁，其他情况停止定时器
  if (widget != nullptr && widget->acceptsText()) restartCaretBlink();
  else if (caretTimerHandler_) caretTimerHandler_(0);
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
  std::erase_if(candidates, [this](const Widget* widget) { return !widget->tabStop() && widget != focus_; });
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
  dismissTooltip();
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

void Root::imeComposition(const ImeComposition& composition) {
  if (focus_ != nullptr && focus_->acceptsText()) focus_->onImeComposition(composition);
}

void Root::imeCommit(std::u16string_view text) {
  if (focus_ != nullptr && focus_->acceptsText()) focus_->onImeCommit(text);
}

void Root::imeEnd() {
  if (focus_ != nullptr && focus_->acceptsText()) focus_->onImeEnd();
}

std::optional<Rect> Root::imeCaretRect() const { return focus_ != nullptr ? focus_->caretRect() : std::nullopt; }

bool Root::focusAcceptsText() const { return focus_ != nullptr && focus_->acceptsText(); }

CursorKind Root::cursor() const {
  for (const Widget* widget = capture_ != nullptr ? capture_ : hover_; widget != nullptr; widget = widget->parent()) {
    if (widget->cursor() != CursorKind::Arrow) return widget->cursor();
  }
  return CursorKind::Arrow;
}

}  // namespace rujian::ui
