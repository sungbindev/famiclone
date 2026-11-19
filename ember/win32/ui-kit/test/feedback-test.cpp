/*
 * ------------------------------------------------------------------------
 * 名称：feedback-test.cpp
 * 说明：提示块、说明块、状态标签的测试：内边距与边框、内容间距、出错时让出图标槽、状态标签不换行且宽度含图标槽。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-19
 * 备注：缩放 1.0，边框 1；提示块内边距上下 6、左右 10，内容间距 4。
 * ------------------------------------------------------------------------
 */

#include <gtest/gtest.h>

#include <memory>

#include "rujian/ui/callout.hpp"
#include "rujian/ui/root.hpp"
#include "rujian/ui/state.hpp"
#include "rujian/ui/status-label.hpp"

using namespace rujian::ui;

namespace {

/** 固定高度的内容。 */
class Block final : public Widget {
 public:
  explicit Block(double height) : height_(height) {}
  Size measure(Size available) override { return Size{available.width, height_}; }

 private:
  double height_;
};

template <typename T>
struct Scene {
  Root root{ThemeName::Dark, 1.0};
  T* widget{nullptr};

  Scene() {
    auto created = std::make_unique<T>();
    widget = created.get();
    root.setContent(std::move(created));
  }

  void layout(double width) {
    root.layout(Size{width, 400});
    widget->arrange(Rect{0, 0, width, widget->measure(Size{width, 0}).height});
  }
};

}  // namespace

TEST(Callout, HeightIsTextPlusContentsPlusPaddingAndBorders) {
  Scene<Callout> s;
  s.widget->setText("说明文字");
  s.layout(300);
  const double text = s.widget->children()[0]->bounds().height;
  EXPECT_GT(text, 0);
  EXPECT_DOUBLE_EQ(s.widget->measure(Size{300, 0}).height, 2 + 12 + text);
  s.widget->addContent(std::make_unique<Block>(20));
  s.layout(300);
  EXPECT_DOUBLE_EQ(s.widget->measure(Size{300, 0}).height, 2 + 12 + text + 4 + 20);
}

TEST(Callout, ContentsStartInsideTheBorderAndPadding) {
  Scene<Callout> s;
  s.widget->setText("说明文字");
  s.widget->addContent(std::make_unique<Block>(20));
  s.layout(300);
  EXPECT_DOUBLE_EQ(s.widget->children()[0]->bounds().x, 1 + 10);
  EXPECT_DOUBLE_EQ(s.widget->children()[0]->bounds().y, 1 + 6);
  EXPECT_DOUBLE_EQ(s.widget->children()[0]->bounds().width, 300 - 2 - 20);
  EXPECT_DOUBLE_EQ(s.widget->children()[1]->bounds().y, 1 + 6 + s.widget->children()[0]->bounds().height + 4);
}

TEST(State, ErrorMakesRoomForTheIconSlot) {
  Scene<State> s;
  s.widget->setText("出错了");
  s.layout(300);
  EXPECT_DOUBLE_EQ(s.widget->children()[0]->bounds().x, 0);
  s.widget->setError(true);
  s.layout(300);
  EXPECT_DOUBLE_EQ(s.widget->children()[0]->bounds().x, 16 + 4);
  EXPECT_DOUBLE_EQ(s.widget->children()[0]->bounds().width, 300 - 20);
}

TEST(StatusLabel, WidthIncludesTheIconSlotAndTheGap) {
  Scene<StatusLabel> s;
  s.widget->setText("已启用");
  s.widget->setTone(StatusTone::Success);
  s.root.layout(Size{300, 400});
  const Size size = s.widget->measure(Size{});
  EXPECT_GT(size.height, 0);
  EXPECT_DOUBLE_EQ(size.width, 16 + 4 + s.widget->children()[0]->measure(Size{}).width);
}

TEST(StatusLabel, DoesNotWrap) {
  Scene<StatusLabel> s;
  s.widget->setText("很长很长很长很长很长很长的状态文字");
  s.root.layout(Size{300, 400});
  const double single = s.widget->measure(Size{50, 0}).height;
  s.widget->setText("短");
  EXPECT_DOUBLE_EQ(s.widget->measure(Size{50, 0}).height, single);
}
