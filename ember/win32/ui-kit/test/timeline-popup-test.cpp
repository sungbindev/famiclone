/*
 * ------------------------------------------------------------------------
 * 名称：timeline-popup-test.cpp
 * 说明：时间轴弹层的测试：在锚点上方打开并获得焦点、松开鼠标时才按位置回调定位、拖到轨道之外时限制在两端、没有总时长时不能定位、Esc 与点击弹层之外关闭而点击打开它的控件不关闭、窗口大小变化时关闭、时间的显示格式、包络与说明文字都能画出。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-21
 * 备注：窗口 800×300，锚点（打开弹层的控件）在 (300, 200)，宽 100、高 24；总时长设为 100 秒，轨道上每 1 像素大约对应 100 / 轨道宽度 秒。
 * ------------------------------------------------------------------------
 */

#include <gtest/gtest.h>

#include <cstdint>
#include <memory>
#include <vector>

#include "rujian/ui/offscreen.hpp"
#include "rujian/ui/root.hpp"
#include "rujian/ui/timeline-popup.hpp"

using namespace rujian::ui;

namespace {

constexpr int kVkEscape = 0x1B;

/** 打开弹层的控件：放在固定位置。 */
class Anchor final : public Widget {};

class Holder final : public Widget {
 public:
  explicit Holder(double anchorTop) : anchorTop_(anchorTop) {}
  void arrange(const Rect& bounds) override {
    Widget::arrange(bounds);
    for (const auto& child : children()) child->arrange(Rect{300, anchorTop_, 100, 24});
  }

 private:
  double anchorTop_;
};

struct Scene {
  Root root{ThemeName::Dark, 1.0};
  Anchor* anchor{nullptr};
  TimelinePopup popup;
  std::vector<double> seeks;
  int closed{0};

  explicit Scene(Size size = Size{800, 300}, double anchorTop = 200) {
    auto holder = std::make_unique<Holder>(anchorTop);
    auto created = std::make_unique<Anchor>();
    anchor = created.get();
    holder->addChild(std::move(created));
    root.setContent(std::move(holder));
    root.layout(size);
    popup.setDuration(100);
    popup.setOnSeek([this](double seconds) { seeks.push_back(seconds); });
    popup.setOnClosed([this] { ++closed; });
  }

  void open() { popup.open(*anchor, anchor->bounds()); }

  /** 轨道上第 fraction（0 到 1）处的横坐标，纵向取轨道中间。 */
  [[nodiscard]] Point plotPoint(double fraction) const {
    const Rect plot = popup.plotRect();
    return Point{plot.x + plot.width * fraction, plot.y + plot.height / 2};
  }

  void press(Point point) {
    root.mouseMove(point, 0);
    root.mouseDown(point, MouseButton::Left, 0);
  }

  void release(Point point) {
    root.mouseMove(point, 0);
    root.mouseUp(point, MouseButton::Left, 0);
  }

  void click(Point point) {
    press(point);
    release(point);
  }
};

}  // namespace

TEST(TimelinePopup, OpensAboveTheAnchorAndTakesTheKeyboardFocus) {
  Scene s;
  EXPECT_FALSE(s.popup.isOpen());
  s.open();
  ASSERT_TRUE(s.popup.isOpen());
  EXPECT_LE(s.popup.bounds().bottom(), s.anchor->bounds().y) << "锚点下面放不下时（这里上面更宽裕）显示在锚点上方";
  EXPECT_GE(s.popup.bounds().x, 8.0);
  EXPECT_LE(s.popup.bounds().right(), 800.0 - 8.0);
  EXPECT_TRUE(s.popup.focused());
  EXPECT_GT(s.popup.plotRect().width, 500.0);
}

TEST(TimelinePopup, FlipsBelowTheAnchorWhenThereIsNoRoomAbove) {
  Scene s(Size{800, 300}, 10);
  s.open();
  ASSERT_TRUE(s.popup.isOpen());
  EXPECT_GE(s.popup.bounds().y, s.anchor->bounds().bottom()) << "上面放不下时显示在锚点下方";
}

TEST(TimelinePopup, SeeksToTheTimeUnderTheMouseOnlyWhenTheButtonIsReleased) {
  Scene s;
  s.open();
  s.press(s.plotPoint(0.25));
  EXPECT_TRUE(s.seeks.empty()) << "按下时不定位";
  s.root.mouseMove(s.plotPoint(0.5), 0);
  EXPECT_TRUE(s.seeks.empty()) << "拖动中不定位";
  s.release(s.plotPoint(0.5));
  ASSERT_EQ(s.seeks.size(), 1U);
  EXPECT_NEAR(s.seeks[0], 50.0, 0.2) << "按松开的位置定位，不是按下的位置";
  EXPECT_TRUE(s.popup.isOpen()) << "定位后弹层保持打开，可以继续找";
}

TEST(TimelinePopup, AClickSeeksToThePointClicked) {
  Scene s;
  s.open();
  s.click(s.plotPoint(0.8));
  ASSERT_EQ(s.seeks.size(), 1U);
  EXPECT_NEAR(s.seeks[0], 80.0, 0.2);
}

TEST(TimelinePopup, DraggingBeyondThePlotClampsToTheEnds) {
  Scene s;
  s.open();
  s.press(s.plotPoint(0.5));
  s.release(Point{790, 100});
  ASSERT_EQ(s.seeks.size(), 1U);
  EXPECT_NEAR(s.seeks[0], 100.0, 1e-9) << "拖到右边之外按总时长";
  s.press(s.plotPoint(0.5));
  s.release(Point{2, 100});
  ASSERT_EQ(s.seeks.size(), 2U);
  EXPECT_NEAR(s.seeks[1], 0.0, 1e-9) << "拖到左边之外按 0";
}

TEST(TimelinePopup, PressingOnThePopupOutsideThePlotDoesNothing) {
  Scene s;
  s.open();
  const Point padding{s.popup.bounds().x + 3, s.popup.bounds().y + 3};
  s.click(padding);
  EXPECT_TRUE(s.seeks.empty());
  EXPECT_TRUE(s.popup.isOpen());
}

TEST(TimelinePopup, WithoutADurationNothingCanBeSought) {
  Scene s;
  s.popup.setDuration(0);
  s.open();
  s.click(s.plotPoint(0.5));
  EXPECT_TRUE(s.seeks.empty());
  EXPECT_TRUE(s.popup.isOpen());
}

TEST(TimelinePopup, EscapeClosesAndTellsTheCaller) {
  Scene s;
  s.open();
  s.root.keyDown(kVkEscape, 0, false);
  EXPECT_FALSE(s.popup.isOpen());
  EXPECT_EQ(s.closed, 1);
}

TEST(TimelinePopup, PressingOutsideClosesButPressingTheOwnerDoesNot) {
  Scene s;
  s.open();
  const Rect owner = s.anchor->bounds();
  s.click(Point{owner.x + 5, owner.y + 5});
  EXPECT_TRUE(s.popup.isOpen()) << "在打开它的控件上按下不关闭，由那个控件自己决定";
  EXPECT_EQ(s.closed, 0);
  s.click(Point{10, 10});
  EXPECT_FALSE(s.popup.isOpen());
  EXPECT_EQ(s.closed, 1);
  EXPECT_TRUE(s.seeks.empty());
}

TEST(TimelinePopup, ClosingFromTheCallerDoesNotNotifyAndAWindowResizeCloses) {
  Scene s;
  s.open();
  s.popup.close();
  EXPECT_FALSE(s.popup.isOpen());
  EXPECT_EQ(s.closed, 0) << "调用方自己关闭时不回调";
  s.open();
  ASSERT_TRUE(s.popup.isOpen());
  s.root.layout(Size{640, 300});
  EXPECT_FALSE(s.popup.isOpen());
  EXPECT_EQ(s.closed, 1) << "窗口大小变化时关闭并回调";
}

TEST(TimelinePopup, FormatsTimeAsMinutesAndSecondsOrWithHours) {
  EXPECT_EQ(TimelinePopup::formatTime(0), "0:00");
  EXPECT_EQ(TimelinePopup::formatTime(59.9), "0:59");
  EXPECT_EQ(TimelinePopup::formatTime(61), "1:01");
  EXPECT_EQ(TimelinePopup::formatTime(3599), "59:59");
  EXPECT_EQ(TimelinePopup::formatTime(3600), "1:00:00");
  EXPECT_EQ(TimelinePopup::formatTime(3725.4), "1:02:05");
  EXPECT_EQ(TimelinePopup::formatTime(-5), "0:00");
}

TEST(TimelinePopup, DrawsTheEnvelopeTheBlanksAndTheNotice) {
  Scene s;
  s.open();
  std::vector<std::uint8_t> peaks(1000);
  for (std::size_t index = 0; index < peaks.size(); ++index) peaks[index] = static_cast<std::uint8_t>(index % 100);
  s.popup.setEnvelope(peaks, 0.1, {TimelineRange{10, 20}, TimelineRange{50, 60}});
  s.popup.setPosition(30);
  OffscreenSurface surface(Size{800, 300}, 1.0);
  ASSERT_TRUE(surface.render(s.root, Color::rgba(0, 0, 0)));
  const auto withEnvelope = surface.pixels();
  s.popup.setNotice("分析中 30%");
  ASSERT_TRUE(surface.render(s.root, Color::rgba(0, 0, 0)));
  EXPECT_NE(withEnvelope, surface.pixels()) << "有说明文字时不画包络，改写文字";
  s.popup.setNotice("");
  s.popup.clearEnvelope();
  ASSERT_TRUE(surface.render(s.root, Color::rgba(0, 0, 0)));
  EXPECT_NE(withEnvelope, surface.pixels()) << "清除包络后轨道是空的";
}
