/*
 * ------------------------------------------------------------------------
 * 名称：notification.cpp
 * 说明：通知中心的实现：弹出的通知、通知面板（标题行、历史列表、空状态）、消息摘要（按行数截断加省略号、较长的消息可点击）、状态栏上的通知按钮。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-19
 * 备注：尺寸与网页版一致：通知宽 340，内边跑 6×10，图标 14、关闭按钮 22、列间距 6；面板最高 480（也不高过窗口去掉底部偏移、标题栏与 10 的留白）；
 *       弹出通知的消息最多 3 行，面板里最多 4 行；通知层右下角离窗口边 10。
 * ------------------------------------------------------------------------
 */

#include <algorithm>
#include <ctime>

#include "base/theme-tokens.hpp"
#include "base/unicode.hpp"
#include "rujian/ui/button.hpp"
#include "rujian/ui/geometry.hpp"
#include "rujian/ui/layout.hpp"
#include "rujian/ui/notification.hpp"
#include "rujian/ui/scroll-area.hpp"
#include "rujian/ui/status-bar.hpp"

namespace rujian::ui {

namespace {

constexpr int kVirtualKeyReturn = 0x0D;
constexpr int kVirtualKeyEscape = 0x1B;
constexpr int kVirtualKeySpace = 0x20;
constexpr std::size_t kLongMessageLength = 60;
constexpr std::size_t kMaxHistory = 100;
constexpr std::size_t kMaxToasts = 3;
constexpr int kToastLines = 3;
constexpr int kPanelLines = 4;
constexpr double kIconColumn = 14.0;
constexpr double kColumnGap = 6.0;
constexpr double kIconTop = 2.0;
constexpr double kContentGap = 2.0;
constexpr double kPanelTitleLineHeight = 1.3;
constexpr double kBadgeGap = 2.0;
constexpr double kPanelMinGap = 10.0;
constexpr const char* kTitleText = "通知";
constexpr const char* kEmptyText = "暂无通知";

IconId iconFor(NoticeLevel level) {
  switch (level) {
    case NoticeLevel::Success: return IconId::CircleCheck;
    case NoticeLevel::Warning: return IconId::AlertTriangle;
    case NoticeLevel::Error: return IconId::AlertCircle;
    case NoticeLevel::Info: break;
  }
  return IconId::InfoCircle;
}

Color colorFor(NoticeLevel level, const tokens::ThemeTokens& colors) {
  switch (level) {
    case NoticeLevel::Success: return colors.status_success;
    case NoticeLevel::Warning: return colors.status_warning;
    case NoticeLevel::Error: return colors.status_error;
    case NoticeLevel::Info: break;
  }
  return colors.link;
}

double borderOf(const Widget& widget) { return snapBorderWidth(tokens::space_hairline, widget.host() != nullptr ? widget.host()->scale() : 1.0); }

TextStyle bodyStyle(Color color, bool wrap = true) {
  TextStyle style;
  style.fontSize = tokens::font_size_body;
  style.lineHeight = tokens::line_height_body;
  style.color = color;
  style.wrap = wrap;
  return style;
}

TextStyle smallStyle(double size, int weight, double lineHeight, Color color) {
  TextStyle style;
  style.fontSize = size;
  style.fontWeight = weight;
  style.lineHeight = lineHeight;
  style.color = color;
  return style;
}

/** 当前本地时间，“时:分”。 */
std::string localTime() {
  const std::time_t now = std::time(nullptr);
  std::tm local{};
  localtime_s(&local, &now);
  char text[8];
  std::strftime(text, sizeof(text), "%H:%M", &local);
  return text;
}

/** 按网页的排版规则折叠空白：换行、制表符与连续空格都变成一个空格，首尾的空白去掉。 */
std::string collapseWhitespace(const std::string& text) {
  std::string result;
  bool pendingSpace = false;
  for (const char c : text) {
    if (c == ' ' || c == '\n' || c == '\r' || c == '\t') {
      pendingSpace = !result.empty();
      continue;
    }
    if (pendingSpace) result.push_back(' ');
    pendingSpace = false;
    result.push_back(c);
  }
  return result;
}

/** 关闭按钮：无底色的纯图标按钮。 */
std::unique_ptr<Button> makeCloseButton(bool compact) {
  auto button = std::make_unique<Button>("关闭");
  button->setVariant(ButtonVariant::Ghost);
  button->setIconOnly(true);
  button->setIcon(IconId::X);
  button->setCompact(compact);
  return button;
}

}  // namespace

bool isLongNotice(const std::string& text) { return toUtf16(text).size() > kLongMessageLength || text.find('\n') != std::string::npos; }

namespace {

/** 通知消息：最多显示若干行，超出的部分以省略号收起；较长的消息可点击（或按回车、空格）查看全文。 */
class NoticeMessage final : public Widget {
 public:
  NoticeMessage(std::string text, int lines, std::function<void(const std::string&)> onExpand) : text_(std::move(text)), lines_(lines), onExpand_(std::move(onExpand)) {}

  [[nodiscard]] bool expandable() const { return isLongNotice(text_); }

  [[nodiscard]] Size measure(Size available) override { return Size{available.width, fit(available.width).height}; }
  [[nodiscard]] bool focusable() const override { return expandable(); }
  [[nodiscard]] CursorKind cursor() const override { return expandable() ? CursorKind::Hand : CursorKind::Arrow; }
  [[nodiscard]] bool stretchesHeight() const override { return false; }

  void paint(Canvas& canvas) override {
    const auto& colors = themeTokens(host()->theme());
    const Fit shown = fit(bounds().width);
    canvas.drawText16(shown.text, Rect{bounds().x, bounds().y, bounds().width, shown.height}, bodyStyle(colors.notice_surface_fg), VerticalAlign::Top);
    if (focusVisible()) canvas.strokeOutline(bounds(), 0, tokens::space_hairline, tokens::space_hairline, colors.focus_border);
  }

  bool onMouseDown(const MouseEvent& event) override { return expandable() && event.button == MouseButton::Left; }
  bool onMouseUp(const MouseEvent& event) override {
    if (!expandable() || event.button != MouseButton::Left) return false;
    if (pressed() && bounds().contains(event.position)) expand();
    return true;
  }
  bool onKeyDown(const KeyEvent& event) override {
    if (!expandable() || (event.virtualKey != kVirtualKeyReturn && event.virtualKey != kVirtualKeySpace)) return false;
    expand();
    return true;
  }

 private:
  /** 在给定宽度里实际显示的文字与高度。 */
  struct Fit {
    std::u16string text;
    double height{0};
  };

  void expand() {
    if (onExpand_) onExpand_(text_);
  }

  [[nodiscard]] Fit fit(double width) const {
    Host& host = *this->host();
    const TextStyle style = bodyStyle(kTransparent);
    const double lineHeight = tokens::font_size_body * tokens::line_height_body;
    const double limit = lines_ * lineHeight + 0.5;
    const std::u16string full = toUtf16(collapseWhitespace(text_));
    const double natural = host.measureText16(full, style, width).height;
    if (natural <= limit) return Fit{full, natural};
    // 放不下：找出加上省略号后刚好放进限定行数的最长前缀
    std::size_t low = 0;
    std::size_t high = full.size();
    while (low < high) {
      const std::size_t middle = (low + high + 1) / 2;
      const std::u16string candidate = full.substr(0, middle) + u"\u2026";
      if (host.measureText16(candidate, style, width).height <= limit) low = middle;
      else high = middle - 1;
    }
    return Fit{full.substr(0, low) + u"\u2026", lines_ * lineHeight};
  }

  std::string text_;
  int lines_;
  std::function<void(const std::string&)> onExpand_;
};

/** 弹出的一条通知。 */
class NoticeToast final : public Widget {
 public:
  NoticeToast(const NoticeRecord& record, NotificationCenter& center) : level_(record.level) {
    message_ = static_cast<NoticeMessage*>(addChild(std::make_unique<NoticeMessage>(record.text, kToastLines, [&center](const std::string& text) { center.showDetail(text); })));
    auto button = makeCloseButton(false);
    button->setOnClick([&center, id = record.id] { center.dismissToast(id); });
    close_ = static_cast<Button*>(addChild(std::move(button)));
  }

  [[nodiscard]] Size measure(Size available) override {
    const double width = available.width > 0 ? std::min(tokens::notice_width, available.width) : tokens::notice_width;
    const double border = borderOf(*this);
    const Size closeSize = close_->measure(Size{});
    const double content = width - 2 * border - 2 * tokens::space_page - kIconColumn - 2 * kColumnGap - closeSize.width;
    const double messageHeight = message_->measure(Size{content, 0}).height;
    const double row = std::max({kIconTop + tokens::icon_size, messageHeight, closeSize.height});
    return Size{width, 2 * border + 2 * tokens::space_sm + row};
  }

  void arrange(const Rect& bounds) override {
    Widget::arrange(bounds);
    const double border = borderOf(*this);
    const Size closeSize = close_->measure(Size{});
    const double left = bounds.x + border + tokens::space_page;
    const double top = bounds.y + border + tokens::space_sm;
    const double textX = left + kIconColumn + kColumnGap;
    const double closeX = bounds.right() - border - tokens::space_page - closeSize.width;
    message_->arrange(Rect{textX, top, closeX - kColumnGap - textX, message_->measure(Size{closeX - kColumnGap - textX, 0}).height});
    close_->arrange(Rect{closeX, top, closeSize.width, closeSize.height});
  }

  void paint(Canvas& canvas) override {
    const auto& colors = themeTokens(host()->theme());
    const double border = borderOf(*this);
    canvas.drawShadow(bounds(), tokens::radius_lg, colors.notice_surface_shadow);
    canvas.drawBox(bounds(), BoxStyle{.fill = colors.notice_surface_bg, .radius = tokens::radius_lg, .borderWidth = tokens::space_hairline, .borderColor = colors.notice_surface_border});
    canvas.drawIcon(iconFor(level_), Rect{bounds().x + border + tokens::space_page, bounds().y + border + tokens::space_sm + kIconTop, tokens::icon_size, tokens::icon_size}, colorFor(level_, colors), tokens::icon_stroke_width);
  }

 private:
  NoticeLevel level_;
  NoticeMessage* message_{nullptr};
  Button* close_{nullptr};
};

/** 通知面板里的一条历史。 */
class NoticeRow final : public Widget {
 public:
  NoticeRow(const NoticeRecord& record, NotificationCenter& center, bool separator) : level_(record.level), time_(record.time), separator_(separator) {
    message_ = static_cast<NoticeMessage*>(addChild(std::make_unique<NoticeMessage>(record.text, kPanelLines, [&center](const std::string& text) { center.showDetail(text); })));
    auto button = makeCloseButton(true);
    button->setOnClick([&center, id = record.id] { center.remove(id); });
    close_ = static_cast<Button*>(addChild(std::move(button)));
  }

  [[nodiscard]] Size measure(Size available) override {
    const Size closeSize = close_->measure(Size{});
    const double content = available.width - 2 * tokens::space_page - kIconColumn - 2 * kColumnGap - closeSize.width;
    const double messageHeight = message_->measure(Size{content, 0}).height;
    const double timeHeight = host()->measureText(time_, timeStyle(kTransparent), 0).height;
    const double row = std::max({kIconTop + tokens::icon_size, messageHeight + kContentGap + timeHeight, closeSize.height});
    return Size{available.width, (separator_ ? borderOf(*this) : 0.0) + 2 * tokens::space_sm + row};
  }

  void arrange(const Rect& bounds) override {
    Widget::arrange(bounds);
    const Size closeSize = close_->measure(Size{});
    const double top = bounds.y + (separator_ ? borderOf(*this) : 0.0) + tokens::space_sm;
    const double textX = bounds.x + tokens::space_page + kIconColumn + kColumnGap;
    const double closeX = bounds.right() - tokens::space_page - closeSize.width;
    const double width = closeX - kColumnGap - textX;
    message_->arrange(Rect{textX, top, width, message_->measure(Size{width, 0}).height});
    close_->arrange(Rect{closeX, top, closeSize.width, closeSize.height});
    timeY_ = top + message_->bounds().height + kContentGap;
    timeX_ = textX;
  }

  void paint(Canvas& canvas) override {
    const auto& colors = themeTokens(host()->theme());
    const double border = borderOf(*this);
    if (separator_) canvas.drawLine(Rect{bounds().x, bounds().y, bounds().width, border}, colors.notice_surface_border);
    const double top = bounds().y + (separator_ ? border : 0.0) + tokens::space_sm;
    canvas.drawIcon(iconFor(level_), Rect{bounds().x + tokens::space_page, top + kIconTop, tokens::icon_size, tokens::icon_size}, colorFor(level_, colors), tokens::icon_stroke_width);
    const TextStyle style = timeStyle(colors.notification_center_time_color);
    const double height = host()->measureText(time_, style, 0).height;
    canvas.drawText(time_, Rect{timeX_, timeY_, bounds().right() - timeX_, height}, style, VerticalAlign::Top);
  }

 private:
  [[nodiscard]] TextStyle timeStyle(Color color) const { return smallStyle(tokens::notification_center_time_font_size, 400, 0, color); }

  NoticeLevel level_;
  std::string time_;
  bool separator_;
  NoticeMessage* message_{nullptr};
  Button* close_{nullptr};
  double timeX_{0};
  double timeY_{0};
};

/** 通知面板：标题行与历史列表（或空状态）。 */
class NoticePanel final : public Widget {
 public:
  NoticePanel(const std::vector<NoticeRecord>& records, NotificationCenter& center) : hasRecords_(!records.empty()) {
    auto clear = std::make_unique<Button>("全部清除");
    clear->setVariant(ButtonVariant::Ghost);
    clear->setIconOnly(true);
    clear->setIcon(IconId::Eraser);
    clear->setCompact(true);
    clear->setEnabled(hasRecords_);
    clear->setOnClick([&center] { center.clear(); });
    clear_ = static_cast<Button*>(addChild(std::move(clear)));
    auto close = std::make_unique<Button>("关闭");
    close->setVariant(ButtonVariant::Ghost);
    close->setIconOnly(true);
    close->setIcon(IconId::X);
    close->setCompact(true);
    close->setOnClick([&center] { center.setOpen(false); });
    close_ = static_cast<Button*>(addChild(std::move(close)));
    if (hasRecords_) {
      auto list = std::make_unique<Stack>();
      list->setGap(0);
      for (std::size_t i = 0; i < records.size(); ++i) list->addChild(std::make_unique<NoticeRow>(records[i], center, i > 0));
      auto scroll = std::make_unique<ScrollArea>();
      scroll->setContent(std::move(list));
      scroll_ = static_cast<ScrollArea*>(addChild(std::move(scroll)));
    }
  }

  /** 第 index 条历史的区域。 */
  [[nodiscard]] Rect rowBounds(std::size_t index) const {
    if (scroll_ == nullptr || scroll_->content() == nullptr || index >= scroll_->content()->children().size()) return Rect{};
    return scroll_->content()->children()[index]->bounds();
  }
  [[nodiscard]] std::size_t rowTotal() const { return scroll_ != nullptr && scroll_->content() != nullptr ? scroll_->content()->children().size() : 0; }

  /** 面板的尺寸；available.height 是整个面板（含边框）允许的最大高度。 */
  [[nodiscard]] Size measure(Size available) override {
    const double border = borderOf(*this);
    const double width = available.width > 0 ? std::min(tokens::notice_width, available.width) : tokens::notice_width;
    const double header = headerHeight();
    const double bodyMax = std::max(0.0, available.height - 2 * border - header);
    double body = 0.0;
    if (scroll_ != nullptr) {
      scroll_->setMaxHeight(bodyMax);
      body = std::min(scroll_->measure(Size{width - 2 * border, 0}).height, bodyMax);
    } else {
      body = 2 * tokens::space_page + host()->measureText(kEmptyText, emptyStyle(kTransparent), 0).height;
    }
    return Size{width, 2 * border + header + body};
  }

  void arrange(const Rect& bounds) override {
    Widget::arrange(bounds);
    const double border = borderOf(*this);
    const double header = headerHeight();
    const Size buttonSize = close_->measure(Size{});
    const double closeX = bounds.right() - border - tokens::space_xs - buttonSize.width;
    const double top = bounds.y + border + tokens::space_xs;
    close_->arrange(Rect{closeX, top, buttonSize.width, buttonSize.height});
    clear_->arrange(Rect{closeX - tokens::space_xs - buttonSize.width, top, buttonSize.width, buttonSize.height});
    if (scroll_ != nullptr) scroll_->arrange(Rect{bounds.x + border, bounds.y + border + header, bounds.width - 2 * border, bounds.height - 2 * border - header});
  }

  void paint(Canvas& canvas) override {
    const auto& colors = themeTokens(host()->theme());
    const double border = borderOf(*this);
    canvas.drawShadow(bounds(), tokens::radius_lg, colors.notice_surface_shadow);
    canvas.drawBox(bounds(), BoxStyle{.fill = colors.notice_surface_bg, .radius = tokens::radius_lg, .borderWidth = tokens::space_hairline, .borderColor = colors.notice_surface_border});
    const double header = headerHeight();
    const TextStyle title = smallStyle(tokens::notification_center_title_font_size, static_cast<int>(tokens::notification_center_title_font_weight), kPanelTitleLineHeight, colors.notification_center_title_color);
    const double titleHeight = host()->measureText(kTitleText, title, 0).height;
    const double contentHeight = header - border;
    canvas.drawText(kTitleText, Rect{bounds().x + border + tokens::space_page, bounds().y + border + (contentHeight - titleHeight) / 2.0, 80, titleHeight}, title, VerticalAlign::Top);
    canvas.drawLine(Rect{bounds().x + border, bounds().y + border + header - border, bounds().width - 2 * border, border}, colors.notice_surface_border);
    if (scroll_ == nullptr) {
      const TextStyle empty = emptyStyle(colors.notification_center_empty_color);
      const double textHeight = host()->measureText(kEmptyText, empty, 0).height;
      canvas.drawText(kEmptyText, Rect{bounds().x + border + tokens::space_page, bounds().y + border + header + tokens::space_page, bounds().width - 2 * border - 2 * tokens::space_page, textHeight}, empty, VerticalAlign::Top);
    }
  }

 private:
  /** 标题行的高度：上下各 4 的内边距加按钮高度，再加下面的分隔线。 */
  [[nodiscard]] double headerHeight() const { return 2 * tokens::space_xs + close_->measure(Size{}).height + borderOf(*this); }
  [[nodiscard]] static TextStyle emptyStyle(Color color) { return smallStyle(tokens::notification_center_empty_font_size, 400, 0, color); }

  bool hasRecords_;
  Button* clear_{nullptr};
  Button* close_{nullptr};
  ScrollArea* scroll_{nullptr};
};

}  // namespace

// ---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
// NotificationCenter
// ---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

NotificationCenter::NotificationCenter(Root& root) : root_(root) {
  root_.showLayer(this, [this](Widget* hit) { onOutsidePress(hit); });
  arrange(Rect{0, 0, root_.viewportSize().width, root_.viewportSize().height});
}

NotificationCenter::~NotificationCenter() {
  clearTimers();
  if (host() != nullptr) root_.hideLayer(this);
}

void NotificationCenter::notify(std::string text, NoticeLevel level) {
  if (text.empty()) return;
  NoticeRecord record{nextId_++, std::move(text), level, timeSource_ ? timeSource_() : localTime()};
  records_.insert(records_.begin(), record);
  if (records_.size() > kMaxHistory) records_.resize(kMaxHistory);
  // 面板打开期间直接进入历史，不弹出、不计未读
  if (open_) {
    rebuild();
    return;
  }
  ++unread_;
  toasts_.push_back(record);
  // 超出上限时最早弹出的先收起
  while (toasts_.size() > kMaxToasts) {
    const int removed = toasts_.front().id;
    toasts_.erase(toasts_.begin());
    std::erase_if(timers_, [&](const auto& timer) {
      if (timer.second != removed) return false;
      host()->stopTimer(timer.first);
      return true;
    });
  }
  armTimer(record);
  rebuild();
}

void NotificationCenter::armTimer(const NoticeRecord& record) {
  const unsigned duration = toastDuration_ * (record.level == NoticeLevel::Error ? 2 : 1);
  timers_.emplace_back(host()->startTimer(this, duration), record.id);
}

void NotificationCenter::clearTimers() {
  if (host() != nullptr) {
    for (const auto& timer : timers_) host()->stopTimer(timer.first);
  }
  timers_.clear();
}

void NotificationCenter::onTimer(unsigned id) {
  const auto found = std::ranges::find(timers_, id, &std::pair<unsigned, int>::first);
  if (found == timers_.end()) return;
  const int noticeId = found->second;
  timers_.erase(found);
  dismissToast(noticeId);
}

void NotificationCenter::dismissToast(int id) {
  std::erase_if(toasts_, [id](const NoticeRecord& record) { return record.id == id; });
  std::erase_if(timers_, [&](const auto& timer) {
    if (timer.second != id) return false;
    host()->stopTimer(timer.first);
    return true;
  });
  rebuild();
}

void NotificationCenter::remove(int id) {
  std::erase_if(records_, [id](const NoticeRecord& record) { return record.id == id; });
  dismissToast(id);
}

void NotificationCenter::clear() {
  records_.clear();
  rebuild();
}

void NotificationCenter::setOpen(bool open) {
  if (open == open_) return;
  open_ = open;
  if (open_) {
    unread_ = 0;
    clearTimers();
    toasts_.clear();
  }
  rebuild();
}

void NotificationCenter::showDetail(const std::string& text) {
  if (onShowDetail_) onShowDetail_(text);
}

void NotificationCenter::setBottomOffset(double offset) {
  bottomOffset_ = offset;
  relayout();
}

bool NotificationCenter::handleEscape(int virtualKey) {
  if (!open_ || virtualKey != kVirtualKeyEscape) return false;
  setOpen(false);
  return true;
}

void NotificationCenter::windowDeactivated() {
  if (open_) setOpen(false);
}

void NotificationCenter::onOutsidePress(Widget* hit) {
  // 窗口大小变化（hit 为空）：重新摆放
  if (hit == nullptr) {
    arrange(Rect{0, 0, root_.viewportSize().width, root_.viewportSize().height});
    return;
  }
  if (!open_) return;
  for (const Widget* widget = hit; widget != nullptr && toggleWidget_ != nullptr; widget = widget->parent()) {
    if (widget == toggleWidget_) return;
  }
  setOpen(false);
}

Rect NotificationCenter::panelRect() const { return open_ && panel_ != nullptr ? panel_->bounds() : Rect{}; }

Rect NotificationCenter::toastRect(std::size_t index) const { return index < toastWidgets_.size() ? toastWidgets_[index]->bounds() : Rect{}; }

void NotificationCenter::layerRects(std::vector<Rect>& out) const {
  for (const Widget* toast : toastWidgets_) {
    if (toast->visible() && toast->bounds().width > 0 && toast->bounds().height > 0) out.push_back(toast->bounds());
  }
  const Rect panel = panelRect();
  if (panel.width > 0 && panel.height > 0) out.push_back(panel);
}

std::size_t NotificationCenter::rowCount() const {
  const auto* panel = dynamic_cast<const NoticePanel*>(panel_);
  return panel != nullptr ? panel->rowTotal() : 0;
}

Rect NotificationCenter::rowRect(std::size_t index) const {
  const auto* panel = dynamic_cast<const NoticePanel*>(panel_);
  return panel != nullptr ? panel->rowBounds(index) : Rect{};
}

Widget* NotificationCenter::hitTest(Point point) {
  for (auto it = children().rbegin(); it != children().rend(); ++it) {
    if ((*it)->visible() && (*it)->bounds().contains(point)) return (*it)->hitTest(point);
  }
  return nullptr;
}

Size NotificationCenter::measure(Size available) { return available; }

void NotificationCenter::arrange(const Rect& bounds) {
  Widget::arrange(bounds);
  relayout();
}

void NotificationCenter::rebuild() {
  while (!children().empty()) removeChild(children().back().get());
  toastWidgets_.clear();
  panel_ = nullptr;
  if (open_) {
    panel_ = addChild(std::make_unique<NoticePanel>(records_, *this));
  } else {
    for (const NoticeRecord& record : toasts_) toastWidgets_.push_back(addChild(std::make_unique<NoticeToast>(record, *this)));
  }
  relayout();
  invalidate();
}

void NotificationCenter::relayout() {
  if (host() == nullptr) return;
  const double right = bounds().right() - tokens::space_page;
  const double bottom = bounds().bottom() - bottomOffset_;
  const double maxWidth = std::max(0.0, bounds().width - 2 * tokens::space_page);
  if (panel_ != nullptr) {
    const double maxHeight = std::max(0.0, std::min(tokens::notification_center_max_height, bounds().height - bottomOffset_ - tokens::title_bar_height - tokens::space_page));
    const Size size = panel_->measure(Size{maxWidth, maxHeight});
    panel_->arrange(Rect{right - size.width, bottom - size.height, size.width, size.height});
    return;
  }
  double y = bottom;
  for (std::size_t i = toastWidgets_.size(); i > 0; --i) {
    Widget* toast = toastWidgets_[i - 1];
    const Size size = toast->measure(Size{maxWidth, 0});
    toast->arrange(Rect{right - size.width, y - size.height, size.width, size.height});
    y -= size.height + tokens::space_sm;
  }
}

// ---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
// NotificationButton
// ---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

NotificationButton::NotificationButton(NotificationCenter& center) : center_(center) {
  setTooltip("通知");
  center_.setToggleWidget(this);
}

bool NotificationButton::large() const {
  const auto* bar = dynamic_cast<const StatusBar*>(parent());
  return bar != nullptr && bar->large();
}

std::string NotificationButton::badge() const {
  if (center_.unread() <= 0) return {};
  return center_.unread() > 99 ? "99+" : std::to_string(center_.unread());
}

Size NotificationButton::measure(Size /*available*/) {
  const double height = large() ? tokens::status_bar_large_item_height : tokens::status_bar_item_height;
  const double padding = large() ? tokens::status_bar_large_item_padding_x : tokens::status_bar_item_padding_x;
  const double icon = large() ? tokens::status_bar_large_icon_size : tokens::status_bar_icon_size;
  double content = icon;
  const std::string text = badge();
  if (!text.empty()) content += kBadgeGap + host()->measureText(text, smallStyle(tokens::notification_badge_font_size, 400, 0, kTransparent), 0).width;
  return Size{std::max(height, 2 * padding + content), height};
}

void NotificationButton::paint(Canvas& canvas) {
  const auto& colors = themeTokens(host()->theme());
  Color background = kTransparent;
  Color foreground = colors.chrome_text;
  if (center_.open()) {
    background = colors.ghost_selected_background;
    foreground = colors.ghost_selected_foreground;
  } else if (pressed()) {
    background = colors.ghost_active_background;
    foreground = colors.ghost_active_foreground;
  } else if (hovered()) {
    background = colors.ghost_hover_background;
    foreground = colors.ghost_hover_foreground;
  }
  if (background.a() != 0) canvas.drawBox(bounds(), BoxStyle{.fill = background, .radius = tokens::radius_md});
  const double icon = large() ? tokens::status_bar_large_icon_size : tokens::status_bar_icon_size;
  const std::string text = badge();
  const TextStyle style = smallStyle(tokens::notification_badge_font_size, 400, 0, foreground);
  const double textWidth = text.empty() ? 0.0 : host()->measureText(text, style, 0).width;
  const double content = icon + (text.empty() ? 0.0 : kBadgeGap + textWidth);
  const double x = bounds().x + (bounds().width - content) / 2.0;
  canvas.drawIcon(IconId::Bell, Rect{x, bounds().y + (bounds().height - icon) / 2.0, icon, icon}, foreground, tokens::icon_stroke_width);
  if (!text.empty()) canvas.drawText(text, Rect{x + icon + kBadgeGap, bounds().y, textWidth + 1, bounds().height}, style, VerticalAlign::Center);
}

void NotificationButton::toggle() { center_.setOpen(!center_.open()); }

bool NotificationButton::onMouseDown(const MouseEvent& event) { return event.button == MouseButton::Left; }

bool NotificationButton::onMouseUp(const MouseEvent& event) {
  if (event.button != MouseButton::Left) return false;
  if (pressed() && bounds().contains(event.position)) toggle();
  return true;
}

bool NotificationButton::onKeyDown(const KeyEvent& event) {
  if (event.virtualKey == kVirtualKeyReturn) {
    if (!event.repeat) toggle();
    return true;
  }
  if (event.virtualKey == kVirtualKeySpace) {
    spaceDown_ = true;
    return true;
  }
  return false;
}

bool NotificationButton::onKeyUp(const KeyEvent& event) {
  if (event.virtualKey != kVirtualKeySpace) return false;
  if (spaceDown_) {
    spaceDown_ = false;
    toggle();
  }
  return true;
}

void NotificationButton::onStateChanged() {
  if (!focused()) spaceDown_ = false;
  Widget::onStateChanged();
}

}  // namespace rujian::ui
