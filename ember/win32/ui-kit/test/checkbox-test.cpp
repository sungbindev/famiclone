/*
 * ------------------------------------------------------------------------
 * 名称：checkbox-test.cpp
 * 说明：复选框的测试：点击与空格切换并回调、设置值不回调、禁用、Tab 遍历、方框颜色。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-19
 * 备注：颜色断言取方框内侧不含对勾的位置，与生成的令牌比较。
 * ------------------------------------------------------------------------
 */

#include <gtest/gtest.h>

#include <memory>
#include <vector>

#include "base/theme-tokens.hpp"
#include "rujian/ui/checkbox.hpp"
#include "rujian/ui/offscreen.hpp"
#include "rujian/ui/root.hpp"

using namespace rujian::ui;

namespace {

constexpr int kVkTab = 0x09;
constexpr int kVkSpace = 0x20;

/** 容纳一个复选框的页面：复选框放在 (10,10)，宽 120。 */
class Holder final : public Widget {
 public:
  void arrange(const Rect& bounds) override {
    Widget::arrange(bounds);
    for (const auto& child : children()) {
      const Size size = child->measure({});
      child->arrange(Rect{10, 10, 120, size.height});
    }
  }
};

struct Scene {
  Root root{ThemeName::Dark, 1.0};
  Checkbox* checkbox{nullptr};
  std::vector<bool> changes;

  Scene() {
    auto holder = std::make_unique<Holder>();
    auto created = std::make_unique<Checkbox>("启用");
    checkbox = created.get();
    holder->addChild(std::move(created));
    root.setContent(std::move(holder));
    checkbox->setOnChange([this](bool checked) { changes.push_back(checked); });
    root.layout(Size{200, 44});
  }

  void click(Point point = Point{60, 22}) {
    root.mouseMove(point, 0);
    root.mouseDown(point, MouseButton::Left, 0);
    root.mouseUp(point, MouseButton::Left, 0);
  }

  [[nodiscard]] std::vector<std::uint8_t> render() {
    root.layout(Size{200, 44});
    OffscreenSurface surface(Size{200, 44}, 1.0);
    EXPECT_TRUE(surface.render(root, themeTokens(root.theme()).content_bg));
    return surface.pixels();
  }
};

void expectColor(const std::vector<std::uint8_t>& data, int x, int y, Color expected) {
  const std::size_t i = (static_cast<std::size_t>(y) * 200 + x) * 4;
  EXPECT_NEAR(static_cast<int>(data[i + 2]), expected.r(), 1);
  EXPECT_NEAR(static_cast<int>(data[i + 1]), expected.g(), 1);
  EXPECT_NEAR(static_cast<int>(data[i]), expected.b(), 1);
}

/** 半透明的前景色盖在不透明背景上的结果。 */
Color over(Color foreground, Color background) {
  const double alpha = foreground.a() / 255.0;
  const auto mix = [&](int front, int back) { return static_cast<std::uint8_t>(front * alpha + back * (1.0 - alpha) + 0.5); };
  return Color::rgba(mix(foreground.r(), background.r()), mix(foreground.g(), background.g()), mix(foreground.b(), background.b()));
}

}  // namespace

TEST(Checkbox, ClickTogglesAndReportsTheNewState) {
  Scene s;
  s.click();
  EXPECT_TRUE(s.checkbox->checked());
  s.click();
  EXPECT_FALSE(s.checkbox->checked());
  EXPECT_EQ(s.changes, (std::vector<bool>{true, false}));
}

TEST(Checkbox, ClickAnywhereOnTheRowToggles) {
  Scene s;
  s.click(Point{12, 22});
  s.click(Point{125, 22});
  EXPECT_EQ(s.changes, (std::vector<bool>{true, false}));
}

TEST(Checkbox, SetCheckedDoesNotReport) {
  Scene s;
  s.checkbox->setChecked(true);
  EXPECT_TRUE(s.checkbox->checked());
  EXPECT_TRUE(s.changes.empty());
}

TEST(Checkbox, ReleasingOutsideDoesNotToggle) {
  Scene s;
  s.root.mouseMove(Point{60, 22}, 0);
  s.root.mouseDown(Point{60, 22}, MouseButton::Left, 0);
  s.root.mouseMove(Point{190, 40}, 0);
  s.root.mouseUp(Point{190, 40}, MouseButton::Left, 0);
  EXPECT_FALSE(s.checkbox->checked());
  EXPECT_TRUE(s.changes.empty());
}

TEST(Checkbox, SpaceTogglesWhenFocusedFromTheKeyboard) {
  Scene s;
  s.root.keyDown(kVkTab, 0, false);
  ASSERT_TRUE(s.checkbox->focused());
  s.root.keyDown(kVkSpace, 0, false);
  s.root.keyUp(kVkSpace, 0);
  EXPECT_TRUE(s.checkbox->checked());
  EXPECT_EQ(s.changes, (std::vector<bool>{true}));
}

TEST(Checkbox, DisabledIgnoresInputAndIsSkippedByTab) {
  Scene s;
  s.checkbox->setEnabled(false);
  s.click();
  s.root.keyDown(kVkTab, 0, false);
  EXPECT_FALSE(s.checkbox->focused());
  EXPECT_FALSE(s.checkbox->checked());
  EXPECT_TRUE(s.changes.empty());
}

TEST(Checkbox, MouseFocusDoesNotShowTheFocusRing) {
  Scene s;
  s.click();
  EXPECT_TRUE(s.checkbox->focused());
  EXPECT_FALSE(s.checkbox->focusVisible());
}

TEST(Checkbox, CursorIsHandWhenEnabledAndArrowWhenDisabled) {
  Scene s;
  EXPECT_EQ(s.checkbox->cursor(), CursorKind::Hand);
  s.checkbox->setEnabled(false);
  EXPECT_EQ(s.checkbox->cursor(), CursorKind::Arrow);
}

TEST(Checkbox, MeasureHasMinimumHeightAndFitsTheLabel) {
  Scene s;
  const Size size = s.checkbox->measure({});
  EXPECT_DOUBLE_EQ(size.height, tokens::choice_min_height);
  EXPECT_GT(size.width, tokens::choice_mark_size + tokens::choice_gap);
}

TEST(Checkbox, MarkColorsFollowCheckedAndDisabledStates) {
  Scene s;
  const auto& colors = themeTokens(ThemeName::Dark);
  // 方框在 (10,15)-(24,29)，取内侧左上角附近、不含对勾的位置
  expectColor(s.render(), 12, 17, colors.choice_mark_bg);
  s.checkbox->setChecked(true);
  expectColor(s.render(), 12, 17, colors.accent);
  s.checkbox->setEnabled(false);
  expectColor(s.render(), 12, 17, over(colors.disabled_text, colors.content_bg));
  s.checkbox->setChecked(false);
  expectColor(s.render(), 12, 17, over(colors.disabled_bg, colors.content_bg));
}
