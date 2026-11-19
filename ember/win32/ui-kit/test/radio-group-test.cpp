/*
 * ------------------------------------------------------------------------
 * 名称：radio-group-test.cpp
 * 说明：分段单选组的测试：点击选中并回调、设置值不回调、禁用的选项、方向键循环与跳过禁用项、整组单一 Tab 停靠点、选中项与悬停的底色。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-19
 * 备注：颜色断言取选项内部不含文字的位置，与生成的令牌比较。
 * ------------------------------------------------------------------------
 */

#include <gtest/gtest.h>

#include <memory>
#include <string>
#include <vector>

#include "base/theme-tokens.hpp"
#include "rujian/ui/offscreen.hpp"
#include "rujian/ui/radio-group.hpp"
#include "rujian/ui/root.hpp"

using namespace rujian::ui;

namespace {

constexpr int kVkTab = 0x09;
constexpr int kVkSpace = 0x20;
constexpr int kVkLeft = 0x25;
constexpr int kVkRight = 0x27;

/** 容纳一个单选组的页面：单选组放在 (10,10)，宽 300（三个选项各 99.33）。 */
class Holder final : public Widget {
 public:
  void arrange(const Rect& bounds) override {
    Widget::arrange(bounds);
    for (const auto& child : children()) {
      const Size size = child->measure(Size{300, 0});
      child->arrange(Rect{10, 10, size.width, size.height});
    }
  }
};

struct Scene {
  Root root{ThemeName::Dark, 1.0};
  RadioGroup* group{nullptr};
  std::vector<std::string> changes;

  explicit Scene(bool secondDisabled = false) {
    auto holder = std::make_unique<Holder>();
    auto created = std::make_unique<RadioGroup>();
    group = created.get();
    holder->addChild(std::move(created));
    root.setContent(std::move(holder));
    group->setOptions({{"a", "全部", false}, {"b", "最近", secondDisabled}, {"c", "收藏", false}});
    group->setOnChange([this](const std::string& value) { changes.push_back(value); });
    root.layout(Size{320, 60});
  }

  /** 第 index 个选项的中心点。 */
  static Point center(int index) { return Point{11 + 99.33 * (index + 0.5), 10 + 17}; }

  void click(int index) {
    root.mouseMove(center(index), 0);
    root.mouseDown(center(index), MouseButton::Left, 0);
    root.mouseUp(center(index), MouseButton::Left, 0);
  }

  [[nodiscard]] std::vector<std::uint8_t> render() {
    root.layout(Size{320, 60});
    OffscreenSurface surface(Size{320, 60}, 1.0);
    EXPECT_TRUE(surface.render(root, themeTokens(root.theme()).content_bg));
    return surface.pixels();
  }
};

void expectColor(const std::vector<std::uint8_t>& data, int x, int y, Color expected) {
  const std::size_t i = (static_cast<std::size_t>(y) * 320 + x) * 4;
  EXPECT_NEAR(static_cast<int>(data[i + 2]), expected.r(), 2);
  EXPECT_NEAR(static_cast<int>(data[i + 1]), expected.g(), 2);
  EXPECT_NEAR(static_cast<int>(data[i]), expected.b(), 2);
}

/** 半透明的前景色盖在不透明背景上的结果。 */
Color over(Color foreground, Color background) {
  const double alpha = foreground.a() / 255.0;
  const auto mix = [&](int front, int back) { return static_cast<std::uint8_t>(front * alpha + back * (1.0 - alpha) + 0.5); };
  return Color::rgba(mix(foreground.r(), background.r()), mix(foreground.g(), background.g()), mix(foreground.b(), background.b()));
}

}  // namespace

TEST(RadioGroup, ClickSelectsAndReportsOnlyWhenTheValueChanges) {
  Scene s;
  s.click(1);
  EXPECT_EQ(s.group->value(), "b");
  s.click(1);
  s.click(2);
  EXPECT_EQ(s.changes, (std::vector<std::string>{"b", "c"}));
}

TEST(RadioGroup, SetValueDoesNotReportAndUnknownValueSelectsNothing) {
  Scene s;
  s.group->setValue("c");
  EXPECT_EQ(s.group->value(), "c");
  s.group->setValue("none");
  EXPECT_EQ(s.group->value(), "");
  EXPECT_TRUE(s.changes.empty());
}

TEST(RadioGroup, DisabledOptionCannotBeClicked) {
  Scene s(true);
  s.click(1);
  EXPECT_EQ(s.group->value(), "");
  EXPECT_TRUE(s.changes.empty());
}

TEST(RadioGroup, ReleasingOnAnotherOptionDoesNotSelect) {
  Scene s;
  s.root.mouseMove(Scene::center(0), 0);
  s.root.mouseDown(Scene::center(0), MouseButton::Left, 0);
  s.root.mouseMove(Scene::center(2), 0);
  s.root.mouseUp(Scene::center(2), MouseButton::Left, 0);
  EXPECT_EQ(s.group->value(), "");
}

TEST(RadioGroup, TabStopsOnceAndArrowsMoveAndSelectWithWrapping) {
  Scene s;
  s.group->setValue("a");
  s.root.keyDown(kVkTab, 0, false);
  ASSERT_TRUE(s.group->focused());
  s.root.keyDown(kVkRight, 0, false);
  EXPECT_EQ(s.group->value(), "b");
  s.root.keyDown(kVkRight, 0, false);
  s.root.keyDown(kVkRight, 0, false);
  EXPECT_EQ(s.group->value(), "a");
  s.root.keyDown(kVkLeft, 0, false);
  EXPECT_EQ(s.group->value(), "c");
  EXPECT_EQ(s.changes, (std::vector<std::string>{"b", "c", "a", "c"}));
  // 整组只有一个停靠点：Tab 不会在选项之间移动，也不会改变选中项
  s.root.keyDown(kVkTab, 0, false);
  EXPECT_EQ(s.group->value(), "c");
}

TEST(RadioGroup, ArrowsSkipDisabledOptions) {
  Scene s(true);
  s.group->setValue("a");
  s.root.keyDown(kVkTab, 0, false);
  s.root.keyDown(kVkRight, 0, false);
  EXPECT_EQ(s.group->value(), "c");
}

TEST(RadioGroup, SpaceSelectsTheFocusedOptionWhenNothingIsSelected) {
  Scene s;
  s.root.keyDown(kVkTab, 0, false);
  s.root.keyDown(kVkSpace, 0, false);
  EXPECT_EQ(s.group->value(), "a");
}

TEST(RadioGroup, DisabledGroupIgnoresInputAndIsSkippedByTab) {
  Scene s;
  s.group->setEnabled(false);
  s.click(2);
  s.root.keyDown(kVkTab, 0, false);
  EXPECT_FALSE(s.group->focused());
  EXPECT_EQ(s.group->value(), "");
}

TEST(RadioGroup, CursorIsHandOnlyOverEnabledOptions) {
  Scene s(true);
  s.root.mouseMove(Scene::center(0), 0);
  EXPECT_EQ(s.group->cursor(), CursorKind::Hand);
  s.root.mouseMove(Scene::center(1), 0);
  EXPECT_EQ(s.group->cursor(), CursorKind::Arrow);
}

TEST(RadioGroup, MeasureIsFixedHeightAndTakesTheGivenWidth) {
  Scene s;
  const Size size = s.group->measure(Size{300, 0});
  EXPECT_DOUBLE_EQ(size.width, 300);
  EXPECT_DOUBLE_EQ(size.height, tokens::segmented_height + 2);
}

TEST(RadioGroup, SelectedOptionHasBackgroundAndIndicatorAndHoverReplacesTheBackground) {
  Scene s;
  const auto& colors = themeTokens(ThemeName::Dark);
  const Color page = colors.content_bg;
  s.group->setValue("b");
  // 中间项里不含文字的位置：左上角附近；最下面 2 像素是强调线
  auto pixels = s.render();
  expectColor(pixels, 117, 14, over(colors.choice_selected_bg, page));
  expectColor(pixels, 150, 10 + 34 - 2, colors.accent);
  expectColor(pixels, 20, 14, page);
  // 悬停时底色换成悬停色，强调线仍在
  s.root.mouseMove(Scene::center(1), 0);
  pixels = s.render();
  expectColor(pixels, 117, 14, over(colors.option_hover_bg, page));
  expectColor(pixels, 150, 10 + 34 - 2, colors.accent);
}
