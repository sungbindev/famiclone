/*
 * ------------------------------------------------------------------------
 * 名称：text-input.cpp
 * 说明：单行文字输入框的实现。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-19
 * 备注：外观取自 ui-input-controls.css：高 24（含 1px 边框）、圆角、悬停时边框提亮、聚焦时边框与向内偏移 1px 的焦点环同色、校验失败与禁用的配色；
 *       文字在 22px 高的输入区里上下居中，左右各留 8px 内边距，超出部分裁掉并随光标滚动。
 * ------------------------------------------------------------------------
 */

#include "rujian/ui/text-input.hpp"

#include <algorithm>
#include <cmath>

#include "base/theme-tokens.hpp"
#include "base/unicode.hpp"
#include "components/text-input/line-editor.hpp"

namespace rujian::ui {

namespace {

constexpr int kVirtualKeyBack = 0x08;
constexpr int kVirtualKeyReturn = 0x0D;
constexpr int kVirtualKeyEnd = 0x23;
constexpr int kVirtualKeyHome = 0x24;
constexpr int kVirtualKeyLeft = 0x25;
constexpr int kVirtualKeyRight = 0x27;
constexpr int kVirtualKeyInsert = 0x2D;
constexpr int kVirtualKeyDelete = 0x2E;

constexpr double kBorderWidth = 1.0;
/** 输入框没有给定宽度时的默认宽度。 */
constexpr double kDefaultWidth = 200.0;
/** outline: 1px。 */
constexpr double kFocusWidth = 1.0;
/** 光标宽度（逻辑像素）。 */
constexpr double kCaretWidth = 1.0;
/** 焦点环的偏移：outline-offset: -1px，环在边框框里面。 */
constexpr double kFocusOffset = -1.0;
/** 组合串下划线的粗细：普通与正在转换的分句。 */
constexpr double kComposeUnderline = 1.0;
constexpr double kComposeTargetUnderline = 2.0;
/** 搜索框清除按钮：命中区大小、中心距盒子右边缘的距离、十字的半边长与线宽（网页版实测）；有内容时输入区右侧让出的宽度。 */
constexpr double kClearHitSize = 11.0;
constexpr double kClearCenterFromRight = 15.5;
constexpr double kClearArmX = 3.0;
constexpr double kClearArmY = 3.2;
constexpr double kClearLineWidth = 2.1;
constexpr double kClearReserved = 12.0;

/** 选区底色与选中文字的颜色：取自网站版在两个主题下的实测值（浏览器默认的 ::selection）。 */
Color selectionColor(ThemeName theme) { return theme == ThemeName::Dark ? Color::rgba(3, 55, 161) : Color::rgba(50, 102, 208); }
constexpr Color kSelectedTextColor = Color::rgba(255, 255, 255);

}  // namespace

TextInput::TextInput() : editor_(std::make_unique<LineEditor>()) {}

TextInput::~TextInput() = default;

void TextInput::setValue(const std::string& value) {
  if (editor_->setText(toUtf16(value))) {
    scroll_ = 0;
    ensureCaretVisible();
    invalidate();
  }
}

std::string TextInput::value() const { return toUtf8(editor_->text()); }

void TextInput::setPlaceholder(std::string placeholder) {
  placeholder_ = std::move(placeholder);
  invalidate();
}

void TextInput::setInvalid(bool invalid) {
  invalid_ = invalid;
  invalidate();
}

void TextInput::setSearch(bool search) {
  search_ = search;
  invalidate();
}

Size TextInput::measure(Size available) { return Size{available.width > 0 ? available.width : kDefaultWidth, tokens::input_height}; }

Rect TextInput::contentRect() const {
  const Rect box = bounds();
  const double reserved = search_ && !editor_->text().empty() ? kClearReserved : 0.0;
  return Rect{box.x + kBorderWidth + tokens::input_padding_x, box.y + kBorderWidth, std::max(0.0, box.width - 2 * (kBorderWidth + tokens::input_padding_x) - reserved), tokens::input_field_height};
}

TextStyle TextInput::textStyle(Color color) const {
  TextStyle style;
  style.fontSize = tokens::input_font_size;
  style.fontWeight = 400;
  style.lineHeight = 0;
  style.color = color;
  return style;
}

std::u16string TextInput::displayText() const {
  if (!composing_) return editor_->text();
  std::u16string text = editor_->text();
  text.insert(editor_->caret(), composition_.text);
  return text;
}

std::size_t TextInput::displayCaret() const { return composing_ ? editor_->caret() + composition_.cursor : editor_->caret(); }

std::size_t TextInput::indexAt(double x) const {
  if (host() == nullptr) return 0;
  const Rect content = contentRect();
  const double local = x - content.x + scroll_;
  const std::u16string& text = editor_->text();
  return host()->caretIndexAt(text, textStyle(kTransparent), local);
}

std::optional<Rect> TextInput::clearButtonRect() const {
  // 和网页版一样：只在有内容、可用，且鼠标悬停或持有焦点时出现
  if (!search_ || !enabled() || editor_->text().empty() || !(hovered() || focused())) return std::nullopt;
  const Rect box = bounds();
  return Rect{box.right() - kClearCenterFromRight - kClearHitSize / 2.0, box.y + box.height / 2.0 - kClearHitSize / 2.0, kClearHitSize, kClearHitSize};
}

void TextInput::ensureCaretVisible() {
  if (host() == nullptr) return;
  const Rect content = contentRect();
  const std::u16string text = displayText();
  const double caretX = host()->caretOffset(text, textStyle(kTransparent), displayCaret());
  const double total = host()->measureText16(text, textStyle(kTransparent), 0).width;
  if (caretX - scroll_ < 0) scroll_ = caretX;
  else if (caretX - scroll_ > content.width - kCaretWidth) scroll_ = caretX - content.width + kCaretWidth;
  // 文字没填满输入区时不留空白滚动
  scroll_ = std::clamp(scroll_, 0.0, std::max(0.0, total + kCaretWidth - content.width));
}

void TextInput::notifyChanged() {
  if (onChange_) onChange_(value());
}

void TextInput::afterEdit(bool changed) {
  ensureCaretVisible();
  if (host() != nullptr) host()->restartCaretBlink();
  invalidate();
  if (changed) notifyChanged();
}

std::optional<Rect> TextInput::caretRect() const {
  if (host() == nullptr) return std::nullopt;
  const Rect content = contentRect();
  const TextStyle style = textStyle(kTransparent);
  const double lineHeight = host()->measureText16(u"A", style, 0).height;
  const double x = content.x - scroll_ + host()->caretOffset(displayText(), style, displayCaret());
  return Rect{x, content.y + (content.height - lineHeight) / 2.0, kCaretWidth, lineHeight};
}

void TextInput::paint(Canvas& canvas) {
  const auto& colors = themeTokens(host()->theme());
  const Rect box = bounds();
  const bool off = !enabled();
  Color border = colors.input_border;
  if (off) border = colors.disabled_border;
  else if (invalid_) border = colors.status_error;
  else if (focused()) border = colors.focus_border;
  else if (hovered()) border = colors.input_border_hover;
  canvas.drawBox(box, BoxStyle{.fill = off ? colors.disabled_bg : colors.input_bg, .radius = tokens::input_radius, .borderWidth = kBorderWidth, .borderColor = border});
  if (focused() && !off) {
    // :focus-within 的 outline: 1px solid; outline-offset: -1px，校验失败时换成错误色
    canvas.strokeOutline(box, tokens::input_radius, kFocusOffset, kFocusWidth, invalid_ ? colors.status_error : colors.focus_border);
  }

  const Rect content = contentRect();
  const std::u16string text = displayText();
  const Color textColor = off ? colors.disabled_text : colors.input_foreground;
  const TextStyle style = textStyle(textColor);
  const double lineHeight = host()->measureText16(u"A", style, 0).height;
  const double lineTop = content.y + (content.height - lineHeight) / 2.0;

  canvas.pushClip(content);
  if (text.empty()) {
    canvas.drawText(placeholder_, content, textStyle(off ? colors.disabled_text : colors.placeholder_color), VerticalAlign::Center);
  } else {
    const double originX = content.x - scroll_;
    const Rect textBox{originX, content.y, host()->measureText16(text, style, 0).width + 4, content.height};
    if (focused() && editor_->hasSelection() && !composing_) {
      const double from = originX + host()->caretOffset(text, style, editor_->selectionStart());
      const double to = originX + host()->caretOffset(text, style, editor_->selectionEnd());
      canvas.fillRectSmooth(Rect{from, lineTop, to - from, lineHeight}, selectionColor(host()->theme()));
      // 选中的文字用白色，选区两侧的文字仍用原色
      TextStyle selected = style;
      selected.color = kSelectedTextColor;
      const double far = textBox.right() + content.width;
      canvas.pushClip(Rect{content.x - 1, content.y, std::max(0.0, from - content.x + 1), content.height});
      canvas.drawText16(text, textBox, style, VerticalAlign::Center);
      canvas.popClip();
      canvas.pushClip(Rect{from, content.y, std::max(0.0, to - from), content.height});
      canvas.drawText16(text, textBox, selected, VerticalAlign::Center);
      canvas.popClip();
      canvas.pushClip(Rect{to, content.y, std::max(0.0, far - to), content.height});
      canvas.drawText16(text, textBox, style, VerticalAlign::Center);
      canvas.popClip();
    } else {
      canvas.drawText16(text, textBox, style, VerticalAlign::Center);
    }
    if (composing_) {
      // 组合串的下划线，正在转换的分句加粗
      const std::size_t start = editor_->caret();
      const std::size_t end = start + composition_.text.size();
      const double underlineY = lineTop + lineHeight - 1;
      const double from = host()->caretOffset(text, style, start);
      const double to = host()->caretOffset(text, style, end);
      canvas.fillRect(Rect{originX + from, underlineY, to - from, kComposeUnderline}, textColor);
      if (composition_.targetEnd > composition_.targetStart) {
        const double targetFrom = host()->caretOffset(text, style, start + composition_.targetStart);
        const double targetTo = host()->caretOffset(text, style, start + composition_.targetEnd);
        canvas.fillRect(Rect{originX + targetFrom, underlineY - 1, targetTo - targetFrom, kComposeTargetUnderline}, textColor);
      }
    }
  }
  if (focused() && !off && host()->caretOn() && !(editor_->hasSelection() && !composing_)) {
    const double caretX = content.x - scroll_ + host()->caretOffset(text, style, displayCaret());
    canvas.fillRect(Rect{caretX, lineTop, kCaretWidth, lineHeight}, colors.input_foreground);
  }
  canvas.popClip();
  if (const auto clear = clearButtonRect()) {
    const double cx = clear->x + clear->width / 2.0;
    const double cy = clear->y + clear->height / 2.0;
    const Color crossColor = host()->theme() == ThemeName::Dark ? Color::rgba(255, 255, 255) : Color::rgba(0, 0, 0);
    canvas.strokeLine(Point{cx - kClearArmX, cy - kClearArmY}, Point{cx + kClearArmX, cy + kClearArmY}, kClearLineWidth, crossColor);
    canvas.strokeLine(Point{cx - kClearArmX, cy + kClearArmY}, Point{cx + kClearArmX, cy - kClearArmY}, kClearLineWidth, crossColor);
  }
}

bool TextInput::onMouseDown(const MouseEvent& event) {
  if (event.button != MouseButton::Left) return false;
  if (const auto clear = clearButtonRect(); clear && clear->contains(event.position)) {
    // 点清除按钮：清空内容并保持焦点，不开始拖选
    editor_->selectAll();
    afterEdit(editor_->deleteSelection());
    return true;
  }
  const std::size_t index = indexAt(event.position.x);
  const bool extend = (event.modifiers & kModifierShift) != 0;
  if (event.clickCount == 2) editor_->selectWordAt(index);
  else if (event.clickCount >= 3) editor_->selectAll();
  else editor_->moveTo(index, extend);
  dragging_ = true;
  host()->captureMouse(this);
  afterEdit(false);
  return true;
}

bool TextInput::onMouseMove(const MouseEvent& event) {
  if (!dragging_) return false;
  editor_->moveTo(indexAt(event.position.x), true);
  afterEdit(false);
  return true;
}

bool TextInput::onMouseUp(const MouseEvent& event) {
  if (event.button != MouseButton::Left || !dragging_) return false;
  dragging_ = false;
  host()->releaseMouse(this);
  return true;
}

bool TextInput::onKeyDown(const KeyEvent& event) {
  if (!enabled()) return false;
  const bool ctrl = (event.modifiers & kModifierCtrl) != 0;
  const bool shift = (event.modifiers & kModifierShift) != 0;
  const bool alt = (event.modifiers & kModifierAlt) != 0;
  if (alt) return false;
  if (composing_) return event.virtualKey != 0x09;  // 组合中的按键交给输入法，只让 Tab 之类的穿透

  bool changed = false;
  switch (event.virtualKey) {
    case kVirtualKeyLeft:
      editor_->moveLeft(shift, ctrl);
      break;
    case kVirtualKeyRight:
      editor_->moveRight(shift, ctrl);
      break;
    case kVirtualKeyHome:
      editor_->moveToStart(shift);
      break;
    case kVirtualKeyEnd:
      editor_->moveToEnd(shift);
      break;
    case kVirtualKeyBack:
      changed = ctrl ? editor_->deleteWordBackward() : editor_->deleteBackward();
      break;
    case kVirtualKeyDelete:
      if (shift && editor_->hasSelection()) {
        host()->setClipboardText(toUtf8(editor_->selectedText()));
        changed = editor_->deleteSelection();
      } else {
        changed = ctrl ? editor_->deleteWordForward() : editor_->deleteForward();
      }
      break;
    case kVirtualKeyInsert:
      if (ctrl && editor_->hasSelection()) host()->setClipboardText(toUtf8(editor_->selectedText()));
      else if (shift) {
        if (const auto text = host()->clipboardText()) changed = editor_->insert(toUtf16(*text));
      } else return false;
      break;
    case kVirtualKeyReturn:
      if (onEnter_) onEnter_();
      return true;
    case 'A':
      if (!ctrl) return false;
      editor_->selectAll();
      break;
    case 'C':
      if (!ctrl) return false;
      if (editor_->hasSelection()) host()->setClipboardText(toUtf8(editor_->selectedText()));
      break;
    case 'X':
      if (!ctrl) return false;
      if (editor_->hasSelection()) {
        host()->setClipboardText(toUtf8(editor_->selectedText()));
        changed = editor_->deleteSelection();
      }
      break;
    case 'V':
      if (!ctrl) return false;
      if (const auto text = host()->clipboardText()) changed = editor_->insert(toUtf16(*text));
      break;
    case 'Z':
      if (!ctrl) return false;
      changed = shift ? editor_->redo() : editor_->undo();
      break;
    case 'Y':
      if (!ctrl) return false;
      changed = editor_->redo();
      break;
    default:
      return false;
  }
  afterEdit(changed);
  return true;
}

bool TextInput::onChar(char32_t codePoint) {
  if (!enabled() || codePoint < 0x20 || codePoint == 0x7F) return false;
  const bool changed = editor_->insert(toUtf16(codePoint));
  afterEdit(changed);
  return true;
}

void TextInput::onImeComposition(const ImeComposition& composition) {
  if (!enabled()) return;
  // 开始组合时选区被组合串替换
  if (!composing_ && editor_->hasSelection()) {
    editor_->deleteSelection();
    notifyChanged();
  }
  composing_ = !composition.text.empty();
  composition_ = composition;
  afterEdit(false);
}

void TextInput::onImeCommit(std::u16string_view text) {
  if (!enabled()) return;
  composing_ = false;
  composition_ = ImeComposition{};
  if (!text.empty() && editor_->hasSelection()) editor_->deleteSelection();
  const bool changed = editor_->insert(text);
  afterEdit(changed);
}

void TextInput::onImeEnd() {
  composing_ = false;
  composition_ = ImeComposition{};
  afterEdit(false);
}

void TextInput::onStateChanged() {
  // 用键盘（Tab）切到输入框时全选里面的文字，鼠标点进来则由点击位置决定光标
  if (!wasFocused_ && focusVisible()) {
    editor_->selectAll();
    ensureCaretVisible();
  }
  if (wasFocused_ && !focused()) {
    composing_ = false;
    dragging_ = false;
    scroll_ = 0;  // 失去焦点后网页版的输入框回到开头
    if (host() != nullptr) host()->releaseMouse(this);
    if (onBlur_) onBlur_();
  }
  wasFocused_ = focused();
  Widget::onStateChanged();
}

}  // namespace rujian::ui
