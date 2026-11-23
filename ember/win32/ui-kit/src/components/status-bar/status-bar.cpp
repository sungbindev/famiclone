/*
 * ------------------------------------------------------------------------
 * 名称：status-bar.cpp
 * 说明：状态栏的实现：左右两组状态项的排列、放不下时按权重收缩左侧较长的文字、底色与顶部分割线。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-19
 * 备注：左右各留 8；顶部有 1 像素分割线，状态项在去掉分割线的内容区里上下居中；同一组内相邻状态项之间留 4 像素，左右两组之间至少留 8 像素；
 *       右侧保持原宽靠右排列，左侧是一个可收缩的组：放不下时，较长文字的状态项按“文字字符数的平方 × 原宽”分担收缩量（与 CSS flex-shrink 的算法一致）；这样仍放不下时，所有带文字的项都按同样的规则收缩，文字以省略号结尾，不压到右侧的项上。
 * ------------------------------------------------------------------------
 */

#include <algorithm>

#include "base/theme-tokens.hpp"
#include "rujian/ui/geometry.hpp"
#include "rujian/ui/status-bar.hpp"

namespace rujian::ui {

namespace {

/** 按 CSS flex-shrink 的算法在 free（负数）的空间里分配各项的宽度；basis 是各项原宽，shrink 是各项的收缩权重，minimum 是各项的最小宽度。 */
std::vector<double> shrinkWidths(const std::vector<double>& basis, const std::vector<double>& shrink, const std::vector<double>& minimum, double overflow) {
  std::vector<double> widths = basis;
  std::vector<bool> frozen(basis.size(), false);
  double remaining = overflow;
  // 有项收缩到最小宽度被冻结时，剩下的量重新分配
  for (std::size_t round = 0; round <= basis.size() && remaining > 0.0; ++round) {
    double total = 0.0;
    for (std::size_t i = 0; i < basis.size(); ++i) {
      if (!frozen[i]) total += shrink[i] * basis[i];
    }
    if (total <= 0.0) break;
    bool hitMinimum = false;
    double used = 0.0;
    for (std::size_t i = 0; i < basis.size(); ++i) {
      if (frozen[i] || shrink[i] <= 0.0) continue;
      const double share = remaining * (shrink[i] * basis[i]) / total;
      const double next = widths[i] - share;
      if (next < minimum[i]) {
        used += widths[i] - minimum[i];
        widths[i] = minimum[i];
        frozen[i] = true;
        hitMinimum = true;
      } else {
        used += share;
        widths[i] = next;
      }
    }
    remaining -= used;
    if (!hitMinimum) break;
  }
  return widths;
}

}  // namespace

StatusBar::StatusBar() = default;

double StatusBar::borderWidth() const { return snapBorderWidth(tokens::space_hairline, host() != nullptr ? host()->scale() : 1.0); }

StatusBarItem* StatusBar::addStart(std::unique_ptr<StatusBarItem> item) {
  item->setLarge(large_);
  auto* raw = static_cast<StatusBarItem*>(addChild(std::move(item)));
  start_.push_back(raw);
  return raw;
}

StatusBarItem* StatusBar::addEnd(std::unique_ptr<StatusBarItem> item) {
  item->setLarge(large_);
  auto* raw = static_cast<StatusBarItem*>(addChild(std::move(item)));
  end_.push_back(raw);
  return raw;
}

Widget* StatusBar::addEndWidget(std::unique_ptr<Widget> widget) {
  Widget* raw = addChild(std::move(widget));
  end_.push_back(raw);
  return raw;
}

void StatusBar::clear() {
  for (StatusBarItem* item : start_) removeChild(item);
  for (Widget* item : end_) removeChild(item);
  start_.clear();
  end_.clear();
  invalidate();
}

void StatusBar::setLarge(bool large) {
  large_ = large;
  for (StatusBarItem* item : start_) item->setLarge(large);
  for (Widget* item : end_) {
    if (auto* statusItem = dynamic_cast<StatusBarItem*>(item)) statusItem->setLarge(large);
  }
  invalidate();
}

void StatusBar::setWindowActive(bool active) {
  windowActive_ = active;
  invalidate();
}

Size StatusBar::measure(Size available) { return Size{available.width, large_ ? tokens::status_bar_large_height : tokens::status_bar_height}; }

void StatusBar::arrange(const Rect& bounds) {
  Widget::arrange(bounds);
  const double border = borderWidth();
  const double contentTop = bounds.y + border;
  const double contentHeight = bounds.height - border;
  const double available = bounds.width - 2 * tokens::status_bar_padding_x;

  std::vector<Size> startSizes;
  std::vector<Size> endSizes;
  double startNatural = 0.0;
  double endWidth = 0.0;
  for (StatusBarItem* item : start_) {
    startSizes.push_back(item->measure(Size{}));
    startNatural += startSizes.back().width;
  }
  for (Widget* item : end_) {
    endSizes.push_back(item->measure(Size{}));
    endWidth += endSizes.back().width;
  }
  if (start_.size() > 1) startNatural += static_cast<double>(start_.size() - 1) * tokens::icon_text_gap;
  if (end_.size() > 1) endWidth += static_cast<double>(end_.size() - 1) * tokens::icon_text_gap;

  // 左侧的组放不下时收窄到剩下的空间，组内较长文字的状态项分担收缩量
  const double groupWidth = std::min(startNatural, std::max(0.0, available - endWidth - tokens::space_md));
  std::vector<double> basis;
  std::vector<double> shrink;
  std::vector<double> minimum;
  for (std::size_t i = 0; i < start_.size(); ++i) {
    basis.push_back(startSizes[i].width);
    shrink.push_back(start_[i]->shrinkWeight());
    minimum.push_back(0.0);
  }
  const double gaps = start_.size() > 1 ? static_cast<double>(start_.size() - 1) * tokens::icon_text_gap : 0.0;
  const double itemsNatural = startNatural - gaps;
  const double itemsAvailable = std::max(0.0, groupWidth - gaps);
  std::vector<double> widths = itemsAvailable < itemsNatural ? shrinkWidths(basis, shrink, minimum, itemsNatural - itemsAvailable) : basis;
  // 只让长文字收缩仍放不下时，所有带文字的项都按文字长度收缩（文字以省略号结尾），不压到右侧的项上
  double widthsTotal = 0.0;
  for (const double width : widths) widthsTotal += width;
  if (widthsTotal > itemsAvailable + 0.01) {
    std::vector<double> forcedShrink;
    std::vector<double> forcedMinimum;
    for (std::size_t i = 0; i < start_.size(); ++i) {
      forcedShrink.push_back(start_[i]->forcedShrinkWeight());
      forcedMinimum.push_back(std::max(0.0, basis[i] - start_[i]->textShrinkRoom()));
    }
    widths = shrinkWidths(basis, forcedShrink, forcedMinimum, itemsNatural - itemsAvailable);
  }

  double x = bounds.x + tokens::status_bar_padding_x;
  for (std::size_t i = 0; i < start_.size(); ++i) {
    start_[i]->arrange(Rect{x, contentTop + (contentHeight - startSizes[i].height) / 2.0, widths[i], startSizes[i].height});
    x += widths[i] + tokens::icon_text_gap;
  }
  x = bounds.right() - tokens::status_bar_padding_x - endWidth;
  for (std::size_t i = 0; i < end_.size(); ++i) {
    end_[i]->arrange(Rect{x, contentTop + (contentHeight - endSizes[i].height) / 2.0, endSizes[i].width, endSizes[i].height});
    x += endSizes[i].width + tokens::icon_text_gap;
  }
}

void StatusBar::paint(Canvas& canvas) {
  const auto& colors = themeTokens(host()->theme());
  canvas.fillRect(bounds(), windowActive_ ? colors.status_bar_bg : colors.status_bar_inactive_bg);
  const double border = borderWidth();
  canvas.drawLine(Rect{bounds().x, bounds().y, bounds().width, border}, colors.chrome_border);
}

}  // namespace rujian::ui
