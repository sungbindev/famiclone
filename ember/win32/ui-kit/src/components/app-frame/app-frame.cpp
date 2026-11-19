/*
 * ------------------------------------------------------------------------
 * 名称：app-frame.cpp
 * 说明：窗口框架的实现：标题栏、页面区、底部面板与状态栏自上而下的排布，页面区与底部面板的底色与分割线。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-19
 * 备注：窗口底色是外壳底色（chrome_bg）；页面区是内容区底色；底部面板是外壳底色，顶部有 1 像素分割线，高度含这条线。
 * ------------------------------------------------------------------------
 */

#include <algorithm>
#include <cmath>

#include "base/theme-tokens.hpp"
#include "rujian/ui/app-frame.hpp"
#include "rujian/ui/geometry.hpp"

namespace rujian::ui {

namespace {

/** 底部面板高度的上限占窗口高度的比例。 */
constexpr double kBottomMaxRatio = 0.7;

/** 页面区或底部面板的区域：画自己的底色（与顶部分割线），内容铺满去掉分割线的部分。 */
class Region final : public Widget {
 public:
  explicit Region(bool bottom) : bottom_(bottom) {}

  void setContent(std::unique_ptr<Widget> content) {
    if (content_ != nullptr) removeChild(content_);
    content_ = content ? addChild(std::move(content)) : nullptr;
  }

  void arrange(const Rect& bounds) override {
    Widget::arrange(bounds);
    if (content_ != nullptr) content_->arrange(innerRect());
  }

  void paint(Canvas& canvas) override {
    const auto& colors = themeTokens(host()->theme());
    canvas.fillRect(bounds(), bottom_ ? colors.chrome_bg : colors.content_bg);
    if (bottom_) canvas.drawLine(Rect{bounds().x, bounds().y, bounds().width, borderWidth()}, colors.chrome_border);
  }

 private:
  [[nodiscard]] double borderWidth() const { return snapBorderWidth(tokens::space_hairline, host() != nullptr ? host()->scale() : 1.0); }
  [[nodiscard]] Rect innerRect() const {
    const double top = bottom_ ? borderWidth() : 0.0;
    return Rect{bounds().x, bounds().y + top, bounds().width, std::max(0.0, bounds().height - top)};
  }

  bool bottom_;
  Widget* content_{nullptr};
};

}  // namespace

AppFrame::AppFrame() {
  page_ = addChild(std::make_unique<Region>(false));
  bottom_ = addChild(std::make_unique<Region>(true));
  bottom_->setVisible(false);
}

TitleBar* AppFrame::setTitleBar(std::unique_ptr<TitleBar> titleBar) {
  if (titleBar_ != nullptr) removeChild(titleBar_);
  titleBar_ = titleBar ? static_cast<TitleBar*>(addChild(std::move(titleBar))) : nullptr;
  return titleBar_;
}

StatusBar* AppFrame::setStatusBar(std::unique_ptr<StatusBar> statusBar) {
  if (statusBar_ != nullptr) removeChild(statusBar_);
  statusBar_ = statusBar ? static_cast<StatusBar*>(addChild(std::move(statusBar))) : nullptr;
  if (statusBar_ != nullptr) statusBar_->setLarge(statusBarLarge_);
  return statusBar_;
}

void AppFrame::setPage(std::unique_ptr<Widget> page) { static_cast<Region*>(page_)->setContent(std::move(page)); }

void AppFrame::setBottomPanel(std::unique_ptr<Widget> panel) { static_cast<Region*>(bottom_)->setContent(std::move(panel)); }

void AppFrame::setBottomPanelOpen(bool open) {
  bottomOpen_ = open;
  bottom_->setVisible(open);
  invalidate();
}

void AppFrame::setBottomPanelHeight(double height) {
  bottomHeight_ = height;
  invalidate();
}

void AppFrame::setBottomPanelMinHeight(double minHeight) {
  bottomMinHeight_ = minHeight;
  invalidate();
}

double AppFrame::shownBottomHeight() const {
  if (!bottomOpen_) return 0.0;
  const double maximum = std::max(bottomMinHeight_, std::floor(bounds().height * kBottomMaxRatio));
  return std::min(std::max(bottomHeight_, bottomMinHeight_), maximum);
}

void AppFrame::setStatusBarLarge(bool large) {
  statusBarLarge_ = large;
  if (statusBar_ != nullptr) statusBar_->setLarge(large);
  invalidate();
}

void AppFrame::setWindowInactive(bool inactive) {
  if (titleBar_ != nullptr) titleBar_->setWindowActive(!inactive);
  if (statusBar_ != nullptr) statusBar_->setWindowActive(!inactive);
}

void AppFrame::setNotificationCenter(NotificationCenter* center) { notifications_ = center; }

void AppFrame::arrange(const Rect& bounds) {
  Widget::arrange(bounds);
  double top = bounds.y;
  double bottom = bounds.bottom();
  if (titleBar_ != nullptr) {
    const double height = titleBar_->measure(Size{bounds.width, 0}).height;
    titleBar_->arrange(Rect{bounds.x, top, bounds.width, height});
    top += height;
  }
  if (statusBar_ != nullptr) {
    const double height = statusBar_->measure(Size{bounds.width, 0}).height;
    bottom -= height;
    statusBar_->arrange(Rect{bounds.x, bottom, bounds.width, height});
    // 通知让出状态栏的高度，再加 6 的间距
    if (notifications_ != nullptr) notifications_->setBottomOffset(height + tokens::space_sm);
  } else if (notifications_ != nullptr) {
    notifications_->setBottomOffset(tokens::notice_offset_bottom);
  }
  const double panel = std::min(shownBottomHeight(), std::max(0.0, bottom - top));
  if (panel > 0) {
    bottom_->arrange(Rect{bounds.x, bottom - panel, bounds.width, panel});
    bottom -= panel;
  }
  page_->arrange(Rect{bounds.x, top, bounds.width, std::max(0.0, bottom - top)});
}

void AppFrame::paint(Canvas& canvas) {
  canvas.fillRect(bounds(), themeTokens(host()->theme()).chrome_bg);
}

}  // namespace rujian::ui
