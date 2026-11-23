/*
 * ------------------------------------------------------------------------
 * 名称：title-bar.cpp
 * 说明：标题栏的实现：布局（应用图标、应用名、菜单栏、操作区、折叠按钮、系统按钮）、绘制、命中测试。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-19
 * 备注：高 36，底部 1 像素分割线；内容区（去掉分割线）里各项上下居中。左边留 8；应用名与菜单栏之间留 8；
 *       操作区与折叠按钮靠右，各自右边留 10；折叠按钮宽 46、占满内容区高度，相邻按钮之间用与底色相同的 1 像素竖线分开；
 *       系统按钮宽 46、占满标题栏的全部高度（包括分割线），图形是 10×10 的线条，悬停与按下的颜色取系统常见的样式。
 * ------------------------------------------------------------------------
 */

#include <algorithm>

#include "base/theme-tokens.hpp"
#include "rujian/ui/geometry.hpp"
#include "rujian/ui/system-glyph.hpp"
#include "rujian/ui/title-bar.hpp"

namespace rujian::ui {

namespace {

/** 系统按钮的数量：最小化、最大化或还原、关闭。 */
constexpr int kSystemButtonCount = 3;

}  // namespace

TitleBar::TitleBar() = default;

double TitleBar::borderWidth() const { return snapBorderWidth(tokens::space_hairline, host() != nullptr ? host()->scale() : 1.0); }

TextStyle TitleBar::titleStyle(Color color) const {
  TextStyle style;
  style.fontSize = tokens::font_size_chrome;
  style.fontWeight = static_cast<int>(tokens::title_bar_brand_font_weight);
  style.lineHeight = 0;
  style.color = color;
  style.face = TextFace::Brand;
  style.letterSpacing = tokens::title_bar_title_letter_spacing_em;
  return style;
}

TextStyle TitleBar::subtitleStyle(Color color) const {
  TextStyle style = titleStyle(color);
  style.fontWeight = static_cast<int>(tokens::title_bar_subtitle_font_weight);
  style.letterSpacing = 0;
  return style;
}

double TitleBar::titleWidth() const { return title_.empty() ? 0.0 : host()->measureText(title_, titleStyle(kTransparent), 0).width; }

double TitleBar::subtitleWidth() const { return subtitle_.empty() ? 0.0 : host()->measureText(" " + subtitle_, subtitleStyle(kTransparent), 0).width; }

void TitleBar::relayout() {
  if (host() != nullptr && bounds().width > 0) arrange(bounds());
  invalidate();
}

void TitleBar::setTitle(std::string title) {
  title_ = std::move(title);
  relayout();
}

void TitleBar::setSubtitle(std::string subtitle) {
  subtitle_ = std::move(subtitle);
  relayout();
}

void TitleBar::setLogo(std::unique_ptr<Widget> logo) {
  if (logo_ != nullptr) removeChild(logo_);
  logo_ = logo ? addChild(std::move(logo)) : nullptr;
  relayout();
}

MenuBar* TitleBar::setMenuBar(std::unique_ptr<MenuBar> menuBar) {
  if (menuBar_ != nullptr) removeChild(menuBar_);
  menuBar_ = menuBar ? static_cast<MenuBar*>(addChild(std::move(menuBar))) : nullptr;
  relayout();
  return menuBar_;
}

Widget* TitleBar::addAction(std::unique_ptr<Widget> action) {
  Widget* raw = addChild(std::move(action));
  actions_.push_back(raw);
  relayout();
  return raw;
}

void TitleBar::clearActions() {
  for (Widget* action : actions_) removeChild(action);
  actions_.clear();
  relayout();
}

void TitleBar::setTools(std::vector<TitleBarTool> tools) {
  for (Button* button : toolButtons_) removeChild(button);
  toolButtons_.clear();
  for (TitleBarTool& tool : tools) {
    auto button = std::make_unique<Button>(tool.label);
    button->setVariant(ButtonVariant::Ghost);
    button->setIcon(tool.icon);
    button->setIconOnly(true);
    button->setFrameStyle(true);
    button->setToggled(tool.expanded);
    button->setEnabled(!tool.disabled);
    button->setOnClick([this, id = tool.id] {
      if (onToolClick_) onToolClick_(id);
    });
    toolButtons_.push_back(static_cast<Button*>(addChild(std::move(button))));
  }
  relayout();
}

void TitleBar::setWindowButtons(bool show) {
  windowButtons_ = show;
  relayout();
}

void TitleBar::setMaximized(bool maximized) {
  maximized_ = maximized;
  invalidate();
}

void TitleBar::setWindowActive(bool active) {
  windowActive_ = active;
  invalidate();
}

void TitleBar::setSystemButtonState(TitleBarHit hover, TitleBarHit pressed) {
  if (hover == hover_ && pressed == pressed_) return;
  hover_ = hover;
  pressed_ = pressed;
  invalidate();
}

Rect TitleBar::systemButtonRect(TitleBarHit button) const {
  if (!windowButtons_) return Rect{};
  int index = -1;
  if (button == TitleBarHit::Minimize) index = 0;
  else if (button == TitleBarHit::Maximize) index = 1;
  else if (button == TitleBarHit::Close) index = 2;
  if (index < 0) return Rect{};
  const double width = tokens::title_bar_tool_width;
  return Rect{bounds().right() - (kSystemButtonCount - index) * width, bounds().y, width, bounds().height};
}

TitleBarHit TitleBar::hitTestWindow(Point point) const {
  if (!bounds().contains(point)) return TitleBarHit::Client;
  for (const TitleBarHit button : {TitleBarHit::Minimize, TitleBarHit::Maximize, TitleBarHit::Close}) {
    if (systemButtonRect(button).contains(point)) return button;
  }
  // 标题栏里的控件（菜单栏、操作区、折叠按钮）是客户区，其余空白处拖动窗口
  for (const auto& child : children()) {
    if (child->visible() && child->bounds().contains(point)) return TitleBarHit::Client;
  }
  return TitleBarHit::Caption;
}

Size TitleBar::measure(Size available) { return Size{available.width, tokens::title_bar_height}; }

void TitleBar::arrange(const Rect& bounds) {
  Widget::arrange(bounds);
  const double contentHeight = bounds.height - borderWidth();
  const double centerY = bounds.y + contentHeight / 2.0;
  double x = bounds.x + tokens::space_md;
  if (logo_ != nullptr) {
    logo_->arrange(Rect{x, centerY - tokens::title_bar_logo_size / 2.0, tokens::title_bar_logo_size, tokens::title_bar_logo_size});
    x += tokens::title_bar_logo_size + tokens::space_sm;
  }
  titleX_ = x;
  x += titleWidth() + subtitleWidth() + tokens::space_md;

  // 右侧：系统按钮之左依次是折叠按钮与操作区，各自右边留 title_bar_tool_gap
  const double right = bounds.right() - (windowButtons_ ? kSystemButtonCount * tokens::title_bar_tool_width : 0.0);
  const double toolsWidth = static_cast<double>(toolButtons_.size()) * tokens::title_bar_tool_width;
  const double toolsX = right - tokens::title_bar_tool_gap - toolsWidth;
  for (std::size_t i = 0; i < toolButtons_.size(); ++i) {
    toolButtons_[i]->arrange(Rect{toolsX + static_cast<double>(i) * tokens::title_bar_tool_width, bounds.y, tokens::title_bar_tool_width, contentHeight});
  }
  double actionsWidth = 0.0;
  std::vector<Size> sizes;
  for (Widget* action : actions_) {
    sizes.push_back(action->measure(Size{}));
    actionsWidth += sizes.back().width;
  }
  double ax = (toolButtons_.empty() ? right : toolsX) - tokens::title_bar_tool_gap - actionsWidth;
  // 窗口太窄、操作区会压到名称上时整个隐藏，不显示也不响应点击
  const bool actionsFit = actions_.empty() || ax >= x;
  double menuLimit = ax;
  if (!actionsFit) menuLimit = toolButtons_.empty() ? right : toolsX;
  for (std::size_t i = 0; i < actions_.size(); ++i) {
    actions_[i]->setVisible(actionsFit);
    if (actionsFit) actions_[i]->arrange(Rect{ax, centerY - sizes[i].height / 2.0, sizes[i].width, sizes[i].height});
    ax += sizes[i].width;
  }

  // 菜单栏只占名称与右侧控件之间的空位，放不下的菜单名不显示
  if (menuBar_ != nullptr) {
    const Size size = menuBar_->measure(Size{});
    menuBar_->arrange(Rect{x, centerY - size.height / 2.0, std::min(size.width, std::max(0.0, menuLimit - x)), size.height});
  }
}

void TitleBar::paint(Canvas& canvas) {
  const auto& colors = themeTokens(host()->theme());
  const Rect box = bounds();
  const double border = borderWidth();
  const double contentHeight = box.height - border;
  canvas.fillRect(box, windowActive_ ? colors.title_bar_bg : colors.title_bar_inactive_bg);
  canvas.drawLine(Rect{box.x, box.bottom() - border, box.width, border}, colors.chrome_border);
  const double titleW = titleWidth();
  if (!title_.empty()) canvas.drawText(title_, Rect{titleX_, box.y, titleW + 1, contentHeight}, titleStyle(colors.title_bar_title_color), VerticalAlign::Center);
  if (!subtitle_.empty()) {
    const double subtitleW = subtitleWidth();
    canvas.drawText(" " + subtitle_, Rect{titleX_ + titleW, box.y, subtitleW + 1, contentHeight}, subtitleStyle(colors.title_bar_subtitle_color), VerticalAlign::Center);
  }
}

void TitleBar::paintChildren(Canvas& canvas) {
  Widget::paintChildren(canvas);
  const auto& colors = themeTokens(host()->theme());
  const Color background = windowActive_ ? colors.title_bar_bg : colors.title_bar_inactive_bg;
  // 相邻折叠按钮之间的分隔：与标题栏同色的竖线（按整数物理像素绘制，缩放不是 100% 时各处间隔才一致）
  for (std::size_t i = 1; i < toolButtons_.size(); ++i) {
    const Rect button = toolButtons_[i]->bounds();
    canvas.fillRect(Rect{button.x, button.y, borderWidth(), button.height}, background);
  }
  if (windowButtons_) paintSystemButtons(canvas);
}

void TitleBar::paintSystemButtons(Canvas& canvas) {
  const auto& colors = themeTokens(host()->theme());
  const Color background = windowActive_ ? colors.title_bar_bg : colors.title_bar_inactive_bg;
  const Color glyphColor = windowActive_ ? colors.chrome_text_strong : colors.chrome_text;
  for (const TitleBarHit button : {TitleBarHit::Minimize, TitleBarHit::Maximize, TitleBarHit::Close}) {
    const Rect box = systemButtonRect(button);
    const bool isPressed = pressed_ == button;
    const bool isHover = hover_ == button && (pressed_ == TitleBarHit::Client || isPressed);
    Color fill = background;
    Color glyph = glyphColor;
    if (button == TitleBarHit::Close && (isPressed || isHover)) {
      fill = isPressed ? kSystemClosePressedBackground : kSystemCloseHoverBackground;
      glyph = kSystemCloseGlyph;
    } else if (isPressed) {
      fill = compose(colors.ghost_active_background, background);
    } else if (isHover) {
      fill = compose(colors.ghost_hover_background, background);
    }
    canvas.fillRect(Rect{box.x, box.y, box.width, box.height - borderWidth()}, fill);
    SystemGlyph shape = SystemGlyph::Close;
    if (button == TitleBarHit::Minimize) shape = SystemGlyph::Minimize;
    else if (button == TitleBarHit::Maximize) shape = maximized_ ? SystemGlyph::Restore : SystemGlyph::Maximize;
    paintSystemGlyph(canvas, shape, box, glyph);
  }
}

}  // namespace rujian::ui
