/*
 * ------------------------------------------------------------------------
 * 名称：tooltip.cpp
 * 说明：悬停提示控件的实现：纯文字与版式提示的排版、定位、角标与绘制。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-19
 * 备注：纯文字提示：内边距 4×8，最宽 320（含边框），文字超过最大宽度时换行并取最大宽度；
 *       版式提示：内边距 8×10，最宽 380，标题行（图标 16、标题 13 号半粗、下方 8 的间距与分隔线）与分条说明（图标 14 在左，小标题与正文在右）纵向排列、间距 8；
 *       提示与锚点的间距为 10，与窗口边缘至少 8，角标是 8×8 旋转 45 度的方块露在外面的那一半。
 * ------------------------------------------------------------------------
 */

#include <algorithm>
#include <cmath>

#include "base/theme-tokens.hpp"
#include "rujian/ui/geometry.hpp"
#include "rujian/ui/tooltip.hpp"

namespace rujian::ui {

namespace {

/** 与锚点的间距（含角标伸出的部分）与窗口边缘的最小距离。 */
constexpr double kGap = 10.0;
constexpr double kViewportMargin = 8.0;
/** 角标中心离提示上下边缘的最小距离，避开圆角。 */
constexpr double kArrowEdgeMin = 10.0;
/** 元素宽高都不超过它时提示对齐元素；更大的元素对齐鼠标位置。 */
constexpr double kCompactSize = 80.0;
/** 对齐鼠标位置时，提示避开光标自身的宽高。 */
constexpr double kCursorWidth = 12.0;
constexpr double kCursorHeight = 18.0;
/** 条目左侧图标与右侧内容的间距；条目图标的上边距；小标题与正文的间距。 */
constexpr double kColumnGap = 8.0;
constexpr double kItemIconTop = 2.0;
constexpr double kBodyGap = 2.0;

TextStyle textStyle(double size, int weight, Color color, bool wrap) {
  TextStyle style;
  style.fontSize = size;
  style.fontWeight = weight;
  style.lineHeight = tokens::line_height_body;
  style.color = color;
  style.wrap = wrap;
  return style;
}

}  // namespace

Tooltip::Tooltip(Root& root) : root_(root) { root_.setTooltipPresenter(this); }

Tooltip::~Tooltip() {
  hideTooltip();
  root_.setTooltipPresenter(nullptr);
}

double Tooltip::borderWidth() const { return snapBorderWidth(tokens::space_hairline, host() != nullptr ? host()->scale() : 1.0); }

Tooltip::Metrics Tooltip::compute() const {
  Metrics m;
  const double border = borderWidth();
  Host& host = *this->host();
  if (!content_.rich) {
    const double innerMax = tokens::tooltip_max_width - 2 * tokens::tooltip_padding_x - 2 * border;
    const double natural = host.measureText(content_.text, textStyle(tokens::font_size_body, 400, kTransparent, false), 0).width;
    m.innerWidth = std::min(natural, innerMax);
    const double height = host.measureText(content_.text, textStyle(tokens::font_size_body, 400, kTransparent, true), m.innerWidth + 0.5).height;
    m.box = Size{m.innerWidth + 2 * tokens::tooltip_padding_x + 2 * border, height + 2 * tokens::tooltip_padding_y + 2 * border};
    return m;
  }
  const double innerMax = tokens::tooltip_rich_max_width - 2 * tokens::tooltip_rich_padding_x - 2 * border;
  const TextStyle titleNoWrap = textStyle(tokens::tooltip_title_font_size, static_cast<int>(tokens::tooltip_title_font_weight), kTransparent, false);
  const TextStyle itemTitleNoWrap = textStyle(tokens::font_size_body, static_cast<int>(tokens::tooltip_item_title_font_weight), kTransparent, false);
  const TextStyle bodyNoWrap = textStyle(tokens::font_size_body, 400, kTransparent, false);
  double natural = 0.0;
  if (!content_.title.empty()) natural = (content_.icon ? tokens::tooltip_head_icon_size + tokens::space_sm : 0.0) + host.measureText(content_.title, titleNoWrap, 0).width;
  for (const TooltipItem& item : content_.items) {
    double width = item.icon ? tokens::icon_size + kColumnGap : 0.0;
    double body = host.measureText(item.text, bodyNoWrap, 0).width;
    if (!item.title.empty()) body = std::max(body, host.measureText(item.title, itemTitleNoWrap, 0).width);
    natural = std::max(natural, width + body);
  }
  m.innerWidth = std::min(natural, innerMax);
  double content = 0.0;
  int children = 0;
  if (!content_.title.empty()) {
    const double titleHeight = host.measureText(content_.title, titleNoWrap, 0).height;
    m.headHeight = std::max(tokens::tooltip_head_icon_size, titleHeight) + tokens::space_md + border;
    content += m.headHeight;
    ++children;
  }
  for (const TooltipItem& item : content_.items) {
    const double bodyWidth = m.innerWidth - (item.icon ? tokens::icon_size + kColumnGap : 0.0) + 0.5;
    const double titleHeight = item.title.empty() ? 0.0 : host.measureText(item.title, textStyle(tokens::font_size_body, static_cast<int>(tokens::tooltip_item_title_font_weight), kTransparent, true), bodyWidth).height;
    const double textHeight = host.measureText(item.text, textStyle(tokens::font_size_body, 400, kTransparent, true), bodyWidth).height;
    const double body = titleHeight + (item.title.empty() ? 0.0 : kBodyGap) + textHeight;
    m.itemTitleHeights.push_back(titleHeight);
    m.itemHeights.push_back(std::max(body, item.icon ? kItemIconTop + tokens::icon_size : 0.0));
    content += m.itemHeights.back();
    ++children;
  }
  if (children > 1) content += (children - 1) * tokens::space_md;
  m.box = Size{m.innerWidth + 2 * tokens::tooltip_rich_padding_x + 2 * border, content + 2 * tokens::tooltip_rich_padding_y + 2 * border};
  return m;
}

void Tooltip::showTooltip(const Widget& target, const TooltipContent& content, Point pointer) {
  content_ = content;
  if (!shown_) {
    root_.showLayer(this, [](Widget*) {});
    shown_ = true;
  }
  const Metrics m = compute();
  const Rect box = target.bounds();
  // 小控件对齐控件，大控件对齐鼠标位置
  Rect anchor = box;
  if (box.width > kCompactSize || box.height > kCompactSize) anchor = Rect{pointer.x, pointer.y, kCursorWidth, kCursorHeight};
  const Size viewport = host()->viewportSize();
  const double centerY = anchor.y + anchor.height / 2.0;
  const double top = std::max(kViewportMargin, std::min(centerY - m.box.height / 2.0, viewport.height - m.box.height - kViewportMargin));
  const bool fitsRight = anchor.right() + kGap + m.box.width + kViewportMargin <= viewport.width;
  const double left = fitsRight ? anchor.right() + kGap : anchor.x - kGap - m.box.width;
  side_ = fitsRight ? Side::Right : Side::Left;
  arrowY_ = top + std::max(kArrowEdgeMin, std::min(centerY - top, m.box.height - kArrowEdgeMin));
  arrange(Rect{std::max(kViewportMargin, left), top, m.box.width, m.box.height});
  invalidate();
}

void Tooltip::hideTooltip() {
  if (!shown_) return;
  shown_ = false;
  if (host() != nullptr) root_.hideLayer(this);
}

void Tooltip::layerRects(std::vector<Rect>& out) const {
  if (!shown_) return;
  // 角标伸出提示框的左右两侧，一并算上
  constexpr double kArrowReach = 10;
  const Rect box = bounds();
  out.push_back(Rect{box.x - kArrowReach, box.y, box.width + 2 * kArrowReach, box.height});
}

void Tooltip::paintArrow(Canvas& canvas) const {
  const auto& colors = themeTokens(host()->theme());
  const Rect box = bounds();
  const double border = borderWidth();
  // 8×8 的方块旋转 45 度，对角线的一半长 5.657；方块中心在提示的边缘上，露在外面的是朝外的三角形
  const double reach = tokens::tooltip_arrow_size * std::sqrt(2.0) / 2.0;
  const double edge = side_ == Side::Right ? box.x : box.right();
  const double outward = side_ == Side::Right ? -1.0 : 1.0;
  const Point tip{edge + outward * reach, arrowY_};
  const Point upper{edge - outward * border, arrowY_ - reach};
  const Point lower{edge - outward * border, arrowY_ + reach};
  canvas.fillPolygon({tip, upper, lower}, colors.tooltip_background);
  canvas.strokeLine(tip, Point{edge, arrowY_ - reach}, border, colors.layer_border);
  canvas.strokeLine(tip, Point{edge, arrowY_ + reach}, border, colors.layer_border);
}

void Tooltip::paint(Canvas& canvas) {
  const auto& colors = themeTokens(host()->theme());
  const Rect box = bounds();
  const double border = borderWidth();
  const Metrics m = compute();
  canvas.drawShadow(box, tokens::radius_md, colors.layer_shadow);
  canvas.drawBox(box, BoxStyle{.fill = colors.tooltip_background, .radius = tokens::radius_md, .borderWidth = tokens::space_hairline, .borderColor = colors.layer_border});
  paintArrow(canvas);

  if (!content_.rich) {
    const double x = box.x + border + tokens::tooltip_padding_x;
    const double y = box.y + border + tokens::tooltip_padding_y;
    canvas.drawText(content_.text, Rect{x, y, m.innerWidth + 0.5, box.height}, textStyle(tokens::font_size_body, 400, colors.text, true), VerticalAlign::Top);
    return;
  }
  const double x = box.x + border + tokens::tooltip_rich_padding_x;
  double y = box.y + border + tokens::tooltip_rich_padding_y;
  const double width = m.innerWidth;
  if (!content_.title.empty()) {
    const TextStyle titleStyle = textStyle(tokens::tooltip_title_font_size, static_cast<int>(tokens::tooltip_title_font_weight), colors.text, false);
    const double titleHeight = canvas.measureText(content_.title, titleStyle, 0).height;
    const double rowHeight = std::max(tokens::tooltip_head_icon_size, titleHeight);
    double textX = x;
    if (content_.icon) {
      canvas.drawIcon(*content_.icon, Rect{x, y + (rowHeight - tokens::tooltip_head_icon_size) / 2.0, tokens::tooltip_head_icon_size, tokens::tooltip_head_icon_size}, colors.accent, tokens::icon_stroke_width);
      textX += tokens::tooltip_head_icon_size + tokens::space_sm;
    }
    canvas.drawText(content_.title, Rect{textX, y + (rowHeight - titleHeight) / 2.0, width - (textX - x) + 1, titleHeight}, titleStyle, VerticalAlign::Top);
    canvas.drawLine(Rect{x, y + rowHeight + tokens::space_md, width, border}, colors.divider);
    y += m.headHeight + tokens::space_md;
  }
  for (std::size_t i = 0; i < content_.items.size(); ++i) {
    const TooltipItem& item = content_.items[i];
    double bodyX = x;
    if (item.icon) {
      canvas.drawIcon(*item.icon, Rect{x, y + kItemIconTop, tokens::icon_size, tokens::icon_size}, colors.tooltip_item_icon_color, tokens::icon_stroke_width);
      bodyX += tokens::icon_size + kColumnGap;
    }
    const double bodyWidth = width - (bodyX - x) + 0.5;
    double textY = y;
    if (!item.title.empty()) {
      canvas.drawText(item.title, Rect{bodyX, y, bodyWidth, m.itemTitleHeights[i]}, textStyle(tokens::font_size_body, static_cast<int>(tokens::tooltip_item_title_font_weight), colors.text, true), VerticalAlign::Top);
      textY += m.itemTitleHeights[i] + kBodyGap;
    }
    canvas.drawText(item.text, Rect{bodyX, textY, bodyWidth, box.bottom() - textY}, textStyle(tokens::font_size_body, 400, colors.tooltip_text_color, true), VerticalAlign::Top);
    y += m.itemHeights[i] + tokens::space_md;
  }
}

}  // namespace rujian::ui
