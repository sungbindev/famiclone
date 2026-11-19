/*
 * ------------------------------------------------------------------------
 * 名称：pad-diagram-test.cpp
 * 说明：手柄图的测试：按宽度等比缩放并水平居中（最大 560）、键的位置与大小（圆形键宽高相等）、键的内容与选中、学习状态、整张图置灰、鼠标与键盘点击只在可用时触发、Tab 逐个到达。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-20
 * 备注：用只有两个键（一个矩形键、一个圆形键）的简化版面；这个控件不在网页版组件库里，没有像素基线。
 * ------------------------------------------------------------------------
 */

#include <gtest/gtest.h>

#include <memory>
#include <string>
#include <vector>

#include "rujian/ui/pad-diagram.hpp"
#include "rujian/ui/root.hpp"

using namespace rujian::ui;

namespace {

constexpr int kVkTab = 0x09;
constexpr int kVkReturn = 0x0D;
constexpr int kVkSpace = 0x20;

PadDiagramLayout simpleLayout() {
  PadDiagramLayout layout;
  layout.canvasWidth = 560;
  layout.canvasHeight = 220;
  layout.body = Rect{4, 4, 552, 212};
  layout.bodyRadius = 36;
  layout.crossCenter = Rect{80, 88, 60, 44};
  layout.keys = {{"up", false, 110, 66, 60, 44}, {"a", true, 500, 140, 64, 64}};
  return layout;
}

struct Scene {
  Root root{ThemeName::Dark, 1.0};
  PadDiagram* diagram{nullptr};
  std::vector<std::string> picks;

  explicit Scene(double width = 700) {
    auto created = std::make_unique<PadDiagram>(simpleLayout());
    diagram = created.get();
    diagram->setOnPick([this](const std::string& id) { picks.push_back(id); });
    root.setContent(std::move(created));
    root.layout(Size{width, 300});
    diagram->setHotspots({{"up", "上", "↑", false, "绑定键盘按键：上（当前：↑）", false, false}, {"a", "A", "未绑定", true, "绑定键盘按键：A（当前：未绑定）", false, false}});
  }

  void click(const Widget& widget) {
    const auto& box = widget.bounds();
    const Point at{box.x + box.width / 2, box.y + box.height / 2};
    root.mouseMove(at, 0);
    root.mouseDown(at, MouseButton::Left, 0);
    root.mouseUp(at, MouseButton::Left, 0);
  }
};

}  // namespace

TEST(PadDiagram, MeasuresTheCanvasAspectRatioAndNeverGrowsBeyond560) {
  Scene s;
  EXPECT_DOUBLE_EQ(s.diagram->measure(Size{700, 0}).height, 560.0 * 220 / 560);
  EXPECT_DOUBLE_EQ(s.diagram->measure(Size{280, 0}).height, 280.0 * 220 / 560);
  EXPECT_DOUBLE_EQ(s.diagram->measure(Size{280, 0}).width, 280.0);
}

TEST(PadDiagram, KeysScaleWithTheWidthAndAreCenteredInAWiderArea) {
  Scene wide(700);
  // 绘制区 560 宽，水平居中：左边留 70
  const Rect up = wide.diagram->key("up")->bounds();
  EXPECT_DOUBLE_EQ(up.width, 60);
  EXPECT_DOUBLE_EQ(up.height, 44);
  EXPECT_DOUBLE_EQ(up.x + up.width / 2, 70 + 110);
  EXPECT_DOUBLE_EQ(up.y + up.height / 2, 66);
  const Rect a = wide.diagram->key("a")->bounds();
  EXPECT_DOUBLE_EQ(a.width, 64);
  EXPECT_DOUBLE_EQ(a.height, 64);

  Scene narrow(280);
  const Rect small = narrow.diagram->key("up")->bounds();
  EXPECT_DOUBLE_EQ(small.width, 30);
  EXPECT_DOUBLE_EQ(small.height, 22);
  EXPECT_DOUBLE_EQ(small.x + small.width / 2, 55);
  EXPECT_DOUBLE_EQ(narrow.diagram->key("a")->bounds().width, narrow.diagram->key("a")->bounds().height);
}

TEST(PadDiagram, KeyContentAndStateFollowTheHotspots) {
  Scene s;
  EXPECT_EQ(s.diagram->key("up")->hotspot().role, "上");
  EXPECT_EQ(s.diagram->key("up")->hotspot().current, "↑");
  EXPECT_TRUE(s.diagram->key("a")->hotspot().unbound);
  EXPECT_TRUE(s.diagram->key("a")->round());
  EXPECT_FALSE(s.diagram->key("up")->round());
  s.diagram->setHotspots({{"up", "上", "请按键", false, "", true, true}});
  EXPECT_TRUE(s.diagram->key("up")->selected());
  EXPECT_TRUE(s.diagram->key("up")->learning());
  EXPECT_EQ(s.diagram->key("up")->hotspot().current, "请按键");
  // 不认识的键忽略
  s.diagram->setHotspots({{"nothing", "x", "y", false, "", false, false}});
  EXPECT_EQ(s.diagram->key("nothing"), nullptr);
}

TEST(PadDiagram, ClickingAKeyReportsItsId) {
  Scene s;
  s.click(*s.diagram->key("up"));
  s.click(*s.diagram->key("a"));
  EXPECT_EQ(s.picks, (std::vector<std::string>{"up", "a"}));
}

TEST(PadDiagram, ReleasingOutsideTheKeyDoesNotPick) {
  Scene s;
  const Rect box = s.diagram->key("up")->bounds();
  s.root.mouseMove(Point{box.x + 5, box.y + 5}, 0);
  s.root.mouseDown(Point{box.x + 5, box.y + 5}, MouseButton::Left, 0);
  s.root.mouseUp(Point{box.x - 20, box.y - 20}, MouseButton::Left, 0);
  EXPECT_TRUE(s.picks.empty());
}

TEST(PadDiagram, DisabledDiagramIgnoresClicksAndKeys) {
  Scene s;
  s.diagram->setDisabled(true);
  EXPECT_TRUE(s.diagram->disabled());
  EXPECT_FALSE(s.diagram->key("up")->enabled());
  s.click(*s.diagram->key("up"));
  EXPECT_TRUE(s.picks.empty());
  s.diagram->setDisabled(false);
  EXPECT_TRUE(s.diagram->key("a")->enabled());
  s.click(*s.diagram->key("a"));
  EXPECT_EQ(s.picks, (std::vector<std::string>{"a"}));
}

TEST(PadDiagram, TabReachesEachKeyAndEnterOrSpacePicks) {
  Scene s;
  s.root.keyDown(kVkTab, 0, false);
  EXPECT_TRUE(s.diagram->key("up")->focused());
  s.root.keyDown(kVkReturn, 0, false);
  EXPECT_EQ(s.picks, (std::vector<std::string>{"up"}));
  s.root.keyDown(kVkTab, 0, false);
  EXPECT_TRUE(s.diagram->key("a")->focused());
  // Space 在抬起时才触发
  s.root.keyDown(kVkSpace, 0, false);
  EXPECT_EQ(s.picks.size(), 1U);
  s.root.keyUp(kVkSpace, 0);
  EXPECT_EQ(s.picks, (std::vector<std::string>{"up", "a"}));
}

TEST(PadDiagram, KeysCarryTheHotspotTooltip) {
  Scene s;
  ASSERT_TRUE(s.diagram->key("up")->tooltip().has_value());
  EXPECT_EQ(s.diagram->key("up")->tooltip()->text, "绑定键盘按键：上（当前：↑）");
}
