/*
 * ------------------------------------------------------------------------
 * 名称：dialog-frame.cpp
 * 说明：对话框框架的实现：标题栏（图标、标题、最小化、最大化与关闭按钮）、正文滚动区域、右对齐的按钮区、键盘处理与命中测试。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-19
 * 备注：标题栏高 32（含底部 1 像素分割线），左边留 6、有图标时图标 16、与标题之间留 6，标题与按钮之间留 4；
 *       默认的按钮是 24×24 的小方块、右边留 4、相互间隔 2；系统按钮样式与主窗口标题栏一致：宽 46、占满标题栏高度、靠窗口右边紧挨着排列、不参与 Tab 遍历；
 *       正文四周留 10；按钮区上方留 6、下方与左右各留 10，按钮之间留 6、最小宽度 72。
 * ------------------------------------------------------------------------
 */

#include <algorithm>

#include "base/theme-tokens.hpp"
#include "rujian/ui/dialog-frame.hpp"
#include "rujian/ui/geometry.hpp"
#include "rujian/ui/layout.hpp"
#include "rujian/ui/system-glyph.hpp"

namespace rujian::ui {

namespace {

constexpr int kVirtualKeyReturn = 0x0D;
constexpr int kVirtualKeyEscape = 0x1B;
constexpr int kVirtualKeySpace = 0x20;
/** 最小尺寸的余量（逻辑像素）：文字宽度刚好放下时不因浮点误差换行。 */
constexpr double kFitSlack = 0.5;

/** 铺满正文区的正文包装：四周留正文内边距（flushRight 时右边不留），正文自己占满剩下的区域（对应网页版正文元素的 height: 100%）。 */
class FillBody final : public Widget {
 public:
  FillBody(std::unique_ptr<Widget> content, bool flushRight) : rightPadding_(flushRight ? 0.0 : tokens::space_page) { content_ = addChild(std::move(content)); }

  [[nodiscard]] Widget* content() const { return content_; }

  [[nodiscard]] Size measure(Size available) override {
    const double inner = std::max(0.0, available.width - tokens::space_page - rightPadding_);
    return Size{available.width, content_->measure(Size{inner, 0}).height + 2 * tokens::space_page};
  }

  void arrange(const Rect& bounds) override {
    setBounds(bounds);
    content_->arrange(Rect{bounds.x + tokens::space_page, bounds.y + tokens::space_page, std::max(0.0, bounds.width - tokens::space_page - rightPadding_), std::max(0.0, bounds.height - 2 * tokens::space_page)});
  }

 private:
  double rightPadding_;
  Widget* content_{nullptr};
};

/** 标题栏里的按钮：网页版的小方块，或和主窗口标题栏一样的系统按钮（线条图形、宽 46、不参与 Tab 遍历）。 */
class DialogControl final : public Widget {
 public:
  DialogControl(IconId icon, bool closeStyle, std::function<void()> onClick) : icon_(icon), closeStyle_(closeStyle), onClick_(std::move(onClick)) {}
  /** 系统按钮样式：windowActive 指向框架的窗口激活状态，给标题栏底色。 */
  DialogControl(SystemGlyph glyph, const bool* windowActive, std::function<void()> onClick) : closeStyle_(glyph == SystemGlyph::Close), glyph_(glyph), windowActive_(windowActive), onClick_(std::move(onClick)) {}

  void setIcon(IconId icon) {
    icon_ = icon;
    invalidate();
  }
  void setGlyph(SystemGlyph glyph) {
    glyph_ = glyph;
    invalidate();
  }

  [[nodiscard]] Size measure(Size /*available*/) override {
    if (glyph_) return Size{tokens::title_bar_tool_width, tokens::dialog_titlebar_height};
    return Size{tokens::dialog_control_size, tokens::dialog_control_size};
  }
  [[nodiscard]] bool focusable() const override { return !glyph_.has_value(); }
  [[nodiscard]] bool stretchesHeight() const override { return false; }
  [[nodiscard]] CursorKind cursor() const override { return glyph_ ? CursorKind::Arrow : CursorKind::Hand; }

  void paint(Canvas& canvas) override {
    const auto& colors = themeTokens(host()->theme());
    if (glyph_) {
      paintSystemStyle(canvas, colors);
      return;
    }
    Color background = kTransparent;
    Color foreground = colors.ghost_foreground;
    if (closeStyle_ && (pressed() || hovered())) {
      background = pressed() ? colors.dialog_close_active_bg : colors.dialog_close_hover_bg;
      foreground = colors.dialog_close_hover_text;
    } else if (pressed()) {
      background = colors.ghost_active_background;
      foreground = colors.ghost_active_foreground;
    } else if (hovered()) {
      background = colors.ghost_hover_background;
      foreground = colors.ghost_hover_foreground;
    }
    if (background.a() != 0) canvas.drawBox(bounds(), BoxStyle{.fill = background, .radius = tokens::radius_md});
    const double size = tokens::btn_icon_size;
    canvas.drawIcon(icon_, Rect{bounds().x + (bounds().width - size) / 2.0, bounds().y + (bounds().height - size) / 2.0, size, size}, foreground, tokens::icon_stroke_width);
    if (focusVisible()) canvas.strokeOutline(bounds(), tokens::radius_md, tokens::space_hairline, tokens::space_hairline, colors.focus_border);
  }

  bool onMouseDown(const MouseEvent& event) override { return event.button == MouseButton::Left; }
  bool onMouseUp(const MouseEvent& event) override {
    if (event.button != MouseButton::Left) return false;
    if (pressed() && bounds().contains(event.position)) fire();
    return true;
  }
  bool onKeyDown(const KeyEvent& event) override {
    if (event.virtualKey == kVirtualKeyReturn) {
      if (!event.repeat) fire();
      return true;
    }
    if (event.virtualKey == kVirtualKeySpace) {
      spaceDown_ = true;
      return true;
    }
    return false;
  }
  bool onKeyUp(const KeyEvent& event) override {
    if (event.virtualKey != kVirtualKeySpace) return false;
    if (spaceDown_) {
      spaceDown_ = false;
      fire();
    }
    return true;
  }
  void onStateChanged() override {
    if (!focused()) spaceDown_ = false;
    Widget::onStateChanged();
  }

 private:
  /** 系统按钮样式：底色与悬停、按下的颜色同主窗口标题栏的系统按钮。 */
  void paintSystemStyle(Canvas& canvas, const tokens::ThemeTokens& colors) {
    const bool active = windowActive_ == nullptr || *windowActive_;
    const Color background = active ? colors.dialog_titlebar_bg : colors.dialog_titlebar_inactive_bg;
    Color fill = background;
    Color glyph = active ? colors.chrome_text_strong : colors.chrome_text;
    if (closeStyle_ && (pressed() || hovered())) {
      fill = pressed() ? kSystemClosePressedBackground : kSystemCloseHoverBackground;
      glyph = kSystemCloseGlyph;
    } else if (pressed()) {
      fill = compose(colors.ghost_active_background, background);
    } else if (hovered()) {
      fill = compose(colors.ghost_hover_background, background);
    }
    const double border = snapBorderWidth(tokens::space_hairline, host()->scale());
    canvas.fillRect(Rect{bounds().x, bounds().y, bounds().width, bounds().height - border}, fill);
    paintSystemGlyph(canvas, *glyph_, bounds(), glyph);
  }

  void fire() {
    if (onClick_) onClick_();
  }

  IconId icon_{IconId::X};
  bool closeStyle_;
  std::optional<SystemGlyph> glyph_;
  const bool* windowActive_{nullptr};
  std::function<void()> onClick_;
  bool spaceDown_{false};
};

}  // namespace

DialogFrame::DialogFrame() {
  scroll_ = static_cast<ScrollArea*>(addChild(std::make_unique<ScrollArea>()));
}

double DialogFrame::borderWidth() const { return snapBorderWidth(tokens::space_hairline, host() != nullptr ? host()->scale() : 1.0); }

double DialogFrame::footerHeight() const { return buttons_.empty() ? 0.0 : tokens::space_sm + tokens::btn_height + tokens::space_page; }

void DialogFrame::setTitle(std::string title) {
  title_ = std::move(title);
  invalidate();
}

void DialogFrame::setTitleDetail(std::string detail) {
  titleDetail_ = std::move(detail);
  invalidate();
}

void DialogFrame::setContent(std::unique_ptr<Widget> content) {
  content_ = nullptr;
  wrapper_ = nullptr;
  if (!content) {
    scroll_->setContent(nullptr);
    return;
  }
  scroll_->setFitContent(contentFill_);
  if (contentFill_) {
    auto body = std::make_unique<FillBody>(std::move(content), contentFlushRight_);
    content_ = body->content();
    wrapper_ = body.get();
    scroll_->setContent(std::move(body));
    invalidate();
    return;
  }
  auto wrapper = std::make_unique<Stack>();
  wrapper->setPadding(Insets::all(tokens::space_page));
  content_ = wrapper->addChild(std::move(content));
  wrapper_ = wrapper.get();
  scroll_->setContent(std::move(wrapper));
  invalidate();
}

void DialogFrame::setButtons(std::vector<DialogButton> buttons) {
  for (Button* button : buttons_) removeChild(button);
  buttons_.clear();
  specs_ = std::move(buttons);
  for (const DialogButton& spec : specs_) {
    auto button = std::make_unique<Button>(spec.text);
    button->setVariant(spec.variant);
    if (spec.icon) button->setIcon(spec.icon);
    button->setMinWidth(tokens::dialog_button_min_width);
    button->setEnabled(!spec.disabled);
    button->setOnClick([this, id = spec.id] { activate(id); });
    buttons_.push_back(static_cast<Button*>(addChild(std::move(button))));
  }
  invalidate();
}

Button* DialogFrame::button(const std::string& id) const {
  for (std::size_t i = 0; i < specs_.size(); ++i) {
    if (specs_[i].id == id) return buttons_[i];
  }
  return nullptr;
}

void DialogFrame::setButtonDisabled(const std::string& id, bool disabled) {
  if (Button* target = button(id)) target->setEnabled(!disabled);
}

std::string DialogFrame::defaultButtonId() const {
  for (const DialogButton& spec : specs_) {
    if (spec.isDefault) return spec.id;
  }
  return {};
}

void DialogFrame::activate(const std::string& id) {
  if (onButton_) onButton_(id);
}

void DialogFrame::rebuildControls() {
  for (Widget* control : {minimize_, maximize_, close_}) {
    if (control != nullptr) removeChild(control);
  }
  minimize_ = nullptr;
  maximize_ = nullptr;
  close_ = nullptr;
  if (systemButtons_) {
    if (minimizable_) {
      minimize_ = addChild(std::make_unique<DialogControl>(SystemGlyph::Minimize, &windowActive_, [this] {
        if (onMinimize_) onMinimize_();
      }));
    }
    if (maximizable_) {
      maximize_ = addChild(std::make_unique<DialogControl>(maximized_ ? SystemGlyph::Restore : SystemGlyph::Maximize, &windowActive_, [this] {
        if (onToggleMaximize_) onToggleMaximize_();
      }));
    }
    if (closable_) {
      close_ = addChild(std::make_unique<DialogControl>(SystemGlyph::Close, &windowActive_, [this] {
        if (onClose_) onClose_(DialogCloseReason::CloseButton);
      }));
    }
  } else {
    if (maximizable_) {
      maximize_ = addChild(std::make_unique<DialogControl>(maximized_ ? IconId::AppWindow : IconId::Square, false, [this] {
        if (onToggleMaximize_) onToggleMaximize_();
      }));
    }
    if (closable_) {
      close_ = addChild(std::make_unique<DialogControl>(IconId::X, true, [this] {
        if (onClose_) onClose_(DialogCloseReason::CloseButton);
      }));
    }
  }
  if (host() != nullptr && bounds().width > 0) arrange(bounds());
  invalidate();
}

void DialogFrame::setLogo(std::unique_ptr<Widget> logo) {
  if (logo_ != nullptr) removeChild(logo_);
  logo_ = logo ? addChild(std::move(logo)) : nullptr;
  if (host() != nullptr && bounds().width > 0) arrange(bounds());
  invalidate();
}

void DialogFrame::setSystemButtons(bool systemButtons) {
  systemButtons_ = systemButtons;
  rebuildControls();
}

void DialogFrame::setMinimizable(bool minimizable) {
  minimizable_ = minimizable;
  rebuildControls();
}

void DialogFrame::setMaximizable(bool maximizable) {
  maximizable_ = maximizable;
  rebuildControls();
}

void DialogFrame::setClosable(bool closable) {
  closable_ = closable;
  rebuildControls();
}

void DialogFrame::setMaximized(bool maximized) {
  maximized_ = maximized;
  if (maximize_ == nullptr) return;
  auto* control = static_cast<DialogControl*>(maximize_);
  if (systemButtons_) control->setGlyph(maximized ? SystemGlyph::Restore : SystemGlyph::Maximize);
  else control->setIcon(maximized ? IconId::AppWindow : IconId::Square);
}

void DialogFrame::setWindowActive(bool active) {
  windowActive_ = active;
  invalidate();
}

void DialogFrame::focusInitial() {
  if (host() != nullptr) host()->setFocus(this, false);
}

Rect DialogFrame::titleBarRect() const { return Rect{bounds().x, bounds().y, bounds().width, tokens::dialog_titlebar_height}; }

DialogHit DialogFrame::hitTestWindow(Point point) const {
  if (!titleBarRect().contains(point)) return DialogHit::Client;
  for (const Widget* control : {minimize_, maximize_, close_}) {
    if (control != nullptr && control->bounds().contains(point)) return DialogHit::Client;
  }
  return DialogHit::Caption;
}

double DialogFrame::naturalHeight(double width, double maxBodyHeight) {
  scroll_->setMaxHeight(maxBodyHeight);
  return tokens::dialog_titlebar_height + scroll_->measure(Size{width, 0}).height + footerHeight();
}

Size DialogFrame::minimumWindowSize() {
  const Size body = content_ != nullptr ? content_->minimumSize() : Size{};
  const double rightPadding = contentFill_ && contentFlushRight_ ? 0.0 : tokens::space_page;
  double controls = tokens::space_sm + tokens::space_xs + (logo_ != nullptr ? tokens::title_bar_logo_size + tokens::space_sm : 0.0);
  for (const Widget* control : {minimize_, maximize_, close_}) {
    if (control != nullptr) controls += systemButtons_ ? tokens::title_bar_tool_width : tokens::dialog_control_size + tokens::space_xxs;
  }
  const double bodyWidth = body.width > 0 ? body.width + tokens::space_page + rightPadding + kFitSlack : 0.0;
  const double bodyHeight = body.height > 0 ? body.height + 2 * tokens::space_page + kFitSlack : 0.0;
  return Size{std::max(controls, bodyWidth), tokens::dialog_titlebar_height + bodyHeight + footerHeight()};
}

void DialogFrame::arrange(const Rect& bounds) {
  Widget::arrange(bounds);
  const double titleHeight = tokens::dialog_titlebar_height;
  const double contentHeight = titleHeight - borderWidth();
  if (logo_ != nullptr) logo_->arrange(Rect{bounds.x + tokens::space_sm, bounds.y + (contentHeight - tokens::title_bar_logo_size) / 2.0, tokens::title_bar_logo_size, tokens::title_bar_logo_size});
  if (systemButtons_) {
    // 系统按钮：占满标题栏高度，靠窗口右边紧挨着排列
    double edge = bounds.right();
    for (Widget* control : {close_, maximize_, minimize_}) {
      if (control == nullptr) continue;
      control->arrange(Rect{edge - tokens::title_bar_tool_width, bounds.y, tokens::title_bar_tool_width, titleHeight});
      edge -= tokens::title_bar_tool_width;
    }
  } else {
    const double controlY = bounds.y + (contentHeight - tokens::dialog_control_size) / 2.0;
    double right = bounds.right() - tokens::space_xs;
    for (Widget* control : {close_, maximize_}) {
      if (control == nullptr) continue;
      control->arrange(Rect{right - tokens::dialog_control_size, controlY, tokens::dialog_control_size, tokens::dialog_control_size});
      right -= tokens::dialog_control_size + tokens::space_xxs;
    }
  }
  const double footer = footerHeight();
  scroll_->setMaxHeight(0);
  scroll_->arrange(Rect{bounds.x, bounds.y + titleHeight, bounds.width, std::max(0.0, bounds.height - titleHeight - footer)});
  double x = bounds.right() - tokens::space_page;
  const double y = bounds.bottom() - tokens::space_page - tokens::btn_height;
  for (auto it = buttons_.rbegin(); it != buttons_.rend(); ++it) {
    const double width = (*it)->measure(Size{}).width;
    x -= width;
    (*it)->arrange(Rect{x, y, width, tokens::btn_height});
    x -= tokens::space_sm;
  }
}

void DialogFrame::paint(Canvas& canvas) {
  const auto& colors = themeTokens(host()->theme());
  const Rect box = bounds();
  const double border = borderWidth();
  canvas.fillRect(box, colors.bg);
  const Rect titleBar = titleBarRect();
  canvas.fillRect(titleBar, windowActive_ ? colors.dialog_titlebar_bg : colors.dialog_titlebar_inactive_bg);
  canvas.drawLine(Rect{titleBar.x, titleBar.bottom() - border, titleBar.width, border}, colors.dialog_titlebar_border);
  TextStyle style;
  style.fontSize = tokens::dialog_title_font_size;
  style.fontWeight = static_cast<int>(tokens::dialog_title_font_weight);
  style.lineHeight = 0;
  style.color = colors.text;
  style.overflow = TextOverflow::Ellipsis;
  // 标题占满控件之左的空间，与控件之间留 4
  double textRight = box.right() - tokens::space_xs;
  bool hasControl = false;
  for (const Widget* control : {minimize_, maximize_, close_}) {
    if (control == nullptr) continue;
    textRight = hasControl ? std::min(textRight, control->bounds().x) : control->bounds().x;
    hasControl = true;
  }
  if (hasControl) textRight -= tokens::space_xs;
  const double textX = box.x + tokens::space_sm + (logo_ != nullptr ? tokens::title_bar_logo_size + tokens::space_sm : 0.0);
  const Rect textBox{textX, titleBar.y, std::max(0.0, textRight - textX), titleBar.height - border};
  if (titleDetail_.empty()) {
    canvas.drawText(title_, textBox, style, VerticalAlign::Center);
    return;
  }
  // 标题优先完整显示，说明文字接在后面、用剩下的空间
  const double titleWidth = std::min(host()->measureText(title_, style, 0).width + 1.0, textBox.width);
  canvas.drawText(title_, Rect{textBox.x, textBox.y, titleWidth, textBox.height}, style, VerticalAlign::Center);
  const double detailX = textBox.x + titleWidth + tokens::space_sm;
  if (detailX >= textRight) return;
  TextStyle detailStyle = style;
  detailStyle.fontWeight = 400;
  detailStyle.color = colors.text_muted;
  canvas.drawText(titleDetail_, Rect{detailX, textBox.y, textRight - detailX, textBox.height}, detailStyle, VerticalAlign::Center);
}

bool DialogFrame::onKeyDown(const KeyEvent& event) {
  if (event.virtualKey == kVirtualKeyEscape && closable_) {
    if (onClose_) onClose_(DialogCloseReason::Escape);
    return true;
  }
  if (event.virtualKey == kVirtualKeyReturn) {
    // 在输入框或没有焦点时触发默认按钮；焦点在按钮、多行文本等控件上时回车保留原有含义
    const Widget* focused = host() != nullptr ? host()->focusedWidget() : nullptr;
    const std::string id = defaultButtonId();
    if (!id.empty() && (focused == nullptr || focused == this || focused->acceptsText())) {
      if (const Button* target = button(id); target != nullptr && target->enabled()) {
        activate(id);
        return true;
      }
    }
  }
  return false;
}

}  // namespace rujian::ui
