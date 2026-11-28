/*
 * ------------------------------------------------------------------------
 * 名称：hint-bubble.cpp
 * 说明：短暂提示气泡的实现：排版、定位、绘制与自动收起。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-25
 * 备注：无。
 * ------------------------------------------------------------------------
 */

#include "rujian/ui/hint-bubble.hpp"

#include <algorithm>

#include "base/theme-tokens.hpp"
#include "rujian/ui/geometry.hpp"

namespace rujian::ui {

namespace {

/** 与锚点区域的间距、与窗口边缘的最小距离。 */
constexpr double kGap = 4.0;
constexpr double kViewportMargin = 8.0;

TextStyle bubbleStyle(Color color, bool wrap) {
  TextStyle style;
  style.fontSize = tokens::font_size_body;
  style.fontWeight = 400;
  style.lineHeight = tokens::line_height_body;
  style.color = color;
  style.wrap = wrap;
  return style;
}

}  // namespace

HintBubble::HintBubble(Host& host) : owner_(host) {}

HintBubble::~HintBubble() {
  // 宿主先销毁时它会让弹层脱离（host() 为空），这时不能再访问宿主
  if (host() != nullptr) hide();
}

double HintBubble::borderWidth() const { return snapBorderWidth(tokens::space_hairline, host() != nullptr ? host()->scale() : owner_.scale()); }

double HintBubble::textWidth() const {
  const double border = borderWidth();
  const double innerMax = tokens::tooltip_max_width - 2 * tokens::tooltip_padding_x - 2 * border;
  return std::min(owner_.measureText(text_, bubbleStyle(kTransparent, false), 0).width, innerMax);
}

void HintBubble::show(const Rect& anchor, const std::string& text, unsigned milliseconds) {
  text_ = text;
  const double border = borderWidth();
  const double inner = textWidth();
  const double height = owner_.measureText(text_, bubbleStyle(kTransparent, true), inner + 0.5).height;
  const Size box{inner + 2 * tokens::tooltip_padding_x + 2 * border, height + 2 * tokens::tooltip_padding_y + 2 * border};
  if (!shown_) {
    owner_.showLayer(this, [this](Widget*) { hide(); }, true);
    shown_ = true;
  }
  if (timer_ != 0) owner_.stopTimer(timer_);
  timer_ = owner_.startTimer(this, milliseconds);
  const Rect viewport = owner_.layerViewport();
  const bool fitsBelow = anchor.bottom() + kGap + box.height + kViewportMargin <= viewport.bottom();
  const double top = fitsBelow ? anchor.bottom() + kGap : std::max(viewport.y + kViewportMargin, anchor.y - kGap - box.height);
  const double left = std::max(viewport.x + kViewportMargin, std::min(anchor.x, viewport.right() - kViewportMargin - box.width));
  arrange(Rect{left, top, box.width, box.height});
  invalidate();
}

void HintBubble::hide() {
  if (!shown_) return;
  shown_ = false;
  if (timer_ != 0) owner_.stopTimer(timer_);
  timer_ = 0;
  owner_.hideLayer(this);
}

void HintBubble::onTimer(unsigned /*id*/) {
  timer_ = 0;
  hide();
}

void HintBubble::layerRects(std::vector<Rect>& out) const {
  if (shown_) out.push_back(bounds());
}

void HintBubble::paint(Canvas& canvas) {
  const auto& colors = themeTokens(host()->theme());
  const Rect box = bounds();
  const double border = borderWidth();
  canvas.drawShadow(box, tokens::radius_md, colors.layer_shadow);
  canvas.drawBox(box, BoxStyle{.fill = colors.tooltip_background, .radius = tokens::radius_md, .borderWidth = tokens::space_hairline, .borderColor = colors.layer_border});
  canvas.drawText(text_, Rect{box.x + border + tokens::tooltip_padding_x, box.y + border + tokens::tooltip_padding_y, textWidth() + 0.5, box.height}, bubbleStyle(colors.text, true), VerticalAlign::Top);
}

}  // namespace rujian::ui
