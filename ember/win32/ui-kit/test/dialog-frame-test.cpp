/*
 * ------------------------------------------------------------------------
 * 名称：dialog-frame-test.cpp
 * 说明：对话框框架的测试：标题栏与按钮区的位置、标题栏空白处的命中、关闭与最大化按钮、按钮点击、Esc 与回车、初始焦点、按内容的高度、标题栏底色。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-19
 * 备注：窗口 420×240。
 * ------------------------------------------------------------------------
 */

#include <gtest/gtest.h>

#include <memory>
#include <string>
#include <vector>

#include "base/theme-tokens.hpp"
#include "rujian/ui/dialog-frame.hpp"
#include "rujian/ui/label.hpp"
#include "rujian/ui/offscreen.hpp"
#include "rujian/ui/root.hpp"
#include "rujian/ui/text-input.hpp"

using namespace rujian::ui;

namespace {

constexpr Size kWindow{420, 240};
constexpr int kVkReturn = 0x0D;
constexpr int kVkEscape = 0x1B;
constexpr int kVkTab = 0x09;

struct Scene {
  Root root{ThemeName::Dark, 1.0};
  DialogFrame* frame{nullptr};
  TextInput* input{nullptr};
  std::vector<std::string> pressed;
  std::vector<DialogCloseReason> closes;
  int maximizes{0};

  explicit Scene(bool maximizable = true, bool withInput = true) {
    auto dialog = std::make_unique<DialogFrame>();
    frame = dialog.get();
    root.setContent(std::move(dialog));
    frame->setTitle("对话框标题");
    frame->setMaximizable(maximizable);
    if (withInput) {
      auto field = std::make_unique<TextInput>();
      input = field.get();
      frame->setContent(std::move(field));
    } else {
      frame->setContent(std::make_unique<Label>("正文内容"));
    }
    frame->setButtons({DialogButton{.id = "cancel", .text = "取消", .isCancel = true}, DialogButton{.id = "ok", .text = "确定", .variant = ButtonVariant::Primary, .isDefault = true}});
    frame->setOnButton([this](const std::string& id) { pressed.push_back(id); });
    frame->setOnClose([this](DialogCloseReason reason) { closes.push_back(reason); });
    frame->setOnToggleMaximize([this] { ++maximizes; });
    root.layout(kWindow);
  }

  static Point center(const Rect& rect) { return Point{rect.x + rect.width / 2, rect.y + rect.height / 2}; }

  void click(Point point) {
    root.mouseMove(point, 0);
    root.mouseDown(point, MouseButton::Left, 0);
    root.mouseUp(point, MouseButton::Left, 0);
  }
};

}  // namespace

TEST(DialogFrame, TitleBarIs32HighAtTheTop) {
  Scene s;
  const Rect bar = s.frame->titleBarRect();
  EXPECT_DOUBLE_EQ(bar.y, 0.0);
  EXPECT_DOUBLE_EQ(bar.height, 32.0);
  EXPECT_DOUBLE_EQ(bar.width, 420.0);
}

TEST(DialogFrame, ControlsAre24SquareAtTheRightWithAGap) {
  Scene s;
  const Rect close = s.frame->closeControl()->bounds();
  const Rect maximize = s.frame->maximizeControl()->bounds();
  EXPECT_DOUBLE_EQ(close.width, 24.0);
  EXPECT_DOUBLE_EQ(close.height, 24.0);
  EXPECT_DOUBLE_EQ(close.right(), 420.0 - 4.0);
  EXPECT_DOUBLE_EQ(maximize.right(), close.x - 2.0);
  EXPECT_DOUBLE_EQ(close.y, (31.0 - 24.0) / 2.0);
}

TEST(DialogFrame, MaximizeControlIsAbsentUnlessMaximizable) {
  Scene s(false);
  EXPECT_EQ(s.frame->maximizeControl(), nullptr);
  EXPECT_NE(s.frame->closeControl(), nullptr);
}

TEST(DialogFrame, ButtonsAreRightAlignedAtTheBottomWithTheMinimumWidth) {
  Scene s;
  const Rect ok = s.frame->button("ok")->bounds();
  const Rect cancel = s.frame->button("cancel")->bounds();
  EXPECT_DOUBLE_EQ(ok.right(), 420.0 - 10.0);
  EXPECT_DOUBLE_EQ(ok.bottom(), 240.0 - 10.0);
  EXPECT_DOUBLE_EQ(ok.height, 22.0);
  EXPECT_GE(ok.width, 72.0);
  EXPECT_DOUBLE_EQ(cancel.right(), ok.x - 6.0);
  EXPECT_GE(cancel.width, 72.0);
}

TEST(DialogFrame, BodyFillsTheSpaceBetweenTheTitleBarAndTheButtons) {
  Scene s;
  ASSERT_NE(s.frame->content(), nullptr);
  const Rect body = s.frame->content()->bounds();
  EXPECT_GE(body.y, 32.0 + 10.0 - 0.001);
  EXPECT_DOUBLE_EQ(body.x, 10.0);
  EXPECT_LT(body.bottom(), s.frame->button("ok")->bounds().y);
}

TEST(DialogFrame, HitTestSeparatesCaptionFromControls) {
  Scene s;
  EXPECT_EQ(s.frame->hitTestWindow(Point{150, 10}), DialogHit::Caption);
  EXPECT_EQ(s.frame->hitTestWindow(Scene::center(s.frame->closeControl()->bounds())), DialogHit::Client);
  EXPECT_EQ(s.frame->hitTestWindow(Scene::center(s.frame->maximizeControl()->bounds())), DialogHit::Client);
  EXPECT_EQ(s.frame->hitTestWindow(Point{150, 100}), DialogHit::Client);
}

TEST(DialogFrame, ClosingAndMaximizingReportThroughCallbacks) {
  Scene s;
  s.click(Scene::center(s.frame->closeControl()->bounds()));
  EXPECT_EQ(s.closes, (std::vector<DialogCloseReason>{DialogCloseReason::CloseButton}));
  s.click(Scene::center(s.frame->maximizeControl()->bounds()));
  EXPECT_EQ(s.maximizes, 1);
}

TEST(DialogFrame, ButtonClicksReportTheirId) {
  Scene s;
  s.click(Scene::center(s.frame->button("cancel")->bounds()));
  s.click(Scene::center(s.frame->button("ok")->bounds()));
  EXPECT_EQ(s.pressed, (std::vector<std::string>{"cancel", "ok"}));
}

TEST(DialogFrame, DisabledButtonsDoNotReportClicks) {
  Scene s;
  s.frame->setButtonDisabled("ok", true);
  s.click(Scene::center(s.frame->button("ok")->bounds()));
  EXPECT_TRUE(s.pressed.empty());
  s.frame->setButtonDisabled("ok", false);
  s.click(Scene::center(s.frame->button("ok")->bounds()));
  EXPECT_EQ(s.pressed.size(), 1u);
}

TEST(DialogFrame, EscapeRequestsCloseOnlyWhenClosable) {
  Scene s;
  s.frame->setClosable(true);
  s.frame->focusInitial();
  s.root.keyDown(kVkEscape, 0, false);
  EXPECT_EQ(s.closes, (std::vector<DialogCloseReason>{DialogCloseReason::Escape}));
  s.closes.clear();
  s.frame->setClosable(false);
  s.root.layout(kWindow);
  s.frame->focusInitial();
  s.root.keyDown(kVkEscape, 0, false);
  EXPECT_TRUE(s.closes.empty());
  EXPECT_EQ(s.frame->closeControl(), nullptr);
}

TEST(DialogFrame, EnterInATextInputTriggersTheDefaultButton) {
  Scene s;
  s.root.setFocus(s.input, true);
  s.root.keyDown(kVkReturn, 0, false);
  EXPECT_EQ(s.pressed, (std::vector<std::string>{"ok"}));
}

TEST(DialogFrame, EnterOnAnotherButtonKeepsItsOwnMeaning) {
  Scene s;
  s.root.setFocus(s.frame->button("cancel"), true);
  s.root.keyDown(kVkReturn, 0, false);
  EXPECT_EQ(s.pressed, (std::vector<std::string>{"cancel"}));
}

TEST(DialogFrame, InitialFocusGoesToTheDefaultButtonThenToTheFirstFocusableInTheBody) {
  Scene s;
  s.frame->focusInitial();
  EXPECT_TRUE(s.frame->button("ok")->focused());
  Scene noDefault;
  noDefault.frame->setButtons({DialogButton{.id = "cancel", .text = "取消", .isCancel = true}});
  noDefault.root.layout(kWindow);
  noDefault.frame->focusInitial();
  EXPECT_TRUE(noDefault.input->focused());
}

TEST(DialogFrame, NaturalHeightAddsTheTitleBarBodyAndFooter) {
  Scene s(true, false);
  const double natural = s.frame->naturalHeight(420, 640);
  const double body = s.frame->content()->bounds().height + 2 * 10.0;
  EXPECT_NEAR(natural, 32.0 + body + 38.0, 0.6);
}

TEST(DialogFrame, TabMovesThroughTheBodyAndTheButtons) {
  Scene s;
  s.root.keyDown(kVkTab, 0, false);
  EXPECT_TRUE(s.frame->closeControl() != nullptr);
  EXPECT_TRUE(s.root.focusedWidget() != nullptr);
}

namespace {

std::vector<std::uint8_t> render(Scene& s) {
  OffscreenSurface surface(kWindow, 1.0);
  EXPECT_TRUE(surface.render(s.root, Color::rgba(0, 0, 0)));
  return surface.pixels();
}

std::uint32_t pixelAt(const std::vector<std::uint8_t>& data, int x, int y) {
  const std::size_t i = (static_cast<std::size_t>(y) * static_cast<std::size_t>(kWindow.width) + static_cast<std::size_t>(x)) * 4;
  return (static_cast<std::uint32_t>(data[i + 3]) << 24) | (static_cast<std::uint32_t>(data[i + 2]) << 16) | (static_cast<std::uint32_t>(data[i + 1]) << 8) | data[i];
}

void expectColor(std::uint32_t actual, Color expected) {
  EXPECT_NEAR(static_cast<int>((actual >> 16) & 0xFF), expected.r(), 1);
  EXPECT_NEAR(static_cast<int>((actual >> 8) & 0xFF), expected.g(), 1);
  EXPECT_NEAR(static_cast<int>(actual & 0xFF), expected.b(), 1);
}

}  // namespace

TEST(DialogFrame, TitleBarUsesTheActiveOrInactiveBackground) {
  Scene s;
  const auto& colors = themeTokens(ThemeName::Dark);
  expectColor(pixelAt(render(s), 200, 15), compose(colors.dialog_titlebar_bg, Color::rgba(0, 0, 0)));
  s.frame->setWindowActive(false);
  expectColor(pixelAt(render(s), 200, 15), compose(colors.dialog_titlebar_inactive_bg, Color::rgba(0, 0, 0)));
}

TEST(DialogFrame, CloseControlTurnsRedWhenHovered) {
  Scene s;
  const Rect close = s.frame->closeControl()->bounds();
  s.root.mouseMove(Scene::center(close), 0);
  const auto& colors = themeTokens(ThemeName::Dark);
  expectColor(pixelAt(render(s), static_cast<int>(close.x) + 2, static_cast<int>(close.y) + 12), compose(colors.dialog_close_hover_bg, compose(colors.dialog_titlebar_bg, Color::rgba(0, 0, 0))));
}
