/*
 * ------------------------------------------------------------------------
 * 名称：select.cpp
 * 说明：下拉框控件的实现：触发器（Select）与弹层里的选项列表（SelectPopup），尺寸与颜色取自生成的令牌，取值与 ui-select.css 一致。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-19
 * 备注：触发器边框的优先级与样式表一致：禁用 > 键盘焦点 > 按下或展开 > 悬停 > 常态；
 *       箭头与已选项的对勾都是“右边与下边各有一段边框的方块旋转 45 度”，与复选框的对勾用同样的取整方式，这里画成一个多边形（元素整体带不透明度，叠加处不能重复上色）；
 *       弹层定位、宽度与高度按 popup-placement.ts 的规则：默认在下方，放不下且上方更宽裕时翻到上方，最窄与触发器同宽。
 * ------------------------------------------------------------------------
 */

#include "rujian/ui/select.hpp"

#include <algorithm>
#include <cmath>

#include "base/theme-tokens.hpp"
#include "rujian/ui/scroll-area.hpp"

namespace rujian::ui {

namespace {

constexpr int kVirtualKeyTab = 0x09;
constexpr int kVirtualKeyReturn = 0x0D;
constexpr int kVirtualKeyEscape = 0x1B;
constexpr int kVirtualKeySpace = 0x20;
constexpr int kVirtualKeyEnd = 0x23;
constexpr int kVirtualKeyHome = 0x24;
constexpr int kVirtualKeyUp = 0x26;
constexpr int kVirtualKeyDown = 0x28;

constexpr double kBorderWidth = 1.0;
/** outline: 1px solid; outline-offset: -1px（键盘焦点）。 */
constexpr double kFocusWidth = 1.0;
constexpr double kFocusOffset = -1.0;
/** 弹层与触发器的间距、与窗口边缘至少留出的距离、最大高度。 */
constexpr double kPopupGap = 2.0;
constexpr double kViewportMargin = 8.0;
constexpr double kMaxPopupHeight = 240.0;
/** 滚轮每格滚动的距离（逻辑像素）。 */
constexpr double kWheelStep = 48.0;
/** 触发器的箭头：方块边长、右边的位置、纵向的 margin-top（展开时上移的少一些）、边框宽度、不透明度。 */
constexpr double kArrowSize = 6.5;
constexpr double kArrowRight = 8.0;
constexpr double kArrowMargin = -4.0;
constexpr double kArrowMarginExpanded = -1.0;
constexpr double kArrowStroke = 1.5;
constexpr double kArrowOpacity = 0.75;
/** 已选项的对勾：相对 select_option_mark_size 的宽、高、左边位置，边框宽度；向上平移自身高度的 65%。 */
constexpr double kMarkWidthFactor = 0.35;
constexpr double kMarkHeightFactor = 0.65;
constexpr double kMarkLeftFactor = 0.3;
constexpr double kMarkStroke = 1.5;
constexpr double kMarkShift = 0.65;

}  // namespace

// ---------------------------------------------------------------------------
// 弹层：选项列表
// ---------------------------------------------------------------------------

class SelectPopup;

/** 弹层里的选项列表：只负责画选项与处理鼠标，放在滚动区域里；高亮、选中状态由弹层给。 */
class SelectRows final : public Widget {
 public:
  SelectRows(SelectPopup& popup, std::vector<Select::Entry> entries, int selected, int active, bool highlight)
      : popup_(popup), entries_(std::move(entries)), selected_(selected), active_(active), highlight_(highlight) {}

  [[nodiscard]] const Select::Entry& entry(std::size_t index) const { return entries_[index]; }
  [[nodiscard]] std::size_t count() const { return entries_.size(); }
  [[nodiscard]] int active() const { return active_; }
  [[nodiscard]] bool marked() const { return selected_ >= 0 && !entries_[static_cast<std::size_t>(selected_)].placeholder; }

  /** 选项文字左边为对勾列让出的宽度：有已选项时所有选项都让出。 */
  [[nodiscard]] double textInset() const { return marked() ? tokens::select_option_mark_size + tokens::select_option_mark_gap : 0.0; }

  [[nodiscard]] TextStyle rowStyle(Color color) const {
    TextStyle style;
    style.fontSize = tokens::select_font_size;
    style.fontWeight = 400;
    style.lineHeight = 0;
    style.color = color;
    style.overflow = TextOverflow::Ellipsis;
    return style;
  }

  [[nodiscard]] double rowHeight() const { return 2 * tokens::select_option_padding_y + host()->measureText("A", rowStyle(kTransparent), 0).height; }

  /** 内容宽度：最宽的选项文字加上内边距。 */
  [[nodiscard]] double naturalWidth() const {
    double widest = 0;
    for (const Select::Entry& entry : entries_) widest = std::max(widest, host()->measureText(entry.label, rowStyle(kTransparent), 0).width);
    return widest + textInset() + tokens::select_option_padding_x + 2 * tokens::select_popup_padding;
  }

  [[nodiscard]] Size measure(Size available) override {
    return Size{available.width, 2 * tokens::select_popup_padding + rowHeight() * static_cast<double>(entries_.size())};
  }

  /** 第 index 项的区域（窗口坐标，含滚动偏移）。 */
  [[nodiscard]] Rect rowRect(std::size_t index) const {
    return Rect{bounds().x + tokens::select_popup_padding, bounds().y + tokens::select_popup_padding + rowHeight() * static_cast<double>(index), bounds().width - 2 * tokens::select_popup_padding, rowHeight()};
  }

  void setActive(int index, bool highlight) {
    active_ = index;
    highlight_ = highlight;
    invalidate();
  }

  [[nodiscard]] CursorKind cursor() const override { return CursorKind::Hand; }

  void paint(Canvas& canvas) override {
    const auto& colors = themeTokens(host()->theme());
    const Rect clip = parent() != nullptr ? parent()->bounds() : bounds();
    for (std::size_t i = 0; i < entries_.size(); ++i) {
      const Rect row = rowRect(i);
      if (row.bottom() < clip.y || row.y > clip.bottom()) continue;
      const bool active = highlight_ && static_cast<int>(i) == active_;
      if (active) canvas.drawBox(row, BoxStyle{.fill = colors.menu_selection_background, .radius = tokens::menu_item_radius});
      Color color = colors.menu_foreground;
      if (entries_[i].placeholder) color = colors.placeholder_color;
      if (active) color = colors.menu_selection_foreground;
      canvas.drawText(entries_[i].label, Rect{row.x + tokens::select_option_padding_x + textInset(), row.y + tokens::select_option_padding_y, std::max(0.0, row.width - tokens::select_option_padding_x * 2 - textInset()), row.height - 2 * tokens::select_option_padding_y}, rowStyle(color), VerticalAlign::Top);
      if (marked() && static_cast<int>(i) == selected_) drawMark(canvas, row);
    }
  }

  bool onMouseDown(const MouseEvent& event) override {
    if (event.button != MouseButton::Left) return false;
    downIndex_ = indexAt(event.position);
    return true;
  }

  bool onMouseUp(const MouseEvent& event) override;

  bool onMouseMove(const MouseEvent& event) override {
    const int index = indexAt(event.position);
    if (index >= 0 && (index != active_ || !highlight_)) {
      setActive(index, true);
    }
    return true;
  }

  void onStateChanged() override {
    // 鼠标移出弹层后去掉高亮底色，保留高亮位置让键盘从原位置继续
    if (!hovered() && highlight_ && mouseSeen_) highlight_ = false;
    if (hovered()) mouseSeen_ = true;
    Widget::onStateChanged();
  }

 private:
  [[nodiscard]] int indexAt(Point point) const {
    for (std::size_t i = 0; i < entries_.size(); ++i) {
      const Rect row = rowRect(i);
      if (point.y >= row.y && point.y < row.bottom()) return static_cast<int>(i);
    }
    return -1;
  }

  /** 已选项的对勾：left 在选项内边距之内，竖直居中后上移自身高度的 65%，再旋转 45 度。 */
  void drawMark(Canvas& canvas, const Rect& row) const {
    const auto& colors = themeTokens(host()->theme());
    const bool active = highlight_ && selected_ == active_;
    const Color color = active ? colors.menu_selection_foreground : colors.menu_foreground;
    const double size = tokens::select_option_mark_size;
    const double markHeight = size * kMarkHeightFactor;
    canvas.fillRotatedCorner(row.x + tokens::select_option_padding_x + size * kMarkLeftFactor, row.y + row.height / 2.0, size * kMarkWidthFactor, markHeight, kMarkStroke, 45.0, -kMarkShift * markHeight, color);
  }

  SelectPopup& popup_;
  std::vector<Select::Entry> entries_;
  int selected_;
  int active_;
  bool highlight_;
  bool mouseSeen_{false};
  int downIndex_{-1};
};

/** 显示在窗口弹层上的下拉弹层：外框（阴影、底色、边框）加一个滚动区域，滚动区域里是选项列表；键盘操作由触发器转发进来。 */
class SelectPopup final : public Widget {
 public:
  SelectPopup(Select& owner, std::vector<Select::Entry> entries, int selected, int active, bool highlight) : owner_(owner) {
    auto rows = std::make_unique<SelectRows>(*this, std::move(entries), selected, active, highlight);
    rows_ = rows.get();
    auto area = std::make_unique<ScrollArea>();
    scroll_ = area.get();
    area->setContent(std::move(rows));
    addChild(std::move(area));
  }

  [[nodiscard]] const Select::Entry& entry(std::size_t index) const { return rows_->entry(index); }
  [[nodiscard]] int active() const { return rows_->active(); }

  /** 按触发器的位置与窗口大小算出弹层的位置与大小并摆放。 */
  void place(const Rect& trigger) {
    const double border = snapBorderWidth(kBorderWidth, host()->scale());
    const double natural = rows_->measure(Size{0, 0}).height + 2 * border;
    const Size viewport = host()->viewportSize();
    const double below = viewport.height - trigger.bottom() - kViewportMargin;
    const double above = trigger.y - kViewportMargin;
    const double wanted = std::min(natural, kMaxPopupHeight);
    const bool placeBelow = below >= wanted || below >= above;
    const double maxHeight = std::max(std::min(kMaxPopupHeight, placeBelow ? below : above), 0.0);
    const double height = std::min(natural, maxHeight);
    // 宽度：最窄与触发器同宽，按最长的选项撑宽，不超出窗口
    const double content = rows_->naturalWidth() + 2 * border;
    const double width = std::min(std::max(content, trigger.width), std::max(viewport.width - 2 * kViewportMargin, 0.0));
    const double left = std::max(std::min(trigger.x, viewport.width - kViewportMargin - width), kViewportMargin);
    const double top = placeBelow ? trigger.bottom() + kPopupGap : trigger.y - kPopupGap - height;
    arrange(Rect{left, top, width, height});
    scrollToActive();
  }

  /** 第 index 项的区域（窗口坐标，含滚动偏移）。 */
  [[nodiscard]] Rect rowRect(std::size_t index) const { return rows_->rowRect(index); }

  void setActive(int index, bool highlight) {
    rows_->setActive(index, highlight);
    scrollToActive();
  }

  void moveActive(int delta) {
    const int count = static_cast<int>(rows_->count());
    setActive(((rows_->active() + delta) % count + count) % count, true);
  }

  void choose(std::size_t index) { owner_.choose(index); }

  void arrange(const Rect& bounds) override {
    Widget::arrange(bounds);
    const double border = snapBorderWidth(kBorderWidth, host()->scale());
    scroll_->arrange(bounds.inset(Insets::all(border)));
  }

  void paint(Canvas& canvas) override {
    const auto& colors = themeTokens(host()->theme());
    canvas.drawShadow(bounds(), tokens::menu_popup_radius, colors.menu_shadow);
    canvas.drawBox(bounds(), BoxStyle{.fill = colors.menu_background, .radius = tokens::menu_popup_radius, .borderWidth = kBorderWidth, .borderColor = colors.menu_border});
  }

  void paintChildren(Canvas& canvas) override {
    const double border = snapBorderWidth(kBorderWidth, host()->scale());
    canvas.pushClip(bounds().inset(Insets::all(border)), std::max(0.0, tokens::menu_popup_radius - border));
    Widget::paintChildren(canvas);
    canvas.popClip();
  }

 private:
  void scrollToActive() {
    const int active = rows_->active();
    if (active >= 0) scroll_->scrollIntoView(rows_->rowRect(static_cast<std::size_t>(active)));
  }

  Select& owner_;
  ScrollArea* scroll_{nullptr};
  SelectRows* rows_{nullptr};
};

bool SelectRows::onMouseUp(const MouseEvent& event) {
  if (event.button != MouseButton::Left) return false;
  const int down = downIndex_;
  downIndex_ = -1;
  // choose 会关闭并销毁弹层（也包括自己），之后不能再访问成员
  if (down >= 0 && down == indexAt(event.position)) popup_.choose(static_cast<std::size_t>(down));
  return true;
}

// ---------------------------------------------------------------------------
// 触发器
// ---------------------------------------------------------------------------

Select::Select() = default;

Select::~Select() = default;

void Select::setOptions(std::vector<SelectOption> options) {
  options_ = std::move(options);
  closePopup();
  invalidate();
}

void Select::setValue(const std::string& value) {
  current_ = value;
  invalidate();
}

void Select::setPlaceholder(std::string placeholder) {
  placeholder_ = std::move(placeholder);
  invalidate();
}

void Select::setAllowEmpty(bool allowEmpty) {
  allowEmpty_ = allowEmpty;
  closePopup();
  invalidate();
}

int Select::optionIndexOf(const std::string& value) const {
  for (std::size_t i = 0; i < options_.size(); ++i) {
    if (options_[i].value == value) return static_cast<int>(i);
  }
  return -1;
}

const std::string& Select::shownText(bool& isPlaceholder) const {
  const int index = optionIndexOf(current_);
  isPlaceholder = index < 0;
  return index < 0 ? placeholder_ : options_[static_cast<std::size_t>(index)].label;
}

std::vector<Select::Entry> Select::buildEntries() const {
  std::vector<Entry> entries;
  if (allowEmpty_) entries.push_back(Entry{"", placeholder_, true});
  for (const SelectOption& option : options_) entries.push_back(Entry{option.value, option.label, false});
  return entries;
}

TextStyle Select::textStyle(Color color) const {
  TextStyle style;
  style.fontSize = tokens::select_font_size;
  style.fontWeight = 400;
  style.lineHeight = 0;
  style.color = color;
  style.overflow = TextOverflow::Ellipsis;
  return style;
}

std::optional<Rect> Select::optionRect(std::size_t index) const {
  if (popup_ == nullptr || index >= buildEntries().size()) return std::nullopt;
  return popup_->rowRect(index);
}

Size Select::measure(Size available) {
  if (host() == nullptr) return Size{};
  if (available.width > 0) return Size{available.width, tokens::select_height};
  const double border = snapBorderWidth(kBorderWidth, host()->scale());
  bool placeholder = false;
  const std::string& text = shownText(placeholder);
  const double width = host()->measureText(text, textStyle(kTransparent), 0).width + tokens::select_padding_left + tokens::select_padding_right + 2 * border;
  return Size{std::max(tokens::select_min_width, width), tokens::select_height};
}

void Select::paint(Canvas& canvas) {
  const auto& colors = themeTokens(host()->theme());
  const Rect box = bounds();
  const bool off = !enabled();
  const double scale = canvas.scale();
  const double border = snapBorderWidth(kBorderWidth, scale);

  Color borderColor = colors.input_border;
  if (off) borderColor = colors.disabled_border;
  else if (focusVisible()) borderColor = colors.focus_border;
  else if (pressed() || open()) borderColor = colors.input_border_active;
  else if (hovered()) borderColor = colors.input_border_hover;
  canvas.drawBox(box, BoxStyle{.fill = off ? colors.disabled_bg : colors.input_bg, .radius = tokens::input_radius, .borderWidth = kBorderWidth, .borderColor = borderColor});
  if (focusVisible() && !off) canvas.strokeOutline(box, tokens::input_radius, kFocusOffset, kFocusWidth, colors.focus_border);

  // 当前项的文字：在去掉边框与内边距的区域里，行高取输入框的内容高度，垂直居中；过长用省略号
  bool placeholder = false;
  const std::string& text = shownText(placeholder);
  Color textColor = colors.input_foreground;
  if (placeholder) textColor = colors.placeholder_color;
  if (off) textColor = colors.disabled_text;
  const Rect field{box.x + border + tokens::select_padding_left, box.y + border, std::max(0.0, box.width - 2 * border - tokens::select_padding_left - tokens::select_padding_right), tokens::input_field_height};
  canvas.drawText(text, field, textStyle(textColor), VerticalAlign::Center);

  // 箭头：右边距内边距框 8px；纵向是“内边距框高度的一半”加上 margin-top（边框在两边对称，所以与边框宽度无关），展开时上移并翻转
  const double arrowLeft = box.right() - border - kArrowRight - kArrowSize;
  const double arrowTop = box.y + tokens::select_height / 2.0 + (open() ? kArrowMarginExpanded : kArrowMargin);
  canvas.fillRotatedCorner(arrowLeft, arrowTop, kArrowSize, kArrowSize, kArrowStroke, open() ? 225.0 : 45.0, 0.0, (off ? colors.disabled_text : colors.input_foreground).withOpacity(kArrowOpacity));
}

void Select::openPopup(bool byKeyboard) {
  if (open() || !enabled() || host() == nullptr) return;
  std::vector<Entry> entries = buildEntries();
  if (entries.empty()) return;
  const int option = optionIndexOf(current_);
  int selected = -1;
  for (std::size_t i = 0; i < entries.size(); ++i) {
    if (option >= 0 ? entries[i].value == current_ : entries[i].placeholder) selected = static_cast<int>(i);
  }
  popup_ = std::make_unique<SelectPopup>(*this, std::move(entries), selected, std::max(selected, 0), byKeyboard);
  host()->showLayer(popup_.get(), [this](Widget* hit) {
    if (hit != this) closePopup();
  });
  popup_->place(bounds());
  invalidate();
}

void Select::closePopup() {
  if (popup_ == nullptr) return;
  if (host() != nullptr) host()->hideLayer(popup_.get());
  popup_.reset();
  invalidate();
}

void Select::choose(std::size_t index) {
  if (popup_ == nullptr) return;
  const Entry entry = popup_->entry(index);
  closePopup();
  if (entry.value == current_) return;
  current_ = entry.value;
  invalidate();
  if (onChange_) onChange_(current_);
}

bool Select::onMouseDown(const MouseEvent& event) { return event.button == MouseButton::Left; }

bool Select::onMouseUp(const MouseEvent& event) {
  if (event.button != MouseButton::Left) return false;
  if (pressed() && bounds().contains(event.position)) {
    if (open()) closePopup();
    else openPopup(false);
  }
  return true;
}

bool Select::onKeyDown(const KeyEvent& event) {
  if (!enabled()) return false;
  const int key = event.virtualKey;
  if (!open()) {
    if (key == kVirtualKeyDown || key == kVirtualKeyUp || key == kVirtualKeyReturn || key == kVirtualKeySpace) {
      openPopup(true);
      return true;
    }
    return false;
  }
  switch (key) {
    case kVirtualKeyDown:
      popup_->moveActive(1);
      return true;
    case kVirtualKeyUp:
      popup_->moveActive(-1);
      return true;
    case kVirtualKeyHome:
      popup_->setActive(0, true);
      return true;
    case kVirtualKeyEnd:
      popup_->setActive(static_cast<int>(buildEntries().size()) - 1, true);
      return true;
    case kVirtualKeyReturn:
    case kVirtualKeySpace:
      choose(static_cast<std::size_t>(popup_->active()));
      return true;
    case kVirtualKeyEscape:
      closePopup();
      return true;
    case kVirtualKeyTab:
      // 只关闭弹层，Tab 仍然移动焦点
      closePopup();
      return false;
    default:
      return false;
  }
}

void Select::onStateChanged() {
  // 失去焦点或被禁用都关闭弹层
  if ((wasFocused_ && !focused()) || !enabled()) closePopup();
  wasFocused_ = focused();
  Widget::onStateChanged();
}

}  // namespace rujian::ui
