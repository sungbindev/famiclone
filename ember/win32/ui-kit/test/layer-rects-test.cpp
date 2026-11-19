/*
 * ------------------------------------------------------------------------
 * 名称：layer-rects-test.cpp
 * 说明：弹层盖住的区域的测试：Root 在绘制后只在区域变化时回调；通知中心只报告弹出的通知与打开的面板，悬停提示只在显示时报告（含角标）；没有重写的弹层报告它的整个区域。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-19
 * 备注：应用把压在窗口上的原生子窗口（运行画面）按这些区域挖空，弹层才能显示在它之上。
 * ------------------------------------------------------------------------
 */

#include <gtest/gtest.h>

#include <memory>
#include <vector>

#include "rujian/ui/notification.hpp"
#include "rujian/ui/offscreen.hpp"
#include "rujian/ui/root.hpp"
#include "rujian/ui/tooltip.hpp"
#include "rujian/ui/widget.hpp"

using namespace rujian::ui;

namespace {

/** 窗口 600×400 的场景：绘制后记下回调给的区域。 */
struct Scene {
  Root root{ThemeName::Dark, 1.0};
  OffscreenSurface surface{Size{600, 400}, 1.0};
  std::vector<std::vector<Rect>> reports;

  Scene() {
    root.setContent(std::make_unique<Widget>());
    root.layout(Size{600, 400});
    root.setLayerRectsHandler([this](const std::vector<Rect>& rects) { reports.push_back(rects); });
  }

  void paint() { ASSERT_TRUE(surface.render(root, Color::rgba(0, 0, 0))); }
};

/** 一个固定区域的弹层。 */
class BoxLayer final : public Widget {
 public:
  explicit BoxLayer(Rect box) : box_(box) {}
  void arrangeBox() { arrange(box_); }

 private:
  Rect box_;
};

}  // namespace

TEST(LayerRects, NoLayersGiveNoReportUntilSomethingChanges) {
  Scene s;
  s.paint();
  // 一开始就没有弹层，区域没有变化
  EXPECT_TRUE(s.reports.empty());
}

TEST(LayerRects, AnOrdinaryLayerReportsItsWholeArea) {
  Scene s;
  BoxLayer layer(Rect{10, 20, 100, 50});
  s.root.showLayer(&layer, [](Widget*) {});
  layer.arrangeBox();
  s.paint();
  ASSERT_EQ(s.reports.size(), 1u);
  ASSERT_EQ(s.reports[0].size(), 1u);
  EXPECT_EQ(s.reports[0][0], (Rect{10, 20, 100, 50}));
  // 再画一次区域没变，不重复回调
  s.paint();
  EXPECT_EQ(s.reports.size(), 1u);
  // 关闭后报告空
  s.root.hideLayer(&layer);
  s.paint();
  ASSERT_EQ(s.reports.size(), 2u);
  EXPECT_TRUE(s.reports[1].empty());
}

TEST(LayerRects, DismissLayersTellsEveryLayerNothingWasHit) {
  Scene s;
  BoxLayer first(Rect{10, 20, 100, 50});
  BoxLayer second(Rect{200, 20, 100, 50});
  std::vector<Widget*> hits;
  s.root.showLayer(&first, [&hits](Widget* hit) { hits.push_back(hit); });
  // 回调里把自己关掉：遍历不受影响
  s.root.showLayer(&second, [&](Widget* hit) {
    hits.push_back(hit);
    s.root.hideLayer(&second);
  });
  s.root.dismissLayers();
  EXPECT_EQ(hits, (std::vector<Widget*>{nullptr, nullptr}));
  // 第二个已关闭，再通知时只剩第一个
  hits.clear();
  s.root.dismissLayers();
  EXPECT_EQ(hits, (std::vector<Widget*>{nullptr}));
}

TEST(LayerRects, TheNotificationCenterOnlyReportsToastsAndAnOpenPanel) {
  Scene s;
  NotificationCenter center(s.root);
  s.root.layout(Size{600, 400});
  s.paint();
  // 铺满窗口但什么都没弹出：没有区域
  EXPECT_TRUE(s.reports.empty());
  center.notify("保存完成");
  s.root.layout(Size{600, 400});
  s.paint();
  ASSERT_EQ(s.reports.size(), 1u);
  ASSERT_EQ(s.reports[0].size(), 1u);
  EXPECT_EQ(s.reports[0][0], center.toastRect(0));
  EXPECT_LT(s.reports[0][0].width, 600);
  center.setOpen(true);
  s.root.layout(Size{600, 400});
  s.paint();
  ASSERT_FALSE(s.reports.empty());
  EXPECT_EQ(s.reports.back().back(), center.panelRect());
}

TEST(LayerRects, TheTooltipReportsItsBoxWithTheArrowOnlyWhileShown) {
  Scene s;
  Tooltip tooltip(s.root);
  auto target = std::make_unique<Widget>();
  Widget* anchor = s.root.content()->addChild(std::move(target));
  anchor->arrange(Rect{100, 100, 20, 20});
  s.paint();
  EXPECT_TRUE(s.reports.empty());
  tooltip.showTooltip(*anchor, TooltipContent{.text = "提示"}, Point{110, 110});
  s.paint();
  ASSERT_EQ(s.reports.size(), 1u);
  ASSERT_EQ(s.reports[0].size(), 1u);
  EXPECT_EQ(s.reports[0][0].x, tooltip.bounds().x - 10);
  EXPECT_EQ(s.reports[0][0].width, tooltip.bounds().width + 20);
  tooltip.hideTooltip();
  s.paint();
  ASSERT_EQ(s.reports.size(), 2u);
  EXPECT_TRUE(s.reports[1].empty());
}
