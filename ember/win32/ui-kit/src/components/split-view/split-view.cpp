/*
 * ------------------------------------------------------------------------
 * 名称：split-view.cpp
 * 说明：分栏视图的实现：左栏（右边带 1 像素分割线）、跨在分割线两侧的 4 像素分隔条、主区（左内边距 10）；分隔条负责拖动、方向键与悬停显示。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-19
 * 备注：分隔条的悬停显示在动画开启时要停留 200 毫秒（计时器），关闭动画（画廊）时立即显示；拖动时立即显示。
 *       方向键每次 10 像素，Home 与 End 取最小与最大宽度；键盘焦点不画任何东西。
 * ------------------------------------------------------------------------
 */

#include "rujian/ui/split-view.hpp"

#include <algorithm>
#include <cmath>

#include "base/theme-tokens.hpp"
#include "rujian/ui/geometry.hpp"
#include "rujian/ui/scroll-area.hpp"

namespace rujian::ui {

namespace {

constexpr double kBorderWidth = 1.0;
constexpr double kKeyStep = 10.0;
constexpr unsigned kHoverDelayMilliseconds = 200;
constexpr int kVirtualKeyEnd = 0x23;
constexpr int kVirtualKeyHome = 0x24;
constexpr int kVirtualKeyLeft = 0x25;
constexpr int kVirtualKeyRight = 0x27;

}  // namespace

/** 分隔条：只处理鼠标拖动、键盘与悬停显示，宽度的取值与摆放交给分栏视图。 */
class SplitterHandle final : public Widget {
 public:
  explicit SplitterHandle(SplitView& owner) : owner_(owner) {}

  [[nodiscard]] bool focusable() const override { return true; }
  [[nodiscard]] CursorKind cursor() const override { return CursorKind::SizeWE; }
  /** 分隔条是否显示着聚焦色。 */
  [[nodiscard]] bool shown() const { return shown_; }

  bool onMouseDown(const MouseEvent& event) override {
    if (event.button != MouseButton::Left) return false;
    dragging_ = true;
    grabX_ = event.position.x;
    startWidth_ = owner_.sideWidth();
    host()->captureMouse(this);
    refresh();
    return true;
  }

  bool onMouseMove(const MouseEvent& event) override {
    if (!dragging_) return false;
    owner_.changeSideWidth(startWidth_ + (event.position.x - grabX_));
    return true;
  }

  bool onMouseUp(const MouseEvent& event) override {
    if (event.button != MouseButton::Left || !dragging_) return false;
    dragging_ = false;
    host()->releaseMouse(this);
    refresh();
    return true;
  }

  bool onKeyDown(const KeyEvent& event) override {
    double target = owner_.sideWidth();
    switch (event.virtualKey) {
      case kVirtualKeyLeft: target -= kKeyStep; break;
      case kVirtualKeyRight: target += kKeyStep; break;
      case kVirtualKeyHome: target = owner_.minimum_; break;
      case kVirtualKeyEnd: target = owner_.maximum_; break;
      default: return false;
    }
    owner_.changeSideWidth(target);
    return true;
  }

  void onStateChanged() override {
    Widget::onStateChanged();
    refresh();
  }

  void onTimer(unsigned id) override {
    if (id != timer_) return;
    timer_ = 0;
    shown_ = true;
    invalidate();
  }

  void paint(Canvas& canvas) override {
    if (shown_) canvas.fillRect(bounds(), themeTokens(host()->theme()).focus_border);
  }

 private:
  /** 按悬停与拖动状态更新显示：悬停要停留一会儿才显示，移开立即隐藏，拖动立即显示。 */
  void refresh() {
    if (host() == nullptr) return;
    const bool wanted = dragging_ || hovered();
    if (!wanted) {
      if (timer_ != 0) host()->stopTimer(timer_);
      timer_ = 0;
      shown_ = false;
    } else if (!shown_) {
      if (dragging_ || !host()->animationsEnabled()) {
        if (timer_ != 0) host()->stopTimer(timer_);
        timer_ = 0;
        shown_ = true;
      } else if (timer_ == 0) {
        timer_ = host()->startTimer(this, kHoverDelayMilliseconds);
      }
    }
    invalidate();
  }

  SplitView& owner_;
  bool dragging_{false};
  bool shown_{false};
  double grabX_{0};
  double startWidth_{0};
  unsigned timer_{0};
};

SplitView::SplitView() {
  sideScroll_ = static_cast<ScrollArea*>(addChild(std::make_unique<ScrollArea>()));
  splitter_ = static_cast<SplitterHandle*>(addChild(std::make_unique<SplitterHandle>(*this)));
}

void SplitView::setSide(std::unique_ptr<Widget> side) {
  sideScroll_->setContent(std::move(side));
  invalidate();
}

void SplitView::setMain(std::unique_ptr<Widget> main) {
  if (main_ != nullptr) removeChild(main_);
  // 分隔条要始终在最上面，主区加进去后再把它放回末尾
  std::unique_ptr<Widget> splitter = removeChild(splitter_);
  main_ = main ? addChild(std::move(main)) : nullptr;
  addChild(std::move(splitter));
  invalidate();
}

double SplitView::clampWidth(double width) const { return std::clamp(std::round(width), minimum_, maximum_); }

Widget* SplitView::splitter() const { return splitter_; }

void SplitView::setSideWidth(double width) { changeSideWidth(width); }

void SplitView::setSideWidthRange(double minimum, double maximum) {
  minimum_ = minimum;
  maximum_ = maximum;
  changeSideWidth(sideWidth_);
}

void SplitView::changeSideWidth(double width) {
  sideWidth_ = clampWidth(width);
  if (host() == nullptr) return;
  arrange(bounds());
  invalidate();
}

Size SplitView::measure(Size available) {
  const double border = host() != nullptr ? snapBorderWidth(kBorderWidth, host()->scale()) : kBorderWidth;
  const double mainWidth = available.width > 0 ? std::max(0.0, available.width - sideWidth_ - tokens::space_page) : 0.0;
  const Size side = sideScroll_->measure(Size{sideWidth_ - border, 0});
  const Size main = main_ != nullptr ? main_->measure(Size{mainWidth, 0}) : Size{};
  const double width = available.width > 0 ? available.width : sideWidth_ + tokens::space_page + main.width;
  return Size{width, std::max(side.height, main.height)};
}

void SplitView::arrange(const Rect& bounds) {
  Widget::arrange(bounds);
  const double border = snapBorderWidth(kBorderWidth, host()->scale());
  sideScroll_->arrange(Rect{bounds.x, bounds.y, sideWidth_ - border, bounds.height});
  // 分隔条宽 4，两侧各 -2 的外边距，所以中心正好在左栏的右边缘
  splitter_->arrange(Rect{bounds.x + sideWidth_ - tokens::space_xs / 2.0, bounds.y, tokens::space_xs, bounds.height});
  if (main_ != nullptr) {
    const double left = bounds.x + sideWidth_ + tokens::space_page;
    main_->arrange(Rect{left, bounds.y, std::max(0.0, bounds.right() - left), bounds.height});
  }
}

void SplitView::paint(Canvas& canvas) {
  const double border = snapBorderWidth(kBorderWidth, host()->scale());
  canvas.fillRect(Rect{bounds().x + sideWidth_ - border, bounds().y, border, bounds().height}, themeTokens(host()->theme()).divider);
}

}  // namespace rujian::ui
