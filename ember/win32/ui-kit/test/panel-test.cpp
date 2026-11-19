/*
 * ------------------------------------------------------------------------
 * 名称：panel-test.cpp
 * 说明：面板的测试：标题行与内容区的高度、紧凑标题栏、平面外观、内容区为空时收起、aside 的位置、内容超出时在内容区滚动。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-19
 * 备注：缩放 1.0，边框 1；标题行最小高度 44（紧凑 28），内容区上下内边距各 10，内容之间间距 8。
 * ------------------------------------------------------------------------
 */

#include <gtest/gtest.h>

#include <memory>

#include "rujian/ui/panel.hpp"
#include "rujian/ui/root.hpp"

using namespace rujian::ui;

namespace {

/** 固定高度的内容。 */
class Block final : public Widget {
 public:
  explicit Block(double height, double width = 0) : height_(height), width_(width) {}
  Size measure(Size available) override { return Size{width_ > 0 ? width_ : available.width, height_}; }

 private:
  double height_;
  double width_;
};

struct Scene {
  Root root{ThemeName::Dark, 1.0};
  Panel* panel{nullptr};

  Scene() {
    auto created = std::make_unique<Panel>();
    panel = created.get();
    root.setContent(std::move(created));
  }

  void layout(double height = 0) {
    root.layout(Size{300, 400});
    const Size size = panel->measure(Size{300, 0});
    panel->arrange(Rect{0, 0, 300, height > 0 ? height : size.height});
  }
};

}  // namespace

TEST(Panel, HeightIsHeadPlusBodyPlusBorders) {
  Scene s;
  s.panel->setTitle("标题");
  s.panel->addContent(std::make_unique<Block>(30));
  s.root.layout(Size{300, 400});
  // 上下边框 2 + 标题行 44 + 内容区（内边距 10 + 30 + 10）
  EXPECT_DOUBLE_EQ(s.panel->measure(Size{300, 0}).height, 2 + 44 + 50);
}

TEST(Panel, CompactHeadUsesTheCompactHeight) {
  Scene s;
  s.panel->setTitle("标题");
  s.panel->setCompactHead(true);
  s.panel->addContent(std::make_unique<Block>(30));
  s.root.layout(Size{300, 400});
  EXPECT_DOUBLE_EQ(s.panel->measure(Size{300, 0}).height, 2 + 28 + 50);
}

TEST(Panel, ContentsAreSeparatedByTheBlockGap) {
  Scene s;
  s.panel->setTitle("标题");
  s.panel->addContent(std::make_unique<Block>(30));
  s.panel->addContent(std::make_unique<Block>(20));
  s.root.layout(Size{300, 400});
  EXPECT_DOUBLE_EQ(s.panel->measure(Size{300, 0}).height, 2 + 44 + 10 + 30 + 8 + 20 + 10);
}

TEST(Panel, PlainHasNoBorder) {
  Scene s;
  s.panel->setTitle("标题");
  s.panel->setPlain(true);
  s.panel->addContent(std::make_unique<Block>(30));
  s.root.layout(Size{300, 400});
  EXPECT_DOUBLE_EQ(s.panel->measure(Size{300, 0}).height, 44 + 50);
}

TEST(Panel, EmptyBodyCollapses) {
  Scene s;
  s.panel->setTitle("标题");
  s.root.layout(Size{300, 400});
  EXPECT_DOUBLE_EQ(s.panel->measure(Size{300, 0}).height, 2 + 44);
}

TEST(Panel, WithoutTitleOrAsideThereIsNoHead) {
  Scene s;
  s.panel->addContent(std::make_unique<Block>(30));
  s.root.layout(Size{300, 400});
  EXPECT_DOUBLE_EQ(s.panel->measure(Size{300, 0}).height, 2 + 50);
}

TEST(Panel, AsideIsOnTheRightOfTheTitleAndOnTheLeftWithoutOne) {
  Scene withTitle;
  withTitle.panel->setTitle("标题");
  withTitle.panel->setAside(std::make_unique<Block>(22, 60));
  withTitle.layout();
  const Widget* right = withTitle.panel->children()[2].get();
  // 面板宽 300，边框 1，aside 宽 60，右内边距 10
  EXPECT_DOUBLE_EQ(right->bounds().x, 300 - 1 - 10 - 60);
  EXPECT_DOUBLE_EQ(right->bounds().y, 1 + (44 - 22) / 2.0);

  Scene noTitle;
  noTitle.panel->setAside(std::make_unique<Block>(22, 60));
  noTitle.layout();
  EXPECT_DOUBLE_EQ(noTitle.panel->children()[2]->bounds().x, 1 + 10);
}

TEST(Panel, BodyScrollsWhenThePanelIsShorterThanItsContent) {
  Scene s;
  s.panel->setTitle("标题");
  s.panel->addContent(std::make_unique<Block>(200));
  s.layout(120);
  // 内容区高度 = 120 - 2 - 44
  const Widget* scroll = s.panel->children()[1].get();
  EXPECT_DOUBLE_EQ(scroll->bounds().height, 120 - 2 - 44);
  EXPECT_DOUBLE_EQ(scroll->bounds().y, 1 + 44);
}
