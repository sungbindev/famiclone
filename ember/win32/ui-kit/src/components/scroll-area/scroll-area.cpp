/*
 * ------------------------------------------------------------------------
 * 名称：scroll-area.cpp
 * 说明：滚动区域控件的实现：滑块的长度与位置按 VS Code 的滚动条状态计算，颜色取自生成的令牌。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-19
 * 备注：滚动条只在显示（不透明度大于 0）时才接收鼠标；显示与隐藏的渐变用 16 毫秒一步的定时器推进。
 * ------------------------------------------------------------------------
 */

#include "rujian/ui/scroll-area.hpp"

#include <algorithm>
#include <cmath>

#include "base/theme-tokens.hpp"

namespace rujian::ui {

namespace {

constexpr int kVirtualKeyPageUp = 0x21;
constexpr int kVirtualKeyPageDown = 0x22;
constexpr int kVirtualKeyEnd = 0x23;
constexpr int kVirtualKeyHome = 0x24;
constexpr int kVirtualKeyUp = 0x26;
constexpr int kVirtualKeyDown = 0x28;
/** 轨道宽度、滑块粗细、滑块最短长度。 */
constexpr double kTrackWidth = 10.0;
constexpr double kSliderWidth = 6.0;
constexpr double kMinimumSlider = 20.0;
/** 滚轮一格滚动的距离（网页版实测：浏览器一格 100 对应 50 像素，各缩放比例下相同），方向键与翻页键的步长。 */
constexpr double kWheelStep = 50.0;
constexpr double kLineStep = 40.0;
/** 渐变：显示 100 毫秒、隐藏 800 毫秒，每步 16 毫秒；停止滚动或鼠标移出后 500 毫秒才开始隐藏。 */
constexpr unsigned kTickMilliseconds = 16;
constexpr double kShowMilliseconds = 100.0;
constexpr double kHideMilliseconds = 800.0;
constexpr unsigned kHideDelayMilliseconds = 500;

}  // namespace

void ScrollArea::setContent(std::unique_ptr<Widget> content) {
  if (content_ != nullptr) removeChild(content_);
  content_ = content ? addChild(std::move(content)) : nullptr;
  invalidate();
}

void ScrollArea::setMaxHeight(double maxHeight) {
  maxHeight_ = maxHeight;
  invalidate();
}

void ScrollArea::setFitContent(bool fit) {
  fitContent_ = fit;
  invalidate();
}

void ScrollArea::setHorizontalScroll(bool enabled) {
  horizontal_ = enabled;
  if (!enabled) scrollLeft_ = 0;
  invalidate();
}

double ScrollArea::maxScrollTop() const { return std::max(0.0, contentHeight_ - bounds().height); }

double ScrollArea::maxScrollLeft() const { return horizontal_ ? std::max(0.0, contentWidth_ - bounds().width) : 0.0; }

Rect ScrollArea::trackRect() const {
  const Rect box = bounds();
  return Rect{box.right() - kTrackWidth, box.y, kTrackWidth, box.height};
}

Rect ScrollArea::horizontalTrackRect() const {
  const Rect box = bounds();
  const double corner = scrollbarNeeded() ? kTrackWidth : 0.0;
  return Rect{box.x, box.bottom() - kTrackWidth, std::max(0.0, box.width - corner), kTrackWidth};
}

double ScrollArea::horizontalSliderLength() const {
  const double track = horizontalTrackRect().width;
  if (contentWidth_ <= 0) return track;
  return std::round(std::max(kMinimumSlider, std::floor(track * bounds().width / contentWidth_)));
}

double ScrollArea::horizontalSliderOffset() const {
  const double range = maxScrollLeft();
  if (range <= 0) return 0;
  return std::round(scrollLeft_ * (horizontalTrackRect().width - horizontalSliderLength()) / range);
}

Rect ScrollArea::horizontalSliderRect() const {
  if (!horizontalNeeded()) return Rect{};
  const Rect track = horizontalTrackRect();
  return Rect{track.x + horizontalSliderOffset(), track.y + (kTrackWidth - kSliderWidth) / 2.0, horizontalSliderLength(), kSliderWidth};
}

double ScrollArea::sliderLength() const {
  const double visible = bounds().height;
  if (contentHeight_ <= 0) return visible;
  return std::round(std::max(kMinimumSlider, std::floor(visible * visible / contentHeight_)));
}

double ScrollArea::sliderOffset() const {
  const double range = maxScrollTop();
  if (range <= 0) return 0;
  return std::round(scrollTop_ * (bounds().height - sliderLength()) / range);
}

Rect ScrollArea::sliderRect() const {
  if (!scrollbarNeeded()) return Rect{};
  const Rect track = trackRect();
  return Rect{track.x + (kTrackWidth - kSliderWidth) / 2.0, track.y + sliderOffset(), kSliderWidth, sliderLength()};
}

Size ScrollArea::measure(Size available) {
  if (content_ == nullptr) return Size{};
  const Size natural = content_->measure(Size{available.width, 0});
  const double height = maxHeight_ > 0 ? std::min(natural.height, maxHeight_) : natural.height;
  return Size{available.width > 0 ? available.width : natural.width, height};
}

void ScrollArea::arrange(const Rect& bounds) {
  setBounds(bounds);
  if (content_ == nullptr) return;
  contentWidth_ = horizontal_ ? std::max(bounds.width, content_->minimumSize().width) : bounds.width;
  contentHeight_ = fitContent_ ? bounds.height : content_->measure(Size{contentWidth_, 0}).height;
  scrollTop_ = std::clamp(scrollTop_, 0.0, maxScrollTop());
  scrollLeft_ = std::clamp(scrollLeft_, 0.0, maxScrollLeft());
  content_->arrange(Rect{bounds.x - scrollLeft_, bounds.y - scrollTop_, contentWidth_, contentHeight_});
  updateVisibility();
}

void ScrollArea::applyScroll() {
  if (content_ != nullptr) content_->arrange(Rect{bounds().x - scrollLeft_, bounds().y - scrollTop_, contentWidth_, contentHeight_});
  invalidate();
}

void ScrollArea::setScrollTop(double top) {
  // 浏览器的滚动位置取整到设备像素
  const double scale = host() != nullptr ? host()->scale() : 1.0;
  const double clamped = std::clamp(std::round(top * scale + 1e-6) / scale, 0.0, maxScrollTop());
  if (clamped == scrollTop_) return;
  scrollTop_ = clamped;
  applyScroll();
  // 滚动时显示滚动条，停下后一会儿再隐藏
  if (host() != nullptr) scheduleHide();
}

void ScrollArea::setScrollLeft(double left) {
  const double scale = host() != nullptr ? host()->scale() : 1.0;
  const double clamped = std::clamp(std::round(left * scale + 1e-6) / scale, 0.0, maxScrollLeft());
  if (clamped == scrollLeft_) return;
  scrollLeft_ = clamped;
  applyScroll();
  if (host() != nullptr) scheduleHide();
}

void ScrollArea::scrollIntoView(const Rect& area) {
  const Rect box = bounds();
  if (area.y < box.y) setScrollTop(scrollTop_ - (box.y - area.y));
  else if (area.bottom() > box.bottom()) setScrollTop(scrollTop_ + std::min(area.bottom() - box.bottom(), area.y - box.y));
  // 比可见区域还宽的区域（整行）不调整横向位置
  if (!horizontal_ || area.width > box.width) return;
  if (area.x < box.x) setScrollLeft(scrollLeft_ - (box.x - area.x));
  else if (area.right() > box.right()) setScrollLeft(scrollLeft_ + (area.right() - box.right()));
}

void ScrollArea::paintChildren(Canvas& canvas) {
  canvas.pushClip(bounds());
  Widget::paintChildren(canvas);
  canvas.popClip();
  if (opacity_ <= 0 || (!scrollbarNeeded() && !horizontalNeeded())) return;
  const auto& colors = themeTokens(host()->theme());
  if (scrollbarNeeded()) {
    Color color = colors.scrollbar_slider_bg;
    if (dragging_) color = colors.scrollbar_slider_active_bg;
    else if (overSlider_) color = colors.scrollbar_slider_hover_bg;
    canvas.drawBox(sliderRect(), BoxStyle{.fill = color.withOpacity(opacity_), .radius = kSliderWidth / 2.0});
  }
  if (horizontalNeeded()) {
    Color color = colors.scrollbar_slider_bg;
    if (draggingHorizontal_) color = colors.scrollbar_slider_active_bg;
    else if (overHorizontalSlider_) color = colors.scrollbar_slider_hover_bg;
    canvas.drawBox(horizontalSliderRect(), BoxStyle{.fill = color.withOpacity(opacity_), .radius = kSliderWidth / 2.0});
  }
}

Widget* ScrollArea::hitTest(Point point) {
  // 滚动条显示着才响应鼠标（不显示时 pointer-events: none，鼠标穿过去点到内容）
  if (visible() && opacity_ > 0) {
    if (scrollbarNeeded() && trackRect().contains(point)) return this;
    if (horizontalNeeded() && horizontalTrackRect().contains(point)) return this;
  }
  return Widget::hitTest(point);
}

bool ScrollArea::onMouseDown(const MouseEvent& event) {
  if (event.button != MouseButton::Left || opacity_ <= 0) return false;
  if (horizontalNeeded() && horizontalTrackRect().contains(event.position)) {
    const Rect slider = horizontalSliderRect();
    if (slider.contains(event.position)) {
      dragGrabHorizontal_ = event.position.x - slider.x;
    } else {
      // 点在轨道上：滑块中心先跳到指针处，再接着拖
      dragGrabHorizontal_ = slider.width / 2.0;
      const double range = horizontalTrackRect().width - slider.width;
      if (range > 0) setScrollLeft(std::round((event.position.x - bounds().x - dragGrabHorizontal_) * maxScrollLeft() / range));
    }
    draggingHorizontal_ = true;
    host()->captureMouse(this);
    updateVisibility();
    return true;
  }
  if (!scrollbarNeeded() || !trackRect().contains(event.position)) return false;
  const Rect slider = sliderRect();
  if (slider.contains(event.position)) {
    dragGrab_ = event.position.y - slider.y;
  } else {
    // 点在轨道上：滑块中心先跳到指针处，再接着拖
    dragGrab_ = slider.height / 2.0;
    const double range = bounds().height - slider.height;
    if (range > 0) setScrollTop(std::round((event.position.y - bounds().y - dragGrab_) * maxScrollTop() / range));
  }
  dragging_ = true;
  host()->captureMouse(this);
  updateVisibility();
  return true;
}

bool ScrollArea::onMouseMove(const MouseEvent& event) {
  if (draggingHorizontal_) {
    const double range = horizontalTrackRect().width - horizontalSliderLength();
    if (range > 0) setScrollLeft(std::round((event.position.x - bounds().x - dragGrabHorizontal_) * maxScrollLeft() / range));
    return true;
  }
  if (dragging_) {
    const double range = bounds().height - sliderLength();
    if (range > 0) setScrollTop(std::round((event.position.y - bounds().y - dragGrab_) * maxScrollTop() / range));
    return true;
  }
  const bool over = scrollbarNeeded() && sliderRect().contains(event.position);
  const bool overHorizontal = horizontalNeeded() && horizontalSliderRect().contains(event.position);
  if (over != overSlider_ || overHorizontal != overHorizontalSlider_) {
    overSlider_ = over;
    overHorizontalSlider_ = overHorizontal;
    invalidate();
  }
  return false;
}

bool ScrollArea::onMouseUp(const MouseEvent& event) {
  if (event.button != MouseButton::Left || (!dragging_ && !draggingHorizontal_)) return false;
  dragging_ = false;
  draggingHorizontal_ = false;
  host()->releaseMouse(this);
  scheduleHide();
  return true;
}

bool ScrollArea::onMouseWheel(const MouseEvent& event) {
  // Shift+滚轮横向滚动；没有纵向滚动而有横向滚动时，滚轮也横向滚动
  const bool toHorizontal = horizontalNeeded() && ((event.modifiers & kModifierShift) != 0 || !scrollbarNeeded());
  if (toHorizontal) {
    const double before = scrollLeft_;
    setScrollLeft(scrollLeft_ - event.wheelDelta * kWheelStep);
    return scrollLeft_ != before;
  }
  if (!scrollbarNeeded()) return false;
  const double before = scrollTop_;
  setScrollTop(scrollTop_ - event.wheelDelta * kWheelStep);
  return scrollTop_ != before;
}

bool ScrollArea::onKeyDown(const KeyEvent& event) {
  // 输入框等自己处理按键的控件有焦点时不滚动
  const Widget* focused = host() != nullptr ? host()->focusedWidget() : nullptr;
  if (focused != nullptr && focused->acceptsText()) return false;
  if ((event.modifiers & (kModifierCtrl | kModifierAlt)) != 0) return false;
  const double page = std::max(bounds().height - kLineStep, kLineStep);
  const double before = scrollTop_;
  switch (event.virtualKey) {
    case kVirtualKeyDown:
      setScrollTop(scrollTop_ + kLineStep);
      break;
    case kVirtualKeyUp:
      setScrollTop(scrollTop_ - kLineStep);
      break;
    case kVirtualKeyPageDown:
      setScrollTop(scrollTop_ + page);
      break;
    case kVirtualKeyPageUp:
      setScrollTop(scrollTop_ - page);
      break;
    case kVirtualKeyHome:
      setScrollTop(0);
      break;
    case kVirtualKeyEnd:
      setScrollTop(maxScrollTop());
      break;
    default:
      return false;
  }
  return scrollTop_ != before;
}

void ScrollArea::onDescendantFocused(Widget* descendant) {
  if (descendant != nullptr) scrollIntoView(descendant->focusRect());
}

void ScrollArea::onStateChanged() {
  if (!hovered()) {
    overSlider_ = false;
    overHorizontalSlider_ = false;
    // 鼠标移出后过一会儿再隐藏
    if (host() != nullptr && opacity_ > 0) scheduleHide();
  }
  updateVisibility();
  Widget::onStateChanged();
}

void ScrollArea::scheduleHide() {
  recentlyScrolled_ = true;
  if (host() == nullptr) return;
  if (hideTimer_ != 0) host()->stopTimer(hideTimer_);
  hideTimer_ = 0;
  // 不播放动画时没有“过一会儿”：直接按当前的悬停与拖动状态决定
  if (!host()->animationsEnabled()) {
    recentlyScrolled_ = false;
  } else {
    hideTimer_ = host()->startTimer(this, kHideDelayMilliseconds);
  }
  updateVisibility();
}

void ScrollArea::updateVisibility() {
  if (host() == nullptr) return;
  setTarget((scrollbarNeeded() || horizontalNeeded()) && (hovered() || dragging_ || draggingHorizontal_ || recentlyScrolled_));
}

void ScrollArea::setTarget(bool visible) {
  targetVisible_ = visible;
  if (!host()->animationsEnabled()) {
    const double target = visible ? 1.0 : 0.0;
    if (opacity_ != target) {
      opacity_ = target;
      invalidate();
    }
    return;
  }
  if (opacity_ != (visible ? 1.0 : 0.0) && animationTimer_ == 0) animationTimer_ = host()->startTimer(this, kTickMilliseconds);
}

void ScrollArea::tickAnimation() {
  const double step = kTickMilliseconds / (targetVisible_ ? kShowMilliseconds : kHideMilliseconds);
  opacity_ = targetVisible_ ? std::min(1.0, opacity_ + step) : std::max(0.0, opacity_ - step);
  invalidate();
  if (opacity_ != (targetVisible_ ? 1.0 : 0.0)) animationTimer_ = host()->startTimer(this, kTickMilliseconds);
}

void ScrollArea::onTimer(unsigned id) {
  if (id == hideTimer_) {
    hideTimer_ = 0;
    recentlyScrolled_ = false;
    updateVisibility();
  } else if (id == animationTimer_) {
    animationTimer_ = 0;
    tickAnimation();
  }
}

}  // namespace rujian::ui
