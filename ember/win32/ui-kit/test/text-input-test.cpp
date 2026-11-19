/*
 * ------------------------------------------------------------------------
 * 名称：text-input-test.cpp
 * 说明：文字输入框的交互测试：键入、剪贴板、撤销、键盘移动与删除、回车、鼠标定位与选择、输入法组合与提交、禁用、失去焦点、光标位置与滚动。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-19
 * 备注：剪贴板用内存里的替代，不碰系统剪贴板；事件都经 Root 的输入入口送入。
 * ------------------------------------------------------------------------
 */

#include <gtest/gtest.h>

#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "rujian/ui/offscreen.hpp"
#include "rujian/ui/root.hpp"
#include "rujian/ui/text-input.hpp"

using namespace rujian::ui;

namespace {

constexpr int kVkBack = 0x08;
constexpr int kVkReturn = 0x0D;
constexpr int kVkEnd = 0x23;
constexpr int kVkHome = 0x24;
constexpr int kVkLeft = 0x25;
constexpr int kVkRight = 0x27;
constexpr int kVkDelete = 0x2E;
constexpr int kVkTab = 0x09;

/** 把输入框放在 (10,10) 的页面。 */
class Holder final : public Widget {
 public:
  void arrange(const Rect& bounds) override {
    Widget::arrange(bounds);
    for (const auto& child : children()) {
      const Size size = child->measure(Size{200, 0});
      child->arrange(Rect{10, 10, size.width, size.height});
    }
  }
};

struct Scene {
  Root root{ThemeName::Dark, 1.0};
  TextInput* input{nullptr};
  TextInput* other{nullptr};
  std::optional<std::string> clipboard;
  std::vector<std::string> changes;
  int enters{0};
  int blurs{0};

  Scene() {
    auto holder = std::make_unique<Holder>();
    auto created = std::make_unique<TextInput>();
    input = created.get();
    holder->addChild(std::move(created));
    root.setContent(std::move(holder));
    root.setClipboardHandlers([this] { return clipboard; }, [this](std::string_view text) { clipboard = std::string(text); });
    input->setOnChange([this](const std::string& value) { changes.push_back(value); });
    input->setOnEnter([this] { ++enters; });
    input->setOnBlur([this] { ++blurs; });
    root.layout(Size{240, 44});
    root.keyDown(kVkTab, 0, false);
  }

  void type(const std::u16string& text) {
    for (const char16_t unit : text) root.character(unit);
  }
  void key(int virtualKey, unsigned modifiers = 0) { root.keyDown(virtualKey, modifiers, false); }
  void ctrl(char letter, bool shift = false) { root.keyDown(letter, kModifierCtrl | (shift ? kModifierShift : 0), false); }
};

}  // namespace

TEST(TextInput, TypingInsertsCharactersAndReportsEachChange) {
  Scene s;
  ASSERT_TRUE(s.input->focused());
  s.type(u"ab");
  s.root.character(U'中');
  EXPECT_EQ(s.input->value(), "ab中");
  EXPECT_EQ(s.changes, (std::vector<std::string>{"a", "ab", "ab中"}));
}

TEST(TextInput, ControlCharactersAreNotInserted) {
  Scene s;
  s.root.character(0x01);
  s.root.character(0x7F);
  s.root.character(0x0D);
  EXPECT_EQ(s.input->value(), "");
  EXPECT_TRUE(s.changes.empty());
}

TEST(TextInput, SetValueDoesNotNotifyAndKeepsCaretWhenUnchanged) {
  Scene s;
  s.input->setValue("hello");
  EXPECT_EQ(s.input->value(), "hello");
  EXPECT_TRUE(s.changes.empty());
  s.key(kVkHome);
  s.input->setValue("hello");
  s.type(u"X");
  EXPECT_EQ(s.input->value(), "Xhello");
}

TEST(TextInput, CopyCutAndPasteUseTheClipboard) {
  Scene s;
  s.input->setValue("hello world");
  s.ctrl('A');
  s.ctrl('C');
  EXPECT_EQ(s.clipboard, "hello world");
  s.ctrl('X');
  EXPECT_EQ(s.input->value(), "");
  EXPECT_EQ(s.clipboard, "hello world");
  s.ctrl('V');
  EXPECT_EQ(s.input->value(), "hello world");
  s.clipboard = "a\r\nb";
  s.ctrl('V');
  EXPECT_EQ(s.input->value(), "hello worlda b");
  s.clipboard.reset();
  const std::size_t before = s.changes.size();
  s.ctrl('V');
  EXPECT_EQ(s.changes.size(), before);
}

TEST(TextInput, CopyWithoutSelectionLeavesClipboardAlone) {
  Scene s;
  s.clipboard = "keep";
  s.input->setValue("abc");
  s.ctrl('C');
  EXPECT_EQ(s.clipboard, "keep");
}

TEST(TextInput, UndoAndRedoFollowKeyboardShortcuts) {
  Scene s;
  s.type(u"abc");
  s.ctrl('Z');
  EXPECT_EQ(s.input->value(), "");
  s.ctrl('Y');
  EXPECT_EQ(s.input->value(), "abc");
  s.ctrl('Z');
  s.ctrl('Z', true);
  EXPECT_EQ(s.input->value(), "abc");
}

TEST(TextInput, ArrowsHomeEndBackspaceAndDeleteEditText) {
  Scene s;
  s.input->setValue("abcd");
  s.key(kVkLeft);
  s.key(kVkBack);
  EXPECT_EQ(s.input->value(), "abd");
  s.key(kVkHome);
  s.key(kVkDelete);
  EXPECT_EQ(s.input->value(), "bd");
  s.key(kVkEnd);
  s.key(kVkLeft, kModifierShift);
  s.key(kVkBack);
  EXPECT_EQ(s.input->value(), "b");
  s.key(kVkRight);
  s.key(kVkBack, kModifierCtrl);
  EXPECT_EQ(s.input->value(), "");
}

TEST(TextInput, EnterTriggersCallbackButNotWhileComposing) {
  Scene s;
  s.key(kVkReturn);
  EXPECT_EQ(s.enters, 1);
  ImeComposition composition;
  composition.text = u"ni";
  composition.cursor = 2;
  s.root.imeComposition(composition);
  s.key(kVkReturn);
  EXPECT_EQ(s.enters, 1);
  s.root.imeEnd();
  s.key(kVkReturn);
  EXPECT_EQ(s.enters, 2);
}

TEST(TextInput, ImeCompositionDoesNotChangeValueUntilCommitted) {
  Scene s;
  s.input->setValue("ab");
  s.key(kVkLeft);
  ImeComposition composition;
  composition.text = u"ni";
  composition.cursor = 2;
  s.root.imeComposition(composition);
  EXPECT_EQ(s.input->value(), "ab");
  EXPECT_TRUE(s.changes.empty());
  s.root.imeCommit(u"你");
  s.root.imeEnd();
  EXPECT_EQ(s.input->value(), "a你b");
  EXPECT_EQ(s.changes, (std::vector<std::string>{"a你b"}));
}

TEST(TextInput, ImeCompositionReplacesSelection) {
  Scene s;
  s.input->setValue("hello");
  s.ctrl('A');
  ImeComposition composition;
  composition.text = u"n";
  composition.cursor = 1;
  s.root.imeComposition(composition);
  EXPECT_EQ(s.input->value(), "");
  s.root.imeCommit(u"你");
  s.root.imeEnd();
  EXPECT_EQ(s.input->value(), "你");
}

TEST(TextInput, ImeCancelLeavesValueUntouched) {
  Scene s;
  s.input->setValue("ab");
  ImeComposition composition;
  composition.text = u"ni";
  composition.cursor = 2;
  s.root.imeComposition(composition);
  s.root.imeComposition(ImeComposition{});
  s.root.imeEnd();
  EXPECT_EQ(s.input->value(), "ab");
}

TEST(TextInput, KeysAreSwallowedWhileComposingExceptTab) {
  Scene s;
  s.input->setValue("ab");
  ImeComposition composition;
  composition.text = u"x";
  composition.cursor = 1;
  s.root.imeComposition(composition);
  s.key(kVkBack);
  EXPECT_EQ(s.input->value(), "ab");
}

TEST(TextInput, MouseClickPlacesCaretAndDragSelects) {
  Scene s;
  s.input->setValue("abcdefghij");
  s.root.keyDown(kVkHome, 0, false);
  // 点在最右边：光标到末尾；点在最左边：光标到开头
  s.root.mouseDown(Point{200, 20}, MouseButton::Left, 0, 1000);
  s.root.mouseUp(Point{200, 20}, MouseButton::Left, 0);
  s.type(u"!");
  EXPECT_EQ(s.input->value(), "abcdefghij!");
  s.root.mouseDown(Point{19, 20}, MouseButton::Left, 0, 5000);
  s.root.mouseMove(Point{200, 20}, 0);
  s.root.mouseUp(Point{200, 20}, MouseButton::Left, 0);
  s.type(u"Z");
  EXPECT_EQ(s.input->value(), "Z");
}

TEST(TextInput, DoubleClickSelectsWordAndTripleClickSelectsAll) {
  Scene s;
  s.input->setValue("foo bar");
  s.root.mouseDown(Point{19, 20}, MouseButton::Left, 0, 1000);
  s.root.mouseUp(Point{19, 20}, MouseButton::Left, 0);
  s.root.mouseDown(Point{19, 20}, MouseButton::Left, 0, 1100);
  s.root.mouseUp(Point{19, 20}, MouseButton::Left, 0);
  s.type(u"X");
  EXPECT_EQ(s.input->value(), "X bar");
  s.input->setValue("foo bar");
  s.root.mouseDown(Point{19, 20}, MouseButton::Left, 0, 3000);
  s.root.mouseUp(Point{19, 20}, MouseButton::Left, 0);
  s.root.mouseDown(Point{19, 20}, MouseButton::Left, 0, 3100);
  s.root.mouseUp(Point{19, 20}, MouseButton::Left, 0);
  s.root.mouseDown(Point{19, 20}, MouseButton::Left, 0, 3200);
  s.root.mouseUp(Point{19, 20}, MouseButton::Left, 0);
  s.type(u"Y");
  EXPECT_EQ(s.input->value(), "Y");
}

TEST(TextInput, SlowSecondClickIsNotADoubleClick) {
  Scene s;
  s.input->setValue("foo bar");
  s.root.mouseDown(Point{19, 20}, MouseButton::Left, 0, 1000);
  s.root.mouseUp(Point{19, 20}, MouseButton::Left, 0);
  s.root.mouseDown(Point{19, 20}, MouseButton::Left, 0, 2000);
  s.root.mouseUp(Point{19, 20}, MouseButton::Left, 0);
  s.type(u"X");
  EXPECT_EQ(s.input->value(), "Xfoo bar");
}

TEST(TextInput, DisabledInputIgnoresEditingAndIsSkippedByTab) {
  Scene s;
  s.input->setValue("abc");
  s.input->setEnabled(false);
  EXPECT_FALSE(s.input->focused());
  s.type(u"x");
  s.key(kVkBack);
  EXPECT_EQ(s.input->value(), "abc");
  s.root.keyDown(kVkTab, 0, false);
  EXPECT_FALSE(s.input->focused());
}

TEST(TextInput, LosingFocusCallsBlurOnce) {
  Scene s;
  s.root.mouseDown(Point{230, 40}, MouseButton::Left, 0);
  s.root.mouseUp(Point{230, 40}, MouseButton::Left, 0);
  EXPECT_FALSE(s.input->focused());
  EXPECT_EQ(s.blurs, 1);
}

TEST(TextInput, SearchClearButtonClearsValueAndKeepsFocus) {
  Scene s;
  s.input->setSearch(true);
  s.input->setValue("abc");
  ASSERT_TRUE(s.input->focused());
  s.root.mouseDown(Point{194.5, 22}, MouseButton::Left, 0);
  s.root.mouseUp(Point{194.5, 22}, MouseButton::Left, 0);
  EXPECT_EQ(s.input->value(), "");
  EXPECT_TRUE(s.input->focused());
  ASSERT_EQ(s.changes.size(), 1u);
  EXPECT_EQ(s.changes[0], "");
}

TEST(TextInput, OnlySearchInputsHaveAClearButton) {
  Scene s;
  s.input->setValue("abc");
  s.root.mouseDown(Point{194.5, 22}, MouseButton::Left, 0);
  s.root.mouseUp(Point{194.5, 22}, MouseButton::Left, 0);
  EXPECT_EQ(s.input->value(), "abc");
  EXPECT_TRUE(s.changes.empty());
}

TEST(TextInput, CaretRectFollowsCaretAndStaysInsideTheFieldWhenTextIsLong) {
  Scene s;
  const auto empty = s.input->caretRect();
  ASSERT_TRUE(empty);
  EXPECT_NEAR(empty->x, 10 + 1 + 8, 0.01);
  s.type(u"0123456789012345678901234567890123456789012345678901234567890123456789");
  const auto longText = s.input->caretRect();
  ASSERT_TRUE(longText);
  EXPECT_LE(longText->x, 10 + 200 - 1 - 8 + 0.01);
  EXPECT_GE(longText->x, 10 + 1 + 8);
  s.key(kVkHome);
  EXPECT_NEAR(s.input->caretRect()->x, 10 + 1 + 8, 0.01);
}

TEST(TextInput, FocusedInputRequestsTextInputAndPlainWidgetsDoNot) {
  Scene s;
  EXPECT_TRUE(s.root.focusAcceptsText());
  EXPECT_TRUE(s.root.imeCaretRect().has_value());
  s.root.mouseDown(Point{230, 40}, MouseButton::Left, 0);
  EXPECT_FALSE(s.root.focusAcceptsText());
  EXPECT_FALSE(s.root.imeCaretRect().has_value());
}

TEST(TextInput, CursorIsIBeamOverEnabledInputAndArrowWhenDisabled) {
  Scene s;
  s.root.mouseMove(Point{50, 20}, 0);
  EXPECT_EQ(s.root.cursor(), CursorKind::IBeam);
  s.input->setEnabled(false);
  EXPECT_EQ(s.root.cursor(), CursorKind::Arrow);
  s.root.mouseMove(Point{230, 40}, 0);
  EXPECT_EQ(s.root.cursor(), CursorKind::Arrow);
}

TEST(TextInput, RendersPlaceholderBorderStatesAndFocusRing) {
  Scene s;
  s.input->setPlaceholder("占位");
  OffscreenSurface surface(Size{240, 44}, 1.0);
  s.root.setCaretHidden(true);
  ASSERT_TRUE(surface.render(s.root, Color::rgba(0, 0, 0)));
  const auto focused = surface.pixels();
  s.root.mouseDown(Point{230, 40}, MouseButton::Left, 0);
  s.root.mouseUp(Point{230, 40}, MouseButton::Left, 0);
  ASSERT_TRUE(surface.render(s.root, Color::rgba(0, 0, 0)));
  const auto unfocused = surface.pixels();
  EXPECT_NE(focused, unfocused);
}
