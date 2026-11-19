/*
 * ------------------------------------------------------------------------
 * 名称：offscreen-test.cpp
 * 说明：离屏绘制的测试：像素级检查画布的对齐规则、边框、裁剪、按钮的各状态颜色、图标与 PNG 输出。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-19
 * 备注：文字的像素受本机字体影响，这里只检查文字区域“有内容”，颜色断言只取不含文字的位置。
 * ------------------------------------------------------------------------
 */

#include <gtest/gtest.h>

#include <cstdint>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <memory>
#include <stdexcept>
#include <vector>

#include "base/theme-tokens.hpp"
#include "rujian/ui/button.hpp"
#include "rujian/ui/offscreen.hpp"
#include "rujian/ui/root.hpp"

using namespace rujian::ui;

namespace {

/** 在固定位置画一个盒子的控件，用来直接测试画布。 */
class BoxWidget final : public Widget {
 public:
  BoxWidget(Rect rect, BoxStyle style) : rect_(rect), style_(style) {}
  void arrange(const Rect&) override { Widget::arrange(rect_); }
  void paint(Canvas& canvas) override { canvas.drawBox(rect_, style_); }

 private:
  Rect rect_;
  BoxStyle style_;
};

/** 读像素：返回 0xAARRGGBB（预乘，不透明时等于原色）。 */
std::uint32_t pixelAt(const std::vector<std::uint8_t>& data, int width, int x, int y) {
  const std::size_t i = (static_cast<std::size_t>(y) * width + x) * 4;
  return (static_cast<std::uint32_t>(data[i + 3]) << 24) | (static_cast<std::uint32_t>(data[i + 2]) << 16) | (static_cast<std::uint32_t>(data[i + 1]) << 8) | data[i];
}

void expectColor(std::uint32_t actual, Color expected, int tolerance = 1) {
  EXPECT_NEAR(static_cast<int>((actual >> 16) & 0xFF), expected.r(), tolerance);
  EXPECT_NEAR(static_cast<int>((actual >> 8) & 0xFF), expected.g(), tolerance);
  EXPECT_NEAR(static_cast<int>(actual & 0xFF), expected.b(), tolerance);
}

/** 渲染单个盒子并返回像素。 */
std::vector<std::uint8_t> renderBox(Size size, double scale, Rect rect, BoxStyle style, Color background) {
  Root root(ThemeName::Dark, scale);
  root.setContent(std::make_unique<BoxWidget>(rect, style));
  root.layout(size);
  OffscreenSurface surface(size, scale);
  EXPECT_TRUE(surface.render(root, background));
  return surface.pixels();
}

constexpr Color kBackground = Color::rgba(0, 0, 0);
constexpr Color kRed = Color::rgba(255, 0, 0);
constexpr Color kBlue = Color::rgba(0, 0, 255);

}  // namespace

TEST(Offscreen, PixelSizeFollowsScale) {
  OffscreenSurface surface(Size{100, 40}, 1.25);
  EXPECT_EQ(surface.pixelWidth(), 125);
  EXPECT_EQ(surface.pixelHeight(), 50);
  EXPECT_EQ(surface.pixels().size(), 125u * 50u * 4u);
}

TEST(Offscreen, RejectsEmptySize) { EXPECT_THROW(OffscreenSurface(Size{0, 10}, 1.0), std::runtime_error); }

TEST(Canvas, FilledBoxCoversExactlyItsSnappedRectAtScaleOne) {
  const auto data = renderBox(Size{20, 20}, 1.0, Rect{5, 5, 10, 10}, BoxStyle{.fill = kRed}, kBackground);
  expectColor(pixelAt(data, 20, 5, 5), kRed, 0);
  expectColor(pixelAt(data, 20, 14, 14), kRed, 0);
  expectColor(pixelAt(data, 20, 4, 5), kBackground, 0);
  expectColor(pixelAt(data, 20, 15, 14), kBackground, 0);
  expectColor(pixelAt(data, 20, 14, 15), kBackground, 0);
}

TEST(Canvas, FractionalScaleSnapsEdgesToDevicePixels) {
  // 缩放 1.25：逻辑矩形 (4,4,8,8) 的设备边缘是 5 与 15，没有半透明的边缘像素
  const auto data = renderBox(Size{20, 20}, 1.25, Rect{4, 4, 8, 8}, BoxStyle{.fill = kRed}, kBackground);
  for (int x = 5; x < 15; ++x) expectColor(pixelAt(data, 25, x, 8), kRed, 0);
  expectColor(pixelAt(data, 25, 4, 8), kBackground, 0);
  expectColor(pixelAt(data, 25, 15, 8), kBackground, 0);
}

TEST(Canvas, BorderIsDrawnInsideTheBoxWithMinimumOneDevicePixel) {
  const BoxStyle style{.fill = kBlue, .borderWidth = 1, .borderColor = kRed};
  const auto data = renderBox(Size{20, 20}, 1.0, Rect{4, 4, 12, 12}, style, kBackground);
  expectColor(pixelAt(data, 20, 4, 10), kRed, 0);
  expectColor(pixelAt(data, 20, 15, 10), kRed, 0);
  expectColor(pixelAt(data, 20, 10, 4), kRed, 0);
  expectColor(pixelAt(data, 20, 10, 15), kRed, 0);
  expectColor(pixelAt(data, 20, 5, 10), kBlue, 0);
  expectColor(pixelAt(data, 20, 3, 10), kBackground, 0);
}

TEST(Canvas, BorderStaysOneDevicePixelAtFractionalScale) {
  const BoxStyle style{.fill = kBlue, .borderWidth = 1, .borderColor = kRed};
  const auto data = renderBox(Size{20, 20}, 1.5, Rect{4, 4, 12, 12}, style, kBackground);
  // 设备像素：盒子 6..24，边框宽 1 个设备像素（不是 1.5）
  expectColor(pixelAt(data, 30, 6, 15), kRed, 0);
  expectColor(pixelAt(data, 30, 7, 15), kBlue, 0);
  expectColor(pixelAt(data, 30, 23, 15), kRed, 0);
}

TEST(Canvas, RoundedCornerLeavesCornerPixelUnfilledButFillsCenter) {
  const auto data = renderBox(Size{40, 40}, 1.0, Rect{4, 4, 32, 32}, BoxStyle{.fill = kRed, .radius = 8}, kBackground);
  expectColor(pixelAt(data, 40, 4, 4), kBackground, 8);
  expectColor(pixelAt(data, 40, 20, 20), kRed, 0);
  expectColor(pixelAt(data, 40, 20, 4), kRed, 0);
}

TEST(Canvas, ClipLimitsDrawing) {
  class ClippedBox final : public Widget {
   public:
    void paint(Canvas& canvas) override {
      canvas.pushClip(Rect{0, 0, 10, 20});
      canvas.fillRect(Rect{0, 0, 20, 20}, kRed);
      canvas.popClip();
    }
  };
  Root root(ThemeName::Dark, 1.0);
  root.setContent(std::make_unique<ClippedBox>());
  root.layout(Size{20, 20});
  OffscreenSurface surface(Size{20, 20}, 1.0);
  ASSERT_TRUE(surface.render(root, kBackground));
  const auto data = surface.pixels();
  expectColor(pixelAt(data, 20, 9, 10), kRed, 0);
  expectColor(pixelAt(data, 20, 10, 10), kBackground, 0);
}

TEST(Canvas, LineIsAtLeastOneDevicePixelThick) {
  class LineWidget final : public Widget {
   public:
    void paint(Canvas& canvas) override { canvas.drawLine(Rect{2, 5, 16, 0.2}, kRed); }
  };
  Root root(ThemeName::Dark, 1.0);
  root.setContent(std::make_unique<LineWidget>());
  root.layout(Size{20, 10});
  OffscreenSurface surface(Size{20, 10}, 1.0);
  ASSERT_TRUE(surface.render(root, kBackground));
  const auto data = surface.pixels();
  expectColor(pixelAt(data, 20, 10, 5), kRed, 0);
  expectColor(pixelAt(data, 20, 10, 4), kBackground, 0);
  expectColor(pixelAt(data, 20, 10, 6), kBackground, 0);
}

namespace {

/** 页面：把一个按钮摆在 (10,10)。 */
class Page final : public Widget {
 public:
  void arrange(const Rect& bounds) override {
    Widget::arrange(bounds);
    for (const auto& child : children()) {
      const Size size = child->measure({});
      child->arrange(Rect{10, 10, size.width, size.height});
    }
  }
};

struct ButtonScene {
  Root root;
  Button* button;
  ButtonScene(ThemeName theme, double scale) : root(theme, scale), button(nullptr) {
    auto page = std::make_unique<Page>();
    auto created = std::make_unique<Button>("确定");
    created->setAutoIcon(false);
    button = created.get();
    page->addChild(std::move(created));
    root.setContent(std::move(page));
    root.layout(Size{120, 44});
  }
  std::vector<std::uint8_t> render(double scale, ThemeName theme) {
    OffscreenSurface surface(Size{120, 44}, scale);
    EXPECT_TRUE(surface.render(root, themeTokens(theme).content_bg));
    return surface.pixels();
  }
};

/** 按钮内部靠左、不含文字的取样点（内边距里）。 */
constexpr int kSampleX = 12;
constexpr int kSampleY = 21;

}  // namespace

TEST(ButtonRendering, NormalHoverPressedUseTheSecondaryTokens) {
  for (const ThemeName theme : {ThemeName::Dark, ThemeName::Light}) {
    const auto& colors = themeTokens(theme);
    const Color window = colors.content_bg;
    ButtonScene scene(theme, 1.0);
    expectColor(pixelAt(scene.render(1.0, theme), 120, kSampleX, kSampleY), compose(colors.button_secondary_bg, window));

    scene.root.mouseMove(Point{20, 20}, 0);
    expectColor(pixelAt(scene.render(1.0, theme), 120, kSampleX, kSampleY), compose(colors.button_secondary_hover_bg, window));

    scene.root.mouseDown(Point{20, 20}, MouseButton::Left, 0);
    expectColor(pixelAt(scene.render(1.0, theme), 120, kSampleX, kSampleY), compose(colors.button_secondary_active_bg, window));
  }
}

TEST(ButtonRendering, PrimaryUsesAccentTokens) {
  ButtonScene scene(ThemeName::Dark, 1.0);
  scene.button->setVariant(ButtonVariant::Primary);
  const auto& colors = themeTokens(ThemeName::Dark);
  expectColor(pixelAt(scene.render(1.0, ThemeName::Dark), 120, kSampleX, kSampleY), compose(colors.accent, colors.content_bg));
}

TEST(ButtonRendering, DisabledUsesDisabledTokens) {
  ButtonScene scene(ThemeName::Dark, 1.0);
  scene.button->setEnabled(false);
  const auto& colors = themeTokens(ThemeName::Dark);
  expectColor(pixelAt(scene.render(1.0, ThemeName::Dark), 120, kSampleX, kSampleY), compose(colors.disabled_bg, colors.content_bg));
}

TEST(ButtonRendering, KeyboardFocusDrawsRingOffsetByOutlineOffset) {
  ButtonScene scene(ThemeName::Dark, 1.0);
  scene.root.keyDown(0x09, 0, false);
  const auto data = scene.render(1.0, ThemeName::Dark);
  const auto& colors = themeTokens(ThemeName::Dark);
  // 按钮左边缘 x=10，outline-offset: 1px、宽 1px：环正好占满 x=8 这一列
  expectColor(pixelAt(data, 120, 8, 21), colors.focus_border, 1);
  expectColor(pixelAt(data, 120, 9, 21), colors.content_bg);
  expectColor(pixelAt(data, 120, 7, 21), colors.content_bg);
}

TEST(ButtonRendering, FocusRingCoversExactlyOneDevicePixelAtScaleOnePointFive) {
  ButtonScene scene(ThemeName::Dark, 1.5);
  scene.root.keyDown(0x09, 0, false);
  const auto data = scene.render(1.5, ThemeName::Dark);
  const auto& colors = themeTokens(ThemeName::Dark);
  // 按钮左边缘在设备像素 15，偏移 1.5 向零取整为 1 个设备像素，线宽 1 个设备像素：整个像素 13 被覆盖
  expectColor(pixelAt(data, 180, 13, 32), colors.focus_border, 1);
  expectColor(pixelAt(data, 180, 12, 32), colors.content_bg);
  expectColor(pixelAt(data, 180, 14, 32), colors.content_bg);
}

TEST(ButtonRendering, MouseFocusDoesNotDrawRing) {
  ButtonScene scene(ThemeName::Dark, 1.0);
  scene.root.mouseDown(Point{20, 20}, MouseButton::Left, 0);
  scene.root.mouseUp(Point{20, 20}, MouseButton::Left, 0);
  const auto data = scene.render(1.0, ThemeName::Dark);
  expectColor(pixelAt(data, 120, 8, 21), themeTokens(ThemeName::Dark).content_bg);
}

TEST(ButtonRendering, LabelDrawsSomethingAndThemeSwitchChangesBackground) {
  ButtonScene scene(ThemeName::Dark, 1.0);
  const auto dark = scene.render(1.0, ThemeName::Dark);
  // 文字区域应有与按钮底色不同的像素
  const std::uint32_t base = pixelAt(dark, 120, kSampleX, kSampleY);
  bool found = false;
  for (int x = 18; x < 40 && !found; ++x) {
    for (int y = 14; y < 30 && !found; ++y) found = pixelAt(dark, 120, x, y) != base;
  }
  EXPECT_TRUE(found);
  scene.root.setTheme(ThemeName::Light);
  EXPECT_NE(pixelAt(scene.render(1.0, ThemeName::Light), 120, kSampleX, kSampleY), base);
}

TEST(ButtonRendering, IconDrawsAndScalesWithDpi) {
  ButtonScene scene(ThemeName::Dark, 1.5);
  scene.button->setIcon(IconId::Check);
  scene.root.layout(Size{120, 44});
  const auto data = scene.render(1.5, ThemeName::Dark);
  EXPECT_EQ(data.size(), 180u * 66u * 4u);
}

TEST(Offscreen, SavePngWritesAValidPngFile) {
  ButtonScene scene(ThemeName::Dark, 1.0);
  OffscreenSurface surface(Size{120, 44}, 1.0);
  ASSERT_TRUE(surface.render(scene.root, Color::rgba(30, 30, 30)));
  const auto path = std::filesystem::temp_directory_path() / "rujian-ui-offscreen-test.png";
  ASSERT_TRUE(surface.savePng(path));
  std::ifstream file(path, std::ios::binary);
  unsigned char signature[8]{};
  file.read(reinterpret_cast<char*>(signature), 8);
  const unsigned char expected[8] = {0x89, 'P', 'N', 'G', 0x0D, 0x0A, 0x1A, 0x0A};
  EXPECT_EQ(std::memcmp(signature, expected, 8), 0);
  file.close();
  std::filesystem::remove(path);
}

TEST(Offscreen, SavePngFailsForUnwritablePath) {
  OffscreenSurface surface(Size{10, 10}, 1.0);
  EXPECT_FALSE(surface.savePng(std::filesystem::temp_directory_path() / "no-such-directory-rujian" / "x.png"));
}
