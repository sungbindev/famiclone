/*
 * ------------------------------------------------------------------------
 * 名称：split-view-test.cpp
 * 说明：分栏视图的测试：左栏、分隔条、主区的位置，拖动与方向键调整宽度并限制在范围内，悬停显示的延迟，光标与焦点。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-19
 * 备注：分栏视图放在 (0,0)、宽 600、高 200，默认左栏宽 220；子控件顺序是“左栏滚动区域、主区、分隔条”。
 * ------------------------------------------------------------------------
 */

#include <gtest/gtest.h>

#include <memory>

#include "base/theme-tokens.hpp"
#include "rujian/ui/offscreen.hpp"
#include "rujian/ui/root.hpp"
#include "rujian/ui/split-view.hpp"

using namespace rujian::ui;

namespace {

constexpr int kVkTab = 0x09;
constexpr int kVkEnd = 0x23;
constexpr int kVkHome = 0x24;
constexpr int kVkLeft = 0x25;
constexpr int kVkRight = 0x27;

/** 固定高度的内容。 */
class Block final : public Widget {
 public:
  explicit Block(double height) : height_(height) {}
  Size measure(Size available) override { return Size{available.width, height_}; }

 private:
  double height_;
};

struct Scene {
  Root root{ThemeName::Dark, 1.0};
  SplitView* view{nullptr};

  explicit Scene(bool animated = false) {
    auto created = std::make_unique<SplitView>();
    view = created.get();
    view->setSide(std::make_unique<Block>(300));
    view->setMain(std::make_unique<Block>(80));
    root.setContent(std::move(created));
    root.setAnimationsEnabled(animated);
    root.layout(Size{600, 200});
  }

  [[nodiscard]] const Rect& side() const { return view->children()[0]->bounds(); }
  [[nodiscard]] const Rect& main() const { return view->children()[1]->bounds(); }
  [[nodiscard]] const Rect& splitter() const { return view->children()[2]->bounds(); }
  [[nodiscard]] Widget* splitterWidget() const { return view->children()[2].get(); }

  /** 渲染后分隔条中间（x=220，分割线之外、分隔条之内）那个像素的 RGB。 */
  [[nodiscard]] std::uint32_t splitterPixel() {
    root.layout(Size{600, 200});
    OffscreenSurface surface(Size{600, 200}, 1.0);
    EXPECT_TRUE(surface.render(root, themeTokens(root.theme()).content_bg));
    const std::size_t i = (static_cast<std::size_t>(100) * 600 + 220) * 4;
    return (static_cast<std::uint32_t>(surface.pixels()[i + 2]) << 16) | (static_cast<std::uint32_t>(surface.pixels()[i + 1]) << 8) | surface.pixels()[i];
  }
};

}  // namespace

TEST(SplitView, DefaultLayoutPutsTheSplitterOnTheSideEdge) {
  Scene s;
  EXPECT_DOUBLE_EQ(s.view->sideWidth(), 220);
  // 左栏宽 220，含右边的 1 像素分割线
  EXPECT_DOUBLE_EQ(s.side().width, 219);
  EXPECT_DOUBLE_EQ(s.side().height, 200);
  EXPECT_DOUBLE_EQ(s.splitter().x, 218);
  EXPECT_DOUBLE_EQ(s.splitter().width, 4);
  EXPECT_DOUBLE_EQ(s.main().x, 230);
  EXPECT_DOUBLE_EQ(s.main().width, 370);
  EXPECT_DOUBLE_EQ(s.main().height, 200);
}

TEST(SplitView, MeasureIsTheTallerOfSideAndMain) {
  Scene s;
  EXPECT_DOUBLE_EQ(s.view->measure(Size{600, 0}).height, 300);
}

TEST(SplitView, DraggingTheSplitterResizesTheSide) {
  Scene s;
  s.root.mouseMove(Point{220, 100}, 0);
  s.root.mouseDown(Point{220, 100}, MouseButton::Left, 0);
  s.root.mouseMove(Point{270, 100}, 0);
  EXPECT_DOUBLE_EQ(s.view->sideWidth(), 270);
  EXPECT_DOUBLE_EQ(s.main().x, 280);
  s.root.mouseMove(Point{900, 100}, 0);
  EXPECT_DOUBLE_EQ(s.view->sideWidth(), 480);
  s.root.mouseMove(Point{-500, 100}, 0);
  EXPECT_DOUBLE_EQ(s.view->sideWidth(), 160);
  s.root.mouseUp(Point{-500, 100}, MouseButton::Left, 0);
  s.root.mouseMove(Point{300, 100}, 0);
  EXPECT_DOUBLE_EQ(s.view->sideWidth(), 160);
}

TEST(SplitView, ArrowKeysMoveByTenAndHomeEndJumpToTheLimits) {
  Scene s;
  s.root.mouseDown(Point{220, 100}, MouseButton::Left, 0);
  s.root.mouseUp(Point{220, 100}, MouseButton::Left, 0);
  s.root.keyDown(kVkLeft, 0, false);
  EXPECT_DOUBLE_EQ(s.view->sideWidth(), 210);
  s.root.keyDown(kVkRight, 0, false);
  s.root.keyDown(kVkRight, 0, false);
  EXPECT_DOUBLE_EQ(s.view->sideWidth(), 230);
  s.root.keyDown(kVkHome, 0, false);
  EXPECT_DOUBLE_EQ(s.view->sideWidth(), 160);
  s.root.keyDown(kVkEnd, 0, false);
  EXPECT_DOUBLE_EQ(s.view->sideWidth(), 480);
}

TEST(SplitView, RangeLimitsTheWidthImmediately) {
  Scene s;
  s.view->setSideWidthRange(100, 200);
  EXPECT_DOUBLE_EQ(s.view->sideWidth(), 200);
  s.view->setSideWidth(50);
  EXPECT_DOUBLE_EQ(s.view->sideWidth(), 100);
}

TEST(SplitView, SplitterIsFocusableAndUsesTheResizeCursor) {
  Scene s;
  EXPECT_TRUE(s.splitterWidget()->focusable());
  EXPECT_EQ(s.splitterWidget()->cursor(), CursorKind::SizeWE);
  s.root.keyDown(kVkTab, 0, false);
  EXPECT_TRUE(s.splitterWidget()->focused());
}

TEST(SplitView, HoverShowsImmediatelyWithoutAnimation) {
  Scene s;
  const std::uint32_t background = s.splitterPixel();
  const std::uint32_t accent = themeTokens(ThemeName::Dark).focus_border.argb & 0xFFFFFFu;
  EXPECT_NE(background, accent);
  s.root.mouseMove(Point{220, 100}, 0);
  EXPECT_EQ(s.splitterPixel(), accent);
  s.root.mouseMove(Point{50, 100}, 0);
  EXPECT_EQ(s.splitterPixel(), background);
}

TEST(SplitView, HoverWaitsForTheDelayWhenAnimated) {
  Scene s(true);
  const std::uint32_t background = s.splitterPixel();
  const std::uint32_t accent = themeTokens(ThemeName::Dark).focus_border.argb & 0xFFFFFFu;
  s.root.mouseMove(Point{220, 100}, 0);
  ASSERT_FALSE(s.root.pendingTimers().empty());
  EXPECT_EQ(s.splitterPixel(), background);
  // 到时间之前移开：计时取消，之后到时间的计时器不会让分隔条显示
  s.root.mouseMove(Point{500, 100}, 0);
  for (const unsigned id : s.root.pendingTimers()) s.root.fireTimer(id);
  EXPECT_EQ(s.splitterPixel(), background);
  s.root.mouseMove(Point{220, 100}, 0);
  for (const unsigned id : s.root.pendingTimers()) s.root.fireTimer(id);
  EXPECT_EQ(s.splitterPixel(), accent);
}

TEST(SplitView, DraggingShowsTheColorWithoutWaiting) {
  Scene s(true);
  const std::uint32_t accent = themeTokens(ThemeName::Dark).focus_border.argb & 0xFFFFFFu;
  s.root.mouseMove(Point{220, 100}, 0);
  s.root.mouseDown(Point{220, 100}, MouseButton::Left, 0);
  EXPECT_EQ(s.splitterPixel(), accent);
}
