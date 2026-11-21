/*
 * ------------------------------------------------------------------------
 * 名称：timeline-popup.cpp
 * 说明：时间轴弹层的实现：外框与阴影取菜单弹层的令牌，轨道取输入框的底色与边框，包络用强调色（播放头之前的部分更深），空白段用弱化文字色的浅底，提示用悬停提示的令牌。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-21
 * 备注：弹层宽度最大 640 逻辑像素，窗口更窄时让出两边的边距；位置在锚点上方，放不下时翻到下方，水平方向与锚点左对齐并限制在窗口内。
 * ------------------------------------------------------------------------
 */

#include "rujian/ui/timeline-popup.hpp"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <iterator>

#include "base/theme-tokens.hpp"

namespace rujian::ui {

namespace {

constexpr int kVirtualKeyEscape = 0x1B;

constexpr double kMaxWidth = 640.0;
constexpr double kViewportMargin = 8.0;
constexpr double kPopupGap = 2.0;
constexpr double kBorderWidth = 1.0;
/** 轨道（含边框）的高度，轨道与刻度之间的间距，刻度线的长度。 */
constexpr double kPlotHeight = 56.0;
constexpr double kAxisGap = 4.0;
constexpr double kTickHeight = 4.0;
/** 相邻两个刻度之间至少隔开的距离，刻度文字的最大宽度。 */
constexpr double kMinTickSpacing = 64.0;
constexpr double kTickLabelWidth = 56.0;
/** 包络上下留出的空白。 */
constexpr double kEnvelopeMargin = 3.0;
/** 播放头的宽度，空白段底色与未播放部分包络的不透明度。 */
constexpr double kPlayheadWidth = 2.0;
constexpr double kBlankOpacity = 0.2;
constexpr double kUnplayedOpacity = 0.5;
/** 刻度可选的间隔（秒）。 */
constexpr double kTickSteps[] = {1, 2, 5, 10, 15, 30, 60, 120, 300, 600, 900, 1800, 3600, 7200};

TextStyle axisStyle(Color color) {
  TextStyle style;
  style.fontSize = tokens::font_size_caption;
  style.color = color;
  style.align = TextAlign::Center;
  return style;
}

}  // namespace

TimelinePopup::TimelinePopup() = default;

TimelinePopup::~TimelinePopup() {
  // 没有通知地关掉：调用方可能正在销毁
  if (open_ && host() != nullptr) host()->hideLayer(this);
}

void TimelinePopup::setDuration(double seconds) {
  duration_ = std::max(0.0, seconds);
  invalidate();
}

void TimelinePopup::setEnvelope(std::vector<std::uint8_t> peaks, double step, std::vector<TimelineRange> blanks) {
  peaks_ = std::move(peaks);
  step_ = step;
  blanks_ = std::move(blanks);
  invalidate();
}

void TimelinePopup::clearEnvelope() {
  peaks_.clear();
  step_ = 0;
  blanks_.clear();
  invalidate();
}

void TimelinePopup::setNotice(std::string text) {
  if (notice_ == text) return;
  notice_ = std::move(text);
  invalidate();
}

void TimelinePopup::setPosition(double seconds) {
  const double next = std::max(0.0, seconds);
  if (next == position_) return;
  position_ = next;
  invalidate();
}

std::string TimelinePopup::formatTime(double seconds) {
  const auto total = static_cast<long long>(std::floor(std::max(0.0, seconds) + 1e-6));
  const long long hours = total / 3600;
  const long long minutes = total / 60 % 60;
  const long long rest = total % 60;
  char text[32];
  if (hours > 0) std::snprintf(text, sizeof(text), "%lld:%02lld:%02lld", hours, minutes, rest);
  else std::snprintf(text, sizeof(text), "%lld:%02lld", minutes, rest);
  return text;
}

// ---------------------------------------------------------------------------
// 打开与关闭
// ---------------------------------------------------------------------------

Size TimelinePopup::measure(Size /*available*/) {
  if (host() == nullptr) return Size{kMaxWidth, 0};
  const double border = snapBorderWidth(kBorderWidth, host()->scale());
  const double axis = host()->measureText("0:00", axisStyle(kTransparent), 0).height;
  return Size{kMaxWidth, 2 * (tokens::space_page + border) + kPlotHeight + kAxisGap + axis};
}

void TimelinePopup::place(const Rect& anchor) {
  const Size viewport = host()->viewportSize();
  const Size size = measure(Size{});
  const double width = std::min(size.width, std::max(viewport.width - 2 * kViewportMargin, 0.0));
  const double below = viewport.height - anchor.bottom() - kViewportMargin;
  const double above = anchor.y - kViewportMargin;
  const bool placeAbove = above >= size.height || above >= below;
  const double top = placeAbove ? anchor.y - kPopupGap - size.height : anchor.bottom() + kPopupGap;
  const double left = std::max(std::min(anchor.x, viewport.width - kViewportMargin - width), kViewportMargin);
  arrange(Rect{left, top, width, size.height});
}

void TimelinePopup::open(Widget& owner, const Rect& anchor) {
  Host* shared = owner.host();
  if (open_ || shared == nullptr) return;
  owner_ = &owner;
  shared->showLayer(this, [this](Widget* hit) {
    // 在 owner 里按下（窗口大小变化时 hit 为空）：只有窗口大小变化才关闭
    for (Widget* widget = hit; widget != nullptr; widget = widget->parent()) {
      if (widget == owner_) return;
    }
    dismiss();
  });
  open_ = true;
  place(anchor);
  shared->setFocus(this, false);
  invalidate();
}

void TimelinePopup::close() {
  if (!open_) return;
  open_ = false;
  if (dragging_) {
    dragging_ = false;
    if (host() != nullptr) host()->releaseMouse(this);
  }
  hoverActive_ = false;
  if (host() != nullptr) host()->hideLayer(this);
}

void TimelinePopup::dismiss() {
  close();
  if (onClosed_) onClosed_();
}

// ---------------------------------------------------------------------------
// 坐标
// ---------------------------------------------------------------------------

Rect TimelinePopup::plotRect() const {
  const double border = host() != nullptr ? snapBorderWidth(kBorderWidth, host()->scale()) : kBorderWidth;
  const double inset = tokens::space_page + border;
  return Rect{bounds().x + inset, bounds().y + inset, std::max(0.0, bounds().width - 2 * inset), kPlotHeight};
}

double TimelinePopup::timeAt(double x) const {
  const Rect plot = plotRect();
  if (plot.width <= 0 || duration_ <= 0) return 0;
  return std::clamp((x - plot.x) / plot.width, 0.0, 1.0) * duration_;
}

double TimelinePopup::xAt(double seconds) const {
  const Rect plot = plotRect();
  if (duration_ <= 0) return plot.x;
  return plot.x + std::clamp(seconds / duration_, 0.0, 1.0) * plot.width;
}

// ---------------------------------------------------------------------------
// 绘制
// ---------------------------------------------------------------------------

void TimelinePopup::paint(Canvas& canvas) {
  const auto& colors = themeTokens(host()->theme());
  canvas.drawShadow(bounds(), tokens::menu_popup_radius, colors.menu_shadow);
  canvas.drawBox(bounds(), BoxStyle{.fill = colors.menu_background, .radius = tokens::menu_popup_radius, .borderWidth = kBorderWidth, .borderColor = colors.menu_border});

  const Rect plot = plotRect();
  canvas.drawBox(plot, BoxStyle{.fill = colors.input_bg, .radius = tokens::input_radius, .borderWidth = kBorderWidth, .borderColor = colors.input_border});
  const double border = snapBorderWidth(kBorderWidth, canvas.scale());
  const Rect inner = plot.inset(Insets::all(border));
  canvas.pushClip(inner, std::max(0.0, tokens::input_radius - border));
  if (notice_.empty()) {
    paintEnvelope(canvas, inner);
  } else {
    TextStyle style = axisStyle(colors.text_muted);
    style.fontSize = tokens::font_size_body;
    canvas.drawText(notice_, inner, style, VerticalAlign::Center);
  }
  if (duration_ > 0) {
    const double x = xAt(position_);
    canvas.fillRect(Rect{x - kPlayheadWidth / 2, inner.y, kPlayheadWidth, inner.height}, colors.text);
    if (dragging_ || hoverActive_) paintMarker(canvas, inner, markerX_);
  }
  canvas.popClip();
  paintAxis(canvas);
}

void TimelinePopup::paintEnvelope(Canvas& canvas, const Rect& inner) const {
  const auto& colors = themeTokens(host()->theme());
  if (duration_ <= 0 || inner.width <= 0) return;
  for (const TimelineRange& blank : blanks_) {
    const double from = xAt(blank.start);
    const double to = xAt(blank.end);
    canvas.fillRect(Rect{from, inner.y, std::max(1.0, to - from), inner.height}, colors.text_muted.withOpacity(kBlankOpacity));
  }
  if (peaks_.empty() || step_ <= 0) return;
  const double perColumn = duration_ / inner.width;
  const double middle = inner.y + inner.height / 2;
  const double half = std::max(1.0, inner.height / 2 - kEnvelopeMargin);
  const double playedX = xAt(position_);
  const auto columns = static_cast<int>(std::ceil(inner.width));
  for (int column = 0; column < columns; ++column) {
    const auto first = std::min(peaks_.size(), static_cast<std::size_t>(std::floor(column * perColumn / step_)));
    const auto last = std::min(peaks_.size(), std::max(first + 1, static_cast<std::size_t>(std::ceil((column + 1) * perColumn / step_))));
    int peak = 0;
    for (std::size_t index = first; index < last; ++index) peak = std::max<int>(peak, peaks_[index]);
    if (peak == 0) continue;
    const double height = std::max(1.0, half * peak / 100.0);
    const double x = inner.x + column;
    canvas.fillRect(Rect{x, middle - height, 1.0, 2 * height}, x + 0.5 < playedX ? colors.accent : colors.accent.withOpacity(kUnplayedOpacity));
  }
}

void TimelinePopup::paintAxis(Canvas& canvas) const {
  if (duration_ <= 0) return;
  const auto& colors = themeTokens(host()->theme());
  const Rect plot = plotRect();
  if (plot.width <= 0) return;
  double step = kTickSteps[std::size(kTickSteps) - 1];
  for (const double candidate : kTickSteps) {
    if (plot.width * candidate / duration_ >= kMinTickSpacing) {
      step = candidate;
      break;
    }
  }
  const double textHeight = host()->measureText("0:00", axisStyle(kTransparent), 0).height;
  for (double time = 0; time <= duration_ + 1e-9; time += step) {
    const double x = xAt(time);
    canvas.fillRect(Rect{x - 0.5, plot.bottom(), 1.0, kTickHeight}, colors.text_muted);
    // 文字以刻度为中心，超出弹层的部分向内收
    const double left = std::clamp(x - kTickLabelWidth / 2, bounds().x + kBorderWidth, std::max(bounds().x + kBorderWidth, bounds().right() - kBorderWidth - kTickLabelWidth));
    canvas.drawText(formatTime(time), Rect{left, plot.bottom() + kAxisGap, kTickLabelWidth, textHeight}, axisStyle(colors.text_muted), VerticalAlign::Top);
  }
}

void TimelinePopup::paintMarker(Canvas& canvas, const Rect& inner, double x) const {
  const auto& colors = themeTokens(host()->theme());
  const double lineX = std::clamp(x, inner.x, inner.right());
  canvas.fillRect(Rect{lineX - 0.5, inner.y, 1.0, inner.height}, colors.text_muted);
  TextStyle style;
  style.fontSize = tokens::font_size_caption;
  style.color = colors.tooltip_text_color;
  const std::string text = formatTime(timeAt(lineX));
  const TextMetrics metrics = host()->measureText(text, style, 0);
  const double width = metrics.width + 2 * tokens::tooltip_padding_x;
  const double height = metrics.height + 2 * tokens::tooltip_padding_y;
  const double left = lineX + 6 + width <= inner.right() ? lineX + 6 : lineX - 6 - width;
  const Rect box{std::max(inner.x, left), inner.y + 4, width, height};
  canvas.drawBox(box, BoxStyle{.fill = colors.tooltip_background, .radius = tokens::input_radius});
  canvas.drawText(text, box.inset(Insets::symmetric(tokens::tooltip_padding_y, tokens::tooltip_padding_x)), style, VerticalAlign::Center);
}

// ---------------------------------------------------------------------------
// 输入
// ---------------------------------------------------------------------------

bool TimelinePopup::onMouseDown(const MouseEvent& event) {
  if (event.button != MouseButton::Left) return false;
  if (duration_ > 0 && plotRect().contains(event.position)) {
    dragging_ = true;
    markerX_ = event.position.x;
    host()->captureMouse(this);
    invalidate();
  }
  return true;
}

bool TimelinePopup::onMouseMove(const MouseEvent& event) {
  const bool over = plotRect().contains(event.position);
  if (over != hoverActive_ || ((over || dragging_) && event.position.x != markerX_)) {
    hoverActive_ = over;
    if (over || dragging_) markerX_ = event.position.x;
    invalidate();
  }
  return true;
}

bool TimelinePopup::onMouseUp(const MouseEvent& event) {
  if (event.button != MouseButton::Left || !dragging_) return false;
  dragging_ = false;
  host()->releaseMouse(this);
  const double target = timeAt(event.position.x);
  // 先把播放头挪到新位置，调用方给出真实位置之前不跳回去
  position_ = target;
  invalidate();
  if (onSeek_) onSeek_(target);
  return true;
}

bool TimelinePopup::onKeyDown(const KeyEvent& event) {
  if (event.virtualKey != kVirtualKeyEscape) return false;
  dismiss();
  return true;
}

void TimelinePopup::onStateChanged() {
  // 鼠标移出弹层后去掉悬停线；拖动时鼠标被捕获，不受影响
  if (!hovered() && !dragging_) hoverActive_ = false;
  Widget::onStateChanged();
}

}  // namespace rujian::ui
