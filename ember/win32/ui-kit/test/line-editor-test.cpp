/*
 * ------------------------------------------------------------------------
 * 名称：line-editor-test.cpp
 * 说明：单行编辑器与 UTF 转换的测试：插入与删除、选区、按字符与按词移动、词选择、撤销与重做的合并规则、代理对与组合符号。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-19
 * 备注：纯逻辑测试，不依赖字体与窗口。
 * ------------------------------------------------------------------------
 */

#include <gtest/gtest.h>

#include "base/unicode.hpp"
#include "components/text-input/line-editor.hpp"

using rujian::ui::LineEditor;
using rujian::ui::toUtf16;
using rujian::ui::toUtf8;

namespace {

/** 建一个带文字的编辑器，光标在末尾。 */
LineEditor make(const std::u16string& text) {
  LineEditor editor;
  editor.setText(text);
  return editor;
}

}  // namespace

TEST(Unicode, RoundTripsAsciiCjkAndSupplementaryCharacters) {
  const std::string text = "abc 中文 😀 é";
  EXPECT_EQ(toUtf8(toUtf16(text)), text);
  EXPECT_EQ(toUtf16("😀").size(), 2u);
  EXPECT_EQ(toUtf16(U'中'), u"中");
}

TEST(Unicode, InvalidInputBecomesReplacementCharacter) {
  EXPECT_EQ(toUtf16(std::string_view("a\xFFz")), u"a\uFFFDz");
  EXPECT_EQ(toUtf16(std::string_view("\xE4\xB8")), u"\uFFFD\uFFFD");
  EXPECT_EQ(toUtf16(std::string_view("\xC0\x80")), u"\uFFFD\uFFFD");
  EXPECT_EQ(toUtf8(std::u16string(1, char16_t(0xD800))), "\xEF\xBF\xBD");
  EXPECT_EQ(toUtf16(char32_t(0xD800)), u"\uFFFD");
}

TEST(LineEditor, InsertsAtCaretAndReplacesSelection) {
  LineEditor editor = make(u"hello");
  editor.moveTo(2, false);
  EXPECT_TRUE(editor.insert(u"XY"));
  EXPECT_EQ(editor.text(), u"heXYllo");
  EXPECT_EQ(editor.caret(), 4u);
  editor.moveTo(1, false);
  editor.moveTo(5, true);
  editor.insert(u"-");
  EXPECT_EQ(editor.text(), u"h-lo");
  EXPECT_EQ(editor.caret(), 2u);
  EXPECT_FALSE(editor.hasSelection());
}

TEST(LineEditor, InsertSanitizesControlCharacters) {
  LineEditor editor;
  editor.insert(u"a\r\nb\tc\x01\x7F");
  EXPECT_EQ(editor.text(), u"a b c");
  EXPECT_FALSE(editor.insert(u"\x01\x02"));
}

TEST(LineEditor, DeleteBackwardAndForwardRemoveOneCharacterOrSelection) {
  LineEditor editor = make(u"abcd");
  editor.moveTo(2, false);
  EXPECT_TRUE(editor.deleteBackward());
  EXPECT_EQ(editor.text(), u"acd");
  EXPECT_TRUE(editor.deleteForward());
  EXPECT_EQ(editor.text(), u"ad");
  editor.selectAll();
  EXPECT_TRUE(editor.deleteBackward());
  EXPECT_EQ(editor.text(), u"");
  EXPECT_FALSE(editor.deleteBackward());
  EXPECT_FALSE(editor.deleteForward());
}

TEST(LineEditor, SupplementaryCharactersAreOneUnit) {
  LineEditor editor = make(u"a😀b");
  editor.moveLeft(false, false);
  editor.moveLeft(false, false);
  EXPECT_EQ(editor.caret(), 1u);
  editor.moveRight(false, false);
  EXPECT_EQ(editor.caret(), 3u);
  editor.deleteBackward();
  EXPECT_EQ(editor.text(), u"ab");
}

TEST(LineEditor, CombiningMarksStayWithTheirBaseCharacter) {
  LineEditor editor = make(u"ae\u0301z");
  editor.moveLeft(false, false);
  editor.moveLeft(false, false);
  EXPECT_EQ(editor.caret(), 1u);
  editor.moveRight(false, false);
  EXPECT_EQ(editor.caret(), 3u);
  editor.moveTo(2, false);
  EXPECT_EQ(editor.caret(), 1u);
}

TEST(LineEditor, ArrowKeysCollapseSelectionToItsEdges) {
  LineEditor editor = make(u"abcdef");
  editor.moveTo(2, false);
  editor.moveTo(4, true);
  editor.moveLeft(false, false);
  EXPECT_EQ(editor.caret(), 2u);
  EXPECT_FALSE(editor.hasSelection());
  editor.moveTo(4, true);
  editor.moveRight(false, false);
  EXPECT_EQ(editor.caret(), 4u);
  EXPECT_FALSE(editor.hasSelection());
}

TEST(LineEditor, ShiftMovesExtendSelectionKeepingTheAnchor) {
  LineEditor editor = make(u"abcdef");
  editor.moveTo(3, false);
  editor.moveRight(true, false);
  editor.moveRight(true, false);
  EXPECT_EQ(editor.selectedText(), u"de");
  editor.moveLeft(true, false);
  editor.moveLeft(true, false);
  editor.moveLeft(true, false);
  EXPECT_EQ(editor.selectedText(), u"c");
  EXPECT_EQ(editor.anchor(), 3u);
  EXPECT_EQ(editor.caret(), 2u);
}

TEST(LineEditor, HomeAndEndMoveAndExtend) {
  LineEditor editor = make(u"abc");
  editor.moveToStart(false);
  EXPECT_EQ(editor.caret(), 0u);
  editor.moveToEnd(true);
  EXPECT_EQ(editor.selectedText(), u"abc");
}

TEST(LineEditor, WordMovementSkipsRunsAndFollowingSpaces) {
  LineEditor editor = make(u"foo  bar_baz, qux");
  editor.moveToStart(false);
  editor.moveRight(false, true);
  EXPECT_EQ(editor.caret(), 5u);
  editor.moveRight(false, true);
  EXPECT_EQ(editor.caret(), 12u);
  editor.moveRight(false, true);
  EXPECT_EQ(editor.caret(), 14u);
  editor.moveRight(false, true);
  EXPECT_EQ(editor.caret(), 17u);
  editor.moveLeft(false, true);
  EXPECT_EQ(editor.caret(), 14u);
  editor.moveLeft(false, true);
  EXPECT_EQ(editor.caret(), 12u);
  editor.moveLeft(false, true);
  EXPECT_EQ(editor.caret(), 5u);
  editor.moveLeft(false, true);
  EXPECT_EQ(editor.caret(), 0u);
}

TEST(LineEditor, EachIdeographIsItsOwnWord) {
  LineEditor editor = make(u"中文abc");
  editor.moveToStart(false);
  editor.moveRight(false, true);
  EXPECT_EQ(editor.caret(), 1u);
  editor.moveRight(false, true);
  EXPECT_EQ(editor.caret(), 2u);
  editor.moveToEnd(false);
  editor.moveLeft(false, true);
  EXPECT_EQ(editor.caret(), 2u);
  editor.moveLeft(false, true);
  EXPECT_EQ(editor.caret(), 1u);
}

TEST(LineEditor, DeleteWordRemovesToWordBoundary) {
  LineEditor editor = make(u"foo bar baz");
  editor.deleteWordBackward();
  EXPECT_EQ(editor.text(), u"foo bar ");
  editor.deleteWordBackward();
  EXPECT_EQ(editor.text(), u"foo ");
  editor.moveToStart(false);
  editor.deleteWordForward();
  EXPECT_EQ(editor.text(), u"");
  EXPECT_FALSE(editor.deleteWordForward());
}

TEST(LineEditor, SelectWordAtSelectsRunOfSameClass) {
  LineEditor editor = make(u"foo  bar,baz");
  editor.selectWordAt(1);
  EXPECT_EQ(editor.selectedText(), u"foo");
  editor.selectWordAt(4);
  EXPECT_EQ(editor.selectedText(), u"  ");
  editor.selectWordAt(8);
  EXPECT_EQ(editor.selectedText(), u",");
  editor.selectWordAt(100);
  EXPECT_EQ(editor.selectedText(), u"baz");
  LineEditor cjk = make(u"中文");
  cjk.selectWordAt(0);
  EXPECT_EQ(cjk.selectedText(), u"中");
  LineEditor empty;
  empty.selectWordAt(0);
  EXPECT_FALSE(empty.hasSelection());
}

TEST(LineEditor, TypingRunsMergeIntoOneUndoStep) {
  LineEditor editor;
  editor.insert(u"a");
  editor.insert(u"b");
  editor.insert(u"c");
  EXPECT_TRUE(editor.undo());
  EXPECT_EQ(editor.text(), u"");
  EXPECT_FALSE(editor.undo());
  EXPECT_TRUE(editor.redo());
  EXPECT_EQ(editor.text(), u"abc");
  EXPECT_EQ(editor.caret(), 3u);
}

TEST(LineEditor, MovingTheCaretStartsANewUndoStep) {
  LineEditor editor;
  editor.insert(u"a");
  editor.insert(u"b");
  editor.moveLeft(false, false);
  editor.insert(u"X");
  EXPECT_TRUE(editor.undo());
  EXPECT_EQ(editor.text(), u"ab");
  EXPECT_TRUE(editor.undo());
  EXPECT_EQ(editor.text(), u"");
}

TEST(LineEditor, ConsecutiveBackspacesMergeButDifferentKindsDoNot) {
  LineEditor editor = make(u"abcdef");
  editor.deleteBackward();
  editor.deleteBackward();
  editor.deleteForward();
  editor.moveToEnd(false);
  editor.deleteBackward();
  EXPECT_EQ(editor.text(), u"abc");
  editor.undo();
  EXPECT_EQ(editor.text(), u"abcd");
  editor.undo();
  EXPECT_EQ(editor.text(), u"abcdef");
}

TEST(LineEditor, ReplacingSelectionAndPasteAreSeparateSteps) {
  LineEditor editor = make(u"hello");
  editor.selectAll();
  editor.insert(u"world");
  editor.insert(u"!");
  EXPECT_TRUE(editor.undo());
  EXPECT_EQ(editor.text(), u"world");
  EXPECT_TRUE(editor.undo());
  EXPECT_EQ(editor.text(), u"hello");
  EXPECT_EQ(editor.selectedText(), u"hello");
}

TEST(LineEditor, NewEditClearsRedoStack) {
  LineEditor editor;
  editor.insert(u"a");
  editor.undo();
  EXPECT_TRUE(editor.canRedo());
  editor.insert(u"b");
  EXPECT_FALSE(editor.canRedo());
  EXPECT_FALSE(editor.redo());
}

TEST(LineEditor, SetTextMovesCaretToEndAndResetsHistoryOnlyWhenChanged) {
  LineEditor editor;
  editor.insert(u"abc");
  editor.moveTo(1, false);
  EXPECT_FALSE(editor.setText(u"abc"));
  EXPECT_EQ(editor.caret(), 1u);
  EXPECT_TRUE(editor.canUndo());
  EXPECT_TRUE(editor.setText(u"xyz"));
  EXPECT_EQ(editor.caret(), 3u);
  EXPECT_FALSE(editor.canUndo());
}

TEST(LineEditor, MoveToClampsAndSnapsToCharacterBoundaries) {
  LineEditor editor = make(u"a😀b");
  editor.moveTo(100, false);
  EXPECT_EQ(editor.caret(), 4u);
  editor.moveTo(2, false);
  EXPECT_EQ(editor.caret(), 1u);
}
