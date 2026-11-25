/*
 * ------------------------------------------------------------------------
 * 名称：line-editor.cpp
 * 说明：单行文字编辑器的实现。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-19
 * 备注：词的划分：空白、字母数字（含下划线与全部非 ASCII 的字母）、标点符号各算一类，连续的同类字符是一个词；汉字与假名每个字单独算一个词。
 * ------------------------------------------------------------------------
 */

#include "components/text-input/line-editor.hpp"

#include <algorithm>

#include "base/unicode.hpp"

namespace rujian::ui {

namespace {

/** 字符类别。 */
enum class CharClass { Space, Word, Punctuation, Ideograph };

bool isCombining(char32_t cp) {
  return (cp >= 0x0300 && cp <= 0x036F) || (cp >= 0x1AB0 && cp <= 0x1AFF) || (cp >= 0x1DC0 && cp <= 0x1DFF) || (cp >= 0x20D0 && cp <= 0x20FF) || (cp >= 0xFE00 && cp <= 0xFE0F) ||
         (cp >= 0xFE20 && cp <= 0xFE2F) || (cp >= 0xE0100 && cp <= 0xE01EF) || (cp >= 0x1F3FB && cp <= 0x1F3FF);
}

CharClass classOf(char32_t cp) {
  if (cp == ' ' || cp == '\t' || cp == 0xA0 || cp == 0x3000 || (cp >= 0x2000 && cp <= 0x200A)) return CharClass::Space;
  if ((cp >= 0x3040 && cp <= 0x30FF) || (cp >= 0x3400 && cp <= 0x4DBF) || (cp >= 0x4E00 && cp <= 0x9FFF) || (cp >= 0xF900 && cp <= 0xFAFF) || (cp >= 0x20000 && cp <= 0x2FFFF)) return CharClass::Ideograph;
  if (cp < 0x80) return ((cp >= '0' && cp <= '9') || (cp >= 'A' && cp <= 'Z') || (cp >= 'a' && cp <= 'z') || cp == '_') ? CharClass::Word : CharClass::Punctuation;
  if ((cp >= 0x2010 && cp <= 0x2027) || (cp >= 0x2030 && cp <= 0x205E) || (cp >= 0x3001 && cp <= 0x303F) || (cp >= 0xFF01 && cp <= 0xFF0F) || (cp >= 0xFF1A && cp <= 0xFF20) ||
      (cp >= 0xFF3B && cp <= 0xFF40) || (cp >= 0xFF5B && cp <= 0xFF65) || (cp >= 0xA1 && cp <= 0xBF && cp != 0xAA && cp != 0xB5 && cp != 0xBA)) {
    return CharClass::Punctuation;
  }
  return CharClass::Word;
}

/** 位置 index 之前一个码点的起点。 */
std::size_t previousCodePoint(std::u16string_view text, std::size_t index) {
  if (index == 0) return 0;
  --index;
  if (index > 0 && text[index] >= 0xDC00 && text[index] <= 0xDFFF && text[index - 1] >= 0xD800 && text[index - 1] <= 0xDBFF) --index;
  return index;
}

/** 去掉控制字符：换行、回车与制表符换成空格，其余控制字符丢弃。 */
std::u16string sanitize(std::u16string_view text) {
  std::u16string out;
  out.reserve(text.size());
  for (std::size_t i = 0; i < text.size(); ++i) {
    const char16_t unit = text[i];
    if (unit == u'\r' && i + 1 < text.size() && text[i + 1] == u'\n') continue;
    if (unit == u'\r' || unit == u'\n' || unit == u'\t') out.push_back(u' ');
    else if (unit >= 0x20 && unit != 0x7F && !(unit >= 0x80 && unit <= 0x9F)) out.push_back(unit);
  }
  return out;
}

}  // namespace

void LineEditor::clamp() {
  caret_ = std::min(caret_, text_.size());
  anchor_ = std::min(anchor_, text_.size());
}

std::size_t LineEditor::previousBoundary(std::size_t position) const {
  std::size_t i = previousCodePoint(text_, position);
  // 前面的字符是组合符号或连接符时，一起退到它所依附的字符
  while (i > 0 && (isCombining(codePointAt(text_, i)) || codePointAt(text_, previousCodePoint(text_, i)) == 0x200D)) i = previousCodePoint(text_, i);
  return i;
}

std::size_t LineEditor::nextBoundary(std::size_t position) const {
  if (position >= text_.size()) return text_.size();
  std::size_t i = position + codePointLength(text_, position);
  while (i < text_.size()) {
    const char32_t cp = codePointAt(text_, i);
    if (cp == 0x200D && i + 1 < text_.size()) i += 1 + codePointLength(text_, i + 1);
    else if (isCombining(cp)) i += codePointLength(text_, i);
    else break;
  }
  return i;
}

std::size_t LineEditor::nextWord(std::size_t position) const {
  const std::size_t size = text_.size();
  std::size_t i = std::min(position, size);
  if (i >= size) return size;
  const CharClass first = classOf(codePointAt(text_, i));
  if (first == CharClass::Ideograph) {
    i = nextBoundary(i);
  } else {
    while (i < size && classOf(codePointAt(text_, i)) == first) i = nextBoundary(i);
  }
  while (i < size && classOf(codePointAt(text_, i)) == CharClass::Space) i = nextBoundary(i);
  return i;
}

std::size_t LineEditor::previousWord(std::size_t position) const {
  std::size_t i = std::min(position, text_.size());
  while (i > 0 && classOf(codePointAt(text_, previousCodePoint(text_, i))) == CharClass::Space) i = previousBoundary(i);
  if (i == 0) return 0;
  const CharClass last = classOf(codePointAt(text_, previousCodePoint(text_, i)));
  if (last == CharClass::Ideograph) return previousBoundary(i);
  while (i > 0 && classOf(codePointAt(text_, previousCodePoint(text_, i))) == last) i = previousBoundary(i);
  return i;
}

void LineEditor::record(Kind kind) {
  const bool mergeable = kind == Kind::Typing || kind == Kind::DeleteBackward || kind == Kind::DeleteForward;
  if (!(mergeable && kind == lastKind_)) undoStack_.push_back(State{text_, caret_, anchor_});
  redoStack_.clear();
  lastKind_ = kind;
}

void LineEditor::replace(std::size_t start, std::size_t end, std::u16string_view replacement) {
  text_.replace(start, end - start, replacement);
  caret_ = start + replacement.size();
  anchor_ = caret_;
}

bool LineEditor::setText(std::u16string text) {
  if (text == text_) return false;
  text_ = std::move(text);
  caret_ = text_.size();
  anchor_ = caret_;
  undoStack_.clear();
  redoStack_.clear();
  lastKind_ = Kind::None;
  return true;
}

bool LineEditor::insert(std::u16string_view text) {
  const std::u16string clean = sanitize(text);
  if (clean.empty()) return false;
  const bool single = codePointLength(clean, 0) == clean.size();
  record(!hasSelection() && single ? Kind::Typing : Kind::Other);
  replace(selectionStart(), selectionEnd(), clean);
  return true;
}

bool LineEditor::deleteSelection() {
  if (!hasSelection()) return false;
  record(Kind::Other);
  replace(selectionStart(), selectionEnd(), u"");
  return true;
}

bool LineEditor::deleteBackward() {
  if (hasSelection()) return deleteSelection();
  if (caret_ == 0) return false;
  record(Kind::DeleteBackward);
  replace(previousBoundary(caret_), caret_, u"");
  return true;
}

bool LineEditor::deleteForward() {
  if (hasSelection()) return deleteSelection();
  if (caret_ >= text_.size()) return false;
  record(Kind::DeleteForward);
  replace(caret_, nextBoundary(caret_), u"");
  return true;
}

bool LineEditor::deleteWordBackward() {
  if (hasSelection()) return deleteSelection();
  if (caret_ == 0) return false;
  record(Kind::Other);
  replace(previousWord(caret_), caret_, u"");
  return true;
}

bool LineEditor::deleteWordForward() {
  if (hasSelection()) return deleteSelection();
  if (caret_ >= text_.size()) return false;
  record(Kind::Other);
  replace(caret_, nextWord(caret_), u"");
  return true;
}

void LineEditor::moveLeft(bool extend, bool byWord) {
  lastKind_ = Kind::None;
  if (!extend && hasSelection()) {
    caret_ = anchor_ = selectionStart();
    return;
  }
  caret_ = byWord ? previousWord(caret_) : previousBoundary(caret_);
  if (!extend) anchor_ = caret_;
}

void LineEditor::moveRight(bool extend, bool byWord) {
  lastKind_ = Kind::None;
  if (!extend && hasSelection()) {
    caret_ = anchor_ = selectionEnd();
    return;
  }
  caret_ = byWord ? nextWord(caret_) : nextBoundary(caret_);
  if (!extend) anchor_ = caret_;
}

void LineEditor::moveToStart(bool extend) { moveTo(0, extend); }

void LineEditor::moveToEnd(bool extend) { moveTo(text_.size(), extend); }

void LineEditor::moveTo(std::size_t position, bool extend) {
  lastKind_ = Kind::None;
  position = std::min(position, text_.size());
  // 对齐到字符边界：落在代理对或组合符号中间时取它前面最近的边界
  std::size_t boundary = 0;
  while (boundary < position) {
    const std::size_t next = nextBoundary(boundary);
    if (next > position) break;
    boundary = next;
  }
  position = boundary;
  caret_ = position;
  if (!extend) anchor_ = caret_;
}

void LineEditor::selectAll() {
  lastKind_ = Kind::None;
  anchor_ = 0;
  caret_ = text_.size();
}

void LineEditor::selectRange(std::size_t start, std::size_t end) {
  lastKind_ = Kind::None;
  anchor_ = std::min(start, text_.size());
  caret_ = std::min(end, text_.size());
}

void LineEditor::selectWordAt(std::size_t position) {
  lastKind_ = Kind::None;
  if (text_.empty()) return;
  const std::size_t index = position < text_.size() ? std::min(position, text_.size()) : previousBoundary(text_.size());
  const CharClass kind = classOf(codePointAt(text_, index));
  std::size_t start = index;
  std::size_t end = nextBoundary(index);
  if (kind != CharClass::Ideograph) {
    while (start > 0 && classOf(codePointAt(text_, previousCodePoint(text_, start))) == kind) start = previousBoundary(start);
    while (end < text_.size() && classOf(codePointAt(text_, end)) == kind) end = nextBoundary(end);
  }
  anchor_ = start;
  caret_ = end;
}

bool LineEditor::undo() {
  if (undoStack_.empty()) return false;
  redoStack_.push_back(State{text_, caret_, anchor_});
  State state = std::move(undoStack_.back());
  undoStack_.pop_back();
  text_ = std::move(state.text);
  caret_ = state.caret;
  anchor_ = state.anchor;
  lastKind_ = Kind::None;
  clamp();
  return true;
}

bool LineEditor::redo() {
  if (redoStack_.empty()) return false;
  undoStack_.push_back(State{text_, caret_, anchor_});
  State state = std::move(redoStack_.back());
  redoStack_.pop_back();
  text_ = std::move(state.text);
  caret_ = state.caret;
  anchor_ = state.anchor;
  lastKind_ = Kind::None;
  clamp();
  return true;
}

}  // namespace rujian::ui
