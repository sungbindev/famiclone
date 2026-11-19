/*
 * ------------------------------------------------------------------------
 * 名称：status-bar-test.cpp
 * 说明：状态栏的测试：高度与尺寸、左右两组的排列、各部分的宽度（状态点、图标、文字、指示灯）、放不下时按权重收缩、按钮状态项的点击与键盘、绘制出的颜色。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-19
 * 备注：窗口宽 600、高与状态栏一致；文字宽度依赖本机字体，所以文字宽度只用相对关系，不断言具体值。
 * ------------------------------------------------------------------------
 */

#include <gtest/gtest.h>

#include <memory>
#include <string>
#include <vector>

#include "base/theme-tokens.hpp"
#include "rujian/ui/offscreen.hpp"
#include "rujian/ui/root.hpp"
#include "rujian/ui/status-bar.hpp"

using namespace rujian::ui;

namespace {

constexpr int kVkReturn = 0x0D;
constexpr int kVkSpace = 0x20;
constexpr int kVkTab = 0x09;

struct Scene {
  Root root{ThemeName::Dark, 1.0};
  StatusBar* bar{nullptr};
  Size size{600, 26};

  explicit Scene(bool large = false) {
    auto statusBar = std::make_unique<StatusBar>();
    bar = statusBar.get();
    bar->setLarge(large);
    size.height = large ? 32 : 26;
    root.setContent(std::move(statusBar));
  }

  void layout() { root.layout(size); }

  void click(Point point) {
    root.mouseMove(point, 0);
    root.mouseDown(point, MouseButton::Left, 0);
    root.mouseUp(point, MouseButton::Left, 0);
  }

  static Point center(const Rect& rect) { return Point{rect.x + rect.width / 2, rect.y + rect.height / 2}; }
};

std::unique_ptr<StatusBarItem> item(const char* text) { return std::make_unique<StatusBarItem>(text); }

}  // namespace

TEST(StatusBar, HasTheTokenHeightForBothSizes) {
  Scene normal;
  normal.layout();
  EXPECT_DOUBLE_EQ(normal.bar->bounds().height, 26.0);
  Scene large(true);
  large.layout();
  EXPECT_DOUBLE_EQ(large.bar->bounds().height, 32.0);
}

TEST(StatusBar, ItemsAreCenteredInTheContentAreaBelowTheTopBorder) {
  Scene s;
  StatusBarItem* a = s.bar->addStart(item("就绪"));
  s.layout();
  EXPECT_DOUBLE_EQ(a->bounds().height, 22.0);
  EXPECT_DOUBLE_EQ(a->bounds().y, 1.0 + (25.0 - 22.0) / 2.0);
  EXPECT_DOUBLE_EQ(a->bounds().x, 8.0);
}

TEST(StatusBar, LargeSizeUsesLargeItemsAndIcons) {
  Scene s(true);
  StatusBarItem* a = s.bar->addStart(item("就绪"));
  StatusBarItem* b = s.bar->addEnd(item("就绪"));
  b->setIcon(IconId::Settings);
  s.layout();
  EXPECT_DOUBLE_EQ(a->bounds().height, 26.0);
  EXPECT_GT(b->bounds().width, a->bounds().width + 18.0);
}

TEST(StatusBar, StartItemsFlowLeftToRightWithAGapAndEndItemsAreRightAligned) {
  Scene s;
  StatusBarItem* a = s.bar->addStart(item("甲"));
  StatusBarItem* b = s.bar->addStart(item("乙"));
  StatusBarItem* c = s.bar->addEnd(item("丙"));
  StatusBarItem* d = s.bar->addEnd(item("丁"));
  s.layout();
  EXPECT_DOUBLE_EQ(b->bounds().x, a->bounds().right() + 4.0);
  EXPECT_DOUBLE_EQ(d->bounds().right(), 600.0 - 8.0);
  EXPECT_DOUBLE_EQ(c->bounds().right() + 4.0, d->bounds().x);
}

TEST(StatusBar, PartsAreSeparatedByFourPixels) {
  Scene s;
  StatusBarItem* text = s.bar->addStart(item("文字"));
  StatusBarItem* withIcon = s.bar->addStart(item("文字"));
  withIcon->setIcon(IconId::Settings);
  StatusBarItem* withDot = s.bar->addStart(item("文字"));
  withDot->setDot(StatusDot::Success);
  StatusBarItem* withLamp = s.bar->addStart(item("文字"));
  withLamp->setLamp(StatusLamp{.lit = true, .label = "灯"});
  s.layout();
  EXPECT_DOUBLE_EQ(withIcon->bounds().width, text->bounds().width + 16.0 + 4.0);
  EXPECT_DOUBLE_EQ(withDot->bounds().width, text->bounds().width + 8.0 + 4.0);
  EXPECT_DOUBLE_EQ(withLamp->bounds().width, text->bounds().width + 8.0 + 4.0);
}

TEST(StatusBar, DotReplacesTheIcon) {
  Scene s;
  StatusBarItem* both = s.bar->addStart(item("文字"));
  both->setIcon(IconId::Settings);
  both->setDot(StatusDot::Warning);
  StatusBarItem* dotOnly = s.bar->addStart(item("文字"));
  dotOnly->setDot(StatusDot::Warning);
  s.layout();
  EXPECT_DOUBLE_EQ(both->bounds().width, dotOnly->bounds().width);
}

TEST(StatusBar, WarningAndErrorTonesBringTheirOwnIcon) {
  Scene s;
  StatusBarItem* normal = s.bar->addStart(item("文字"));
  StatusBarItem* warning = s.bar->addStart(item("文字"));
  warning->setTone(StatusItemTone::Warning);
  StatusBarItem* success = s.bar->addStart(item("文字"));
  success->setTone(StatusItemTone::Success);
  s.layout();
  EXPECT_DOUBLE_EQ(warning->bounds().width, normal->bounds().width + 16.0 + 4.0);
  EXPECT_DOUBLE_EQ(success->bounds().width, normal->bounds().width);
}

TEST(StatusBar, LampDrawnInsideTheIconTakesNoExtraWidth) {
  Scene s;
  StatusBarItem* plain = s.bar->addStart(item("软驱"));
  plain->setIcon(IconId::DeviceFloppy);
  StatusBarItem* inIcon = s.bar->addStart(item("软驱"));
  inIcon->setIcon(IconId::DeviceFloppy);
  inIcon->setLamp(StatusLamp{.lit = true, .label = "读写灯", .inIcon = Point{50, 60}});
  s.layout();
  EXPECT_DOUBLE_EQ(inIcon->bounds().width, plain->bounds().width);
}

TEST(StatusBar, IconOnlyItemsHideTheTextAndDoNotShrink) {
  Scene s;
  StatusBarItem* a = s.bar->addEnd(item("很长很长很长很长很长很长很长很长很长很长很长很长"));
  a->setIcon(IconId::Settings);
  a->setIconOnly(true);
  s.layout();
  EXPECT_DOUBLE_EQ(a->bounds().width, 2 * 4.0 + 16.0);
  EXPECT_DOUBLE_EQ(a->shrinkWeight(), 0.0);
}

TEST(StatusBar, OnlyLongTextShrinksWhenThereIsNotEnoughRoom) {
  Scene s;
  s.size.width = 300;
  StatusBarItem* shortOne = s.bar->addStart(item("短文字"));
  StatusBarItem* longOne = s.bar->addStart(item("这是一段很长很长很长很长很长很长很长很长很长很长的状态文字用来测试收缩"));
  StatusBarItem* right = s.bar->addEnd(item("右侧状态项"));
  s.layout();
  const double natural = shortOne->measure(Size{}).width;
  EXPECT_DOUBLE_EQ(shortOne->bounds().width, natural);
  EXPECT_LT(longOne->bounds().width, longOne->measure(Size{}).width);
  // 右侧保持原宽靠右，左侧的组不越过它
  EXPECT_DOUBLE_EQ(right->bounds().width, right->measure(Size{}).width);
  EXPECT_DOUBLE_EQ(right->bounds().right(), 300.0 - 8.0);
  EXPECT_LE(longOne->bounds().right() + 8.0, right->bounds().x + 0.001);
}

TEST(StatusBar, ShrinkWeightIsTheSquareOfTheLengthOnlyAboveSixteenCharacters) {
  EXPECT_DOUBLE_EQ(item("一二三四五六七八九十一二三四五六")->shrinkWeight(), 0.0);
  EXPECT_DOUBLE_EQ(item("一二三四五六七八九十一二三四五六七")->shrinkWeight(), 17.0 * 17.0);
}

TEST(StatusBar, ButtonItemsReportClicksAndPlainItemsDoNot) {
  Scene s;
  int clicks = 0;
  StatusBarItem* plain = s.bar->addStart(item("普通"));
  StatusBarItem* button = s.bar->addEnd(item("按钮"));
  button->setOnClick([&clicks] { ++clicks; });
  s.layout();
  s.click(Scene::center(plain->bounds()));
  EXPECT_EQ(clicks, 0);
  s.click(Scene::center(button->bounds()));
  EXPECT_EQ(clicks, 1);
  EXPECT_FALSE(plain->focusable());
  EXPECT_TRUE(button->focusable());
}

TEST(StatusBar, ButtonItemsRespondToTheKeyboard) {
  Scene s;
  int clicks = 0;
  StatusBarItem* button = s.bar->addEnd(item("按钮"));
  button->setOnClick([&clicks] { ++clicks; });
  s.layout();
  s.root.keyDown(kVkTab, 0, false);
  ASSERT_TRUE(button->focused());
  s.root.keyDown(kVkReturn, 0, false);
  EXPECT_EQ(clicks, 1);
  s.root.keyDown(kVkSpace, 0, false);
  EXPECT_EQ(clicks, 1);
  s.root.keyUp(kVkSpace, 0);
  EXPECT_EQ(clicks, 2);
}

namespace {

std::vector<std::uint8_t> render(Scene& s) {
  OffscreenSurface surface(s.size, 1.0);
  EXPECT_TRUE(surface.render(s.root, Color::rgba(0, 0, 0)));
  return surface.pixels();
}

std::uint32_t pixelAt(const std::vector<std::uint8_t>& data, int width, int x, int y) {
  const std::size_t i = (static_cast<std::size_t>(y) * static_cast<std::size_t>(width) + static_cast<std::size_t>(x)) * 4;
  return (static_cast<std::uint32_t>(data[i + 3]) << 24) | (static_cast<std::uint32_t>(data[i + 2]) << 16) | (static_cast<std::uint32_t>(data[i + 1]) << 8) | data[i];
}

void expectColor(std::uint32_t actual, Color expected) {
  EXPECT_NEAR(static_cast<int>((actual >> 16) & 0xFF), expected.r(), 1);
  EXPECT_NEAR(static_cast<int>((actual >> 8) & 0xFF), expected.g(), 1);
  EXPECT_NEAR(static_cast<int>(actual & 0xFF), expected.b(), 1);
}

}  // namespace

TEST(StatusBar, PaintsTheBackgroundTheTopBorderAndTheDotColor) {
  Scene s;
  StatusBarItem* running = s.bar->addStart(item("运行中"));
  running->setDot(StatusDot::Success);
  s.layout();
  const auto pixels = render(s);
  const auto& colors = themeTokens(ThemeName::Dark);
  expectColor(pixelAt(pixels, 600, 300, 12), compose(colors.status_bar_bg, Color::rgba(0, 0, 0)));
  expectColor(pixelAt(pixels, 600, 300, 0), compose(compose(colors.chrome_border, colors.status_bar_bg), Color::rgba(0, 0, 0)));
  const Rect box = running->bounds();
  expectColor(pixelAt(pixels, 600, static_cast<int>(box.x + 4 + 4), static_cast<int>(box.y + box.height / 2)), colors.status_success);
}

TEST(StatusBar, LampColorFollowsItsLitState) {
  Scene s;
  StatusBarItem* lamp = s.bar->addEnd(item(""));
  lamp->setLamp(StatusLamp{.lit = true, .label = "灯"});
  s.layout();
  const auto& colors = themeTokens(ThemeName::Dark);
  const Rect box = lamp->bounds();
  const int x = static_cast<int>(box.right() - 4 - 4);
  const int y = static_cast<int>(box.y + box.height / 2);
  expectColor(pixelAt(render(s), 600, x, y), colors.status_bar_lamp_on);
  lamp->setLamp(StatusLamp{.lit = false, .label = "灯"});
  expectColor(pixelAt(render(s), 600, x, y), colors.status_bar_lamp_off);
}

TEST(StatusBar, PressedButtonItemUsesTheSelectedBackground) {
  Scene s;
  StatusBarItem* button = s.bar->addEnd(item("面板"));
  button->setOnClick([] {});
  button->setPressed(true);
  s.layout();
  const auto& colors = themeTokens(ThemeName::Dark);
  const Rect box = button->bounds();
  const Color expected = compose(compose(colors.ghost_selected_background, colors.status_bar_bg), Color::rgba(0, 0, 0));
  expectColor(pixelAt(render(s), 600, static_cast<int>(box.x + 1), static_cast<int>(box.y + box.height / 2)), expected);
}
