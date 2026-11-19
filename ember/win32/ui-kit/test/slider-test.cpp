/*
 * ------------------------------------------------------------------------
 * 名称：slider-test.cpp
 * 说明：滑块的测试：在轨道上按下跳到指针位置并可拖动、量程与步长取整、方向键与翻页键、只有中间的输入框区域响应鼠标、禁用。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-19
 * 备注：滑块放在 (10,10)，宽 96、高 22；响应鼠标的区域是 y 从 15 到 27；滑块中心的活动范围是 x 从 16 到 100。
 * ------------------------------------------------------------------------
 */

#include <gtest/gtest.h>

#include <memory>
#include <vector>

#include "rujian/ui/root.hpp"
#include "rujian/ui/slider.hpp"

using namespace rujian::ui;

namespace {

constexpr int kVkTab = 0x09;
constexpr int kVkPageUp = 0x21;
constexpr int kVkPageDown = 0x22;
constexpr int kVkEnd = 0x23;
constexpr int kVkHome = 0x24;
constexpr int kVkLeft = 0x25;
constexpr int kVkUp = 0x26;
constexpr int kVkRight = 0x27;
constexpr int kVkDown = 0x28;

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

struct Scene {
  Root root{ThemeName::Dark, 1.0};
  Slider* slider{nullptr};
  std::vector<double> changes;

  Scene() {
    auto holder = std::make_unique<Holder>();
    auto created = std::make_unique<Slider>();
    slider = created.get();
    holder->addChild(std::move(created));
    root.setContent(std::move(holder));
    slider->setOnChange([this](double value) { changes.push_back(value); });
    root.layout(Size{200, 60});
  }

  void press(double x, double y = 21) {
    root.mouseMove(Point{x, y}, 0);
    root.mouseDown(Point{x, y}, MouseButton::Left, 0);
  }
  void release(double x, double y = 21) { root.mouseUp(Point{x, y}, MouseButton::Left, 0); }
};

}  // namespace

TEST(Slider, MeasureIsTheFixedTokenSize) {
  Scene s;
  const Size size = s.slider->measure({});
  EXPECT_DOUBLE_EQ(size.width, 96);
  EXPECT_DOUBLE_EQ(size.height, 22);
}

TEST(Slider, PressingOnTheTrackJumpsToThePointerAndReports) {
  Scene s;
  s.press(58);  // 16 与 100 的正中间
  s.release(58);
  EXPECT_DOUBLE_EQ(s.slider->value(), 50);
  EXPECT_EQ(s.changes, (std::vector<double>{50}));
}

TEST(Slider, DraggingFollowsThePointerAndClampsAtTheEnds) {
  Scene s;
  s.press(16);
  s.root.mouseMove(Point{37, 21}, 0);
  EXPECT_DOUBLE_EQ(s.slider->value(), 25);
  s.root.mouseMove(Point{300, 90}, 0);  // 拖到控件外面也继续跟随，并停在最大值
  EXPECT_DOUBLE_EQ(s.slider->value(), 100);
  s.root.mouseMove(Point{-50, 21}, 0);
  EXPECT_DOUBLE_EQ(s.slider->value(), 0);
  s.release(-50);
  s.root.mouseMove(Point{58, 21}, 0);
  EXPECT_DOUBLE_EQ(s.slider->value(), 0) << "松开之后不再跟随";
}

TEST(Slider, OnlyTheFieldInTheMiddleRespondsToTheMouse) {
  Scene s;
  s.press(58, 11);  // 控件上方的空白
  s.release(58, 11);
  s.press(58, 31);  // 下方的空白
  s.release(58, 31);
  EXPECT_DOUBLE_EQ(s.slider->value(), 0);
  EXPECT_TRUE(s.changes.empty());
}

TEST(Slider, KeysStepPageAndJumpToTheEnds) {
  Scene s;
  s.root.keyDown(kVkTab, 0, false);
  ASSERT_TRUE(s.slider->focused());
  s.root.keyDown(kVkRight, 0, false);
  s.root.keyDown(kVkUp, 0, false);
  EXPECT_DOUBLE_EQ(s.slider->value(), 2);
  s.root.keyDown(kVkLeft, 0, false);
  s.root.keyDown(kVkDown, 0, false);
  s.root.keyDown(kVkDown, 0, false);
  EXPECT_DOUBLE_EQ(s.slider->value(), 0) << "到最小值后不再减";
  s.root.keyDown(kVkPageUp, 0, false);
  EXPECT_DOUBLE_EQ(s.slider->value(), 10);
  s.root.keyDown(kVkPageDown, 0, false);
  s.root.keyDown(kVkEnd, 0, false);
  EXPECT_DOUBLE_EQ(s.slider->value(), 100);
  s.root.keyDown(kVkHome, 0, false);
  EXPECT_DOUBLE_EQ(s.slider->value(), 0);
  // 只有真的变了才回调：2 次增、3 次减（其中 1 次已经在最小值）…不重复上报同一个值
  for (std::size_t i = 1; i < s.changes.size(); ++i) EXPECT_NE(s.changes[i], s.changes[i - 1]);
}

TEST(Slider, SetValueNormalizesWithoutReporting) {
  Scene s;
  s.slider->setRange(0, 10);
  s.slider->setStep(2);
  s.slider->setValue(5.2);
  EXPECT_DOUBLE_EQ(s.slider->value(), 6);
  s.slider->setValue(99);
  EXPECT_DOUBLE_EQ(s.slider->value(), 10);
  s.slider->setValue(-3);
  EXPECT_DOUBLE_EQ(s.slider->value(), 0);
  EXPECT_TRUE(s.changes.empty());
}

TEST(Slider, StepSnapsThePointerValue) {
  Scene s;
  s.slider->setRange(0, 10);
  s.slider->setStep(5);
  s.press(40);  // 约 28.6%，向最近的步长取整为 0 或 5 或 10
  s.release(40);
  EXPECT_DOUBLE_EQ(s.slider->value(), 5);
}

TEST(Slider, DisabledIgnoresMouseAndKeysAndIsSkippedByTab) {
  Scene s;
  s.slider->setEnabled(false);
  s.press(58);
  s.release(58);
  s.root.keyDown(kVkTab, 0, false);
  s.root.keyDown(kVkRight, 0, false);
  EXPECT_FALSE(s.slider->focused());
  EXPECT_DOUBLE_EQ(s.slider->value(), 0);
}

TEST(Slider, CursorIsHandOverTheFieldOnly) {
  Scene s;
  s.root.mouseMove(Point{58, 21}, 0);
  EXPECT_EQ(s.slider->cursor(), CursorKind::Hand);
  s.root.mouseMove(Point{58, 12}, 0);
  EXPECT_EQ(s.slider->cursor(), CursorKind::Arrow);
}

TEST(Slider, FixedRangeAtTheLeftWhenMaxIsNotAboveMin) {
  Scene s;
  s.slider->setRange(5, 5);
  s.slider->setValue(5);
  s.press(90);
  s.release(90);
  EXPECT_DOUBLE_EQ(s.slider->value(), 5);
}
