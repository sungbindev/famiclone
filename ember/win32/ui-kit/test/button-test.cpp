/*
 * ------------------------------------------------------------------------
 * 名称：button-test.cpp
 * 说明：按钮的测试：文字到图标的自动匹配、尺寸规则（紧凑、纯图标、图标位置）、各样式与状态的颜色、链接的换行与下划线。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-19
 * 备注：颜色断言取按钮内部不含文字的位置，与生成的令牌比较；尺寸断言只比较相对关系与固定尺寸。
 * ------------------------------------------------------------------------
 */

#include <gtest/gtest.h>

#include <memory>
#include <vector>

#include "base/icon-rules.hpp"
#include "base/theme-tokens.hpp"
#include "rujian/ui/button.hpp"
#include "rujian/ui/offscreen.hpp"
#include "rujian/ui/root.hpp"

using namespace rujian::ui;

namespace {

/** 容纳一个按钮的页面：按钮放在 (10,10)。 */
class Holder final : public Widget {
 public:
  void arrange(const Rect& bounds) override {
    Widget::arrange(bounds);
    for (const auto& child : children()) {
      const Size size = child->measure({});
      child->arrange(Rect{10, 10, size.width, size.height});
    }
  }
};

/** 挂到 Root 上的按钮，方便度量与绘制。 */
struct Scene {
  Root root;
  Button* button;
  explicit Scene(const char* label, ThemeName theme = ThemeName::Dark) : root(theme, 1.0), button(nullptr) {
    auto holder = std::make_unique<Holder>();
    auto created = std::make_unique<Button>(label);
    button = created.get();
    holder->addChild(std::move(created));
    root.setContent(std::move(holder));
    root.layout(Size{200, 44});
  }
  [[nodiscard]] std::vector<std::uint8_t> render() {
    root.layout(Size{200, 44});
    OffscreenSurface surface(Size{200, 44}, 1.0);
    EXPECT_TRUE(surface.render(root, themeTokens(root.theme()).content_bg));
    return surface.pixels();
  }
};

std::uint32_t pixelAt(const std::vector<std::uint8_t>& data, int x, int y) {
  const std::size_t i = (static_cast<std::size_t>(y) * 200 + x) * 4;
  return (static_cast<std::uint32_t>(data[i + 3]) << 24) | (static_cast<std::uint32_t>(data[i + 2]) << 16) | (static_cast<std::uint32_t>(data[i + 1]) << 8) | data[i];
}

void expectColor(std::uint32_t actual, Color expected) {
  EXPECT_NEAR(static_cast<int>((actual >> 16) & 0xFF), expected.r(), 1);
  EXPECT_NEAR(static_cast<int>((actual >> 8) & 0xFF), expected.g(), 1);
  EXPECT_NEAR(static_cast<int>(actual & 0xFF), expected.b(), 1);
}

}  // namespace

TEST(IconRules, MatchesByLabelPrefixAndSuffix) {
  EXPECT_EQ(iconForLabel("取消")->icon, IconId::X);
  EXPECT_EQ(iconForLabel("确定")->icon, IconId::Check);
  EXPECT_EQ(iconForLabel("保存设置")->icon, IconId::DeviceFloppy);
  EXPECT_EQ(iconForLabel("开始生成")->icon, IconId::Sparkles);
  EXPECT_EQ(iconForLabel("批量生成")->icon, IconId::Sparkles);
}

TEST(IconRules, EarlierRulesWinAndEndPositionIsKept) {
  EXPECT_EQ(iconForLabel("取消生成")->icon, IconId::PlayerStop);
  EXPECT_EQ(iconForLabel("重新生成")->icon, IconId::Refresh);
  EXPECT_TRUE(iconForLabel("下一步")->atEnd);
  EXPECT_FALSE(iconForLabel("上一步")->atEnd);
}

TEST(IconRules, TrimsWhitespaceAndRejectsUnknownLabels) {
  EXPECT_EQ(iconForLabel("  取消  ")->icon, IconId::X);
  EXPECT_FALSE(iconForLabel("没有规则的文字"));
  EXPECT_FALSE(iconForLabel(""));
}

TEST(IconRules, AnchoredPatternsDoNotMatchInTheMiddle) {
  EXPECT_FALSE(iconForLabel("请取消"));
  EXPECT_FALSE(iconForLabel("再确定一下"));
}

TEST(Button, AutoIconWidensButtonByIconAndGap) {
  Scene automatic("保存");
  Scene plain("保存");
  plain.button->setAutoIcon(false);
  EXPECT_NEAR(automatic.button->measure({}).width - plain.button->measure({}).width, tokens::btn_icon_size + tokens::icon_text_gap, 1e-9);
}

TEST(Button, ExplicitIconWinsOverAutoMatch) {
  Scene scene("保存");
  const double automatic = scene.button->measure({}).width;
  scene.button->setIcon(IconId::Plus);
  EXPECT_EQ(scene.button->measure({}).width, automatic);
  scene.button->setIcon(std::nullopt);
  scene.button->setAutoIcon(false);
  EXPECT_LT(scene.button->measure({}).width, automatic);
}

TEST(Button, CompactHasMinimumWidthButNeverShrinksContent) {
  Scene scene("好");
  scene.button->setAutoIcon(false);
  EXPECT_LT(scene.button->measure({}).width, tokens::btn_compact_min_width);
  scene.button->setCompact(true);
  EXPECT_EQ(scene.button->measure({}).width, tokens::btn_compact_min_width);
  scene.button->setLabel("一个很长很长很长很长很长很长的按钮");
  EXPECT_GT(scene.button->measure({}).width, tokens::btn_compact_min_width);
}

TEST(Button, IconOnlyIsSquareAndFallsBackToTextWithoutIcon) {
  Scene scene("播放");
  scene.button->setAutoIcon(false);
  scene.button->setIconOnly(true);
  const double textWidth = scene.button->measure({}).width;
  EXPECT_GT(textWidth, tokens::btn_height);
  scene.button->setIcon(IconId::PlayerPlay);
  EXPECT_EQ(scene.button->measure({}), (Size{tokens::btn_height, tokens::btn_height}));
}

TEST(Button, SecondaryPrimaryDangerUseTheirTokensInEachState) {
  struct Case {
    ButtonVariant variant;
    Color normal;
    Color hover;
    Color active;
  };
  for (const ThemeName theme : {ThemeName::Dark, ThemeName::Light}) {
    const auto& c = themeTokens(theme);
    const Case cases[] = {{ButtonVariant::Secondary, c.button_secondary_bg, c.button_secondary_hover_bg, c.button_secondary_active_bg},
                          {ButtonVariant::Primary, c.accent, c.accent_hover, c.accent_active},
                          {ButtonVariant::Danger, c.button_danger_bg, c.button_danger_hover_bg, c.button_danger_active_bg}};
    for (const Case& item : cases) {
      Scene scene("好", theme);
      scene.button->setAutoIcon(false);
      scene.button->setVariant(item.variant);
      const Color window = c.content_bg;
      expectColor(pixelAt(scene.render(), 13, 21), compose(item.normal, window));
      scene.root.mouseMove(Point{20, 20}, 0);
      expectColor(pixelAt(scene.render(), 13, 21), compose(item.hover, window));
      scene.root.mouseDown(Point{20, 20}, MouseButton::Left, 0);
      expectColor(pixelAt(scene.render(), 13, 21), compose(item.active, window));
    }
  }
}

TEST(Button, GhostIsTransparentUntilHoveredAndFollowsSelectedState) {
  Scene scene("好");
  scene.button->setAutoIcon(false);
  scene.button->setVariant(ButtonVariant::Ghost);
  const auto& c = themeTokens(ThemeName::Dark);
  expectColor(pixelAt(scene.render(), 13, 21), c.content_bg);
  scene.root.mouseMove(Point{20, 20}, 0);
  expectColor(pixelAt(scene.render(), 13, 21), compose(c.ghost_hover_background, c.content_bg));
  scene.root.mouseLeave();
  scene.button->setToggled(true);
  expectColor(pixelAt(scene.render(), 13, 21), compose(c.ghost_selected_background, c.content_bg));
  scene.root.mouseMove(Point{20, 20}, 0);
  expectColor(pixelAt(scene.render(), 13, 21), compose(c.ghost_selected_background, c.content_bg));
}

TEST(Button, ToggledNonGhostUsesSelectedOptionColors) {
  Scene scene("好");
  scene.button->setAutoIcon(false);
  scene.button->setToggled(true);
  const auto& c = themeTokens(ThemeName::Dark);
  expectColor(pixelAt(scene.render(), 13, 21), compose(c.option_selected_bg, c.content_bg));
  scene.root.mouseMove(Point{20, 20}, 0);
  expectColor(pixelAt(scene.render(), 13, 21), compose(c.option_selected_hover_bg, c.content_bg));
  scene.root.mouseDown(Point{20, 20}, MouseButton::Left, 0);
  expectColor(pixelAt(scene.render(), 13, 21), compose(c.option_selected_active_bg, c.content_bg));
}

TEST(Button, DisabledOverridesEveryVariantAndToggle) {
  const auto& c = themeTokens(ThemeName::Dark);
  for (const ButtonVariant variant : {ButtonVariant::Secondary, ButtonVariant::Primary, ButtonVariant::Danger, ButtonVariant::Ghost}) {
    Scene scene("好");
    scene.button->setAutoIcon(false);
    scene.button->setVariant(variant);
    scene.button->setToggled(true);
    scene.button->setEnabled(false);
    scene.root.mouseMove(Point{20, 20}, 0);
    expectColor(pixelAt(scene.render(), 13, 21), compose(c.disabled_bg, c.content_bg));
  }
}

TEST(Button, LinkWrapsInNarrowWidthAndHasNoBackground) {
  Scene scene("这是一个很长很长很长很长的目录路径链接");
  scene.button->setVariant(ButtonVariant::Link);
  const Size wide = scene.button->measure({});
  const Size narrow = scene.button->measure(Size{60, 0});
  EXPECT_LE(narrow.width, 60.5);
  EXPECT_GT(narrow.height, wide.height);
  expectColor(pixelAt(scene.render(), 8, 8), themeTokens(ThemeName::Dark).content_bg);
}

TEST(Button, LinkIgnoresIconsAndKeepsKeyboardClick) {
  Scene scene("保存");
  scene.button->setVariant(ButtonVariant::Link);
  const double width = scene.button->measure({}).width;
  scene.button->setIcon(IconId::Plus);
  EXPECT_EQ(scene.button->measure({}).width, width);
  int clicks = 0;
  scene.button->setOnClick([&clicks] { ++clicks; });
  scene.root.keyDown(0x09, 0, false);
  scene.root.keyDown(0x0D, 0, false);
  EXPECT_EQ(clicks, 1);
}
