/*
 * ------------------------------------------------------------------------
 * 名称：select-test.cpp
 * 说明：下拉框的测试：鼠标与键盘展开、选择、关闭，点击弹层外关闭，弹层不夺走键盘焦点，窗口大小变化时关闭，空间不足时翻到上方，占位项与禁用。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-19
 * 备注：弹层里选项的位置用 Select::optionRect 取得。
 * ------------------------------------------------------------------------
 */

#include <gtest/gtest.h>

#include <memory>
#include <string>
#include <vector>

#include "rujian/ui/root.hpp"
#include "rujian/ui/select.hpp"

using namespace rujian::ui;

namespace {

constexpr int kVkTab = 0x09;
constexpr int kVkReturn = 0x0D;
constexpr int kVkEscape = 0x1B;
constexpr int kVkEnd = 0x23;
constexpr int kVkHome = 0x24;
constexpr int kVkUp = 0x26;
constexpr int kVkDown = 0x28;

/** 把下拉框放在 (10, top)、宽 120 的页面；另有一个可获得焦点的下拉框在旁边，用来检验 Tab。 */
class Holder final : public Widget {
 public:
  explicit Holder(double top) : top_(top) {}
  void arrange(const Rect& bounds) override {
    Widget::arrange(bounds);
    double x = 10;
    for (const auto& child : children()) {
      const Size size = child->measure(Size{120, 0});
      child->arrange(Rect{x, top_, size.width, size.height});
      x += 140;
    }
  }

 private:
  double top_;
};

struct Scene {
  Root root{ThemeName::Dark, 1.0};
  Select* select{nullptr};
  Select* other{nullptr};
  std::vector<std::string> changes;

  explicit Scene(double top = 10, Size size = Size{300, 240}) {
    auto holder = std::make_unique<Holder>(top);
    auto first = std::make_unique<Select>();
    select = first.get();
    holder->addChild(std::move(first));
    auto second = std::make_unique<Select>();
    other = second.get();
    holder->addChild(std::move(second));
    root.setContent(std::move(holder));
    for (Select* item : {select, other}) {
      item->setOptions({{"a", "选项一"}, {"b", "选项二"}, {"c", "选项三"}});
      item->setAllowEmpty(false);
    }
    select->setOnChange([this](const std::string& value) { changes.push_back(value); });
    root.layout(size);
  }

  /** 触发器中心点。 */
  [[nodiscard]] Point triggerCenter() const {
    const Rect box = select->bounds();
    return Point{box.x + box.width / 2, box.y + box.height / 2};
  }

  void click(Point point) {
    root.mouseMove(point, 0);
    root.mouseDown(point, MouseButton::Left, 0);
    root.mouseUp(point, MouseButton::Left, 0);
  }

  [[nodiscard]] Point optionCenter(std::size_t index) const {
    const auto row = select->optionRect(index);
    EXPECT_TRUE(row.has_value());
    return row ? Point{row->x + row->width / 2, row->y + row->height / 2} : Point{};
  }
};

}  // namespace

TEST(Select, ClickOpensAndChoosingAnOptionSelectsItAndCloses) {
  Scene s;
  s.click(s.triggerCenter());
  ASSERT_TRUE(s.select->open());
  s.click(s.optionCenter(1));
  EXPECT_FALSE(s.select->open());
  EXPECT_EQ(s.select->value(), "b");
  EXPECT_EQ(s.changes, (std::vector<std::string>{"b"}));
  EXPECT_TRUE(s.select->focused());
}

TEST(Select, ChoosingTheCurrentOptionClosesWithoutReporting) {
  Scene s;
  s.select->setValue("b");
  s.click(s.triggerCenter());
  s.click(s.optionCenter(1));
  EXPECT_FALSE(s.select->open());
  EXPECT_TRUE(s.changes.empty());
}

TEST(Select, ClickingTheTriggerAgainClosesThePopup) {
  Scene s;
  s.click(s.triggerCenter());
  ASSERT_TRUE(s.select->open());
  s.click(s.triggerCenter());
  EXPECT_FALSE(s.select->open());
}

TEST(Select, ClickingOutsideClosesThePopup) {
  Scene s;
  s.click(s.triggerCenter());
  ASSERT_TRUE(s.select->open());
  s.click(Point{280, 220});
  EXPECT_FALSE(s.select->open());
  EXPECT_TRUE(s.changes.empty());
}

TEST(Select, PressingOnThePopupKeepsFocusOnTheTrigger) {
  Scene s;
  s.click(s.triggerCenter());
  const Point row = s.optionCenter(0);
  s.root.mouseMove(row, 0);
  s.root.mouseDown(row, MouseButton::Left, 0);
  EXPECT_TRUE(s.select->open());
  EXPECT_TRUE(s.select->focused());
  s.root.mouseUp(row, MouseButton::Left, 0);
}

TEST(Select, ReleasingOnAnotherOptionDoesNotChoose) {
  Scene s;
  s.click(s.triggerCenter());
  s.root.mouseMove(s.optionCenter(0), 0);
  s.root.mouseDown(s.optionCenter(0), MouseButton::Left, 0);
  s.root.mouseMove(s.optionCenter(2), 0);
  s.root.mouseUp(s.optionCenter(2), MouseButton::Left, 0);
  EXPECT_TRUE(s.select->open());
  EXPECT_TRUE(s.changes.empty());
}

TEST(Select, KeyboardOpensNavigatesAndChooses) {
  Scene s;
  s.root.keyDown(kVkTab, 0, false);
  ASSERT_TRUE(s.select->focused());
  s.root.keyDown(kVkDown, 0, false);
  ASSERT_TRUE(s.select->open());
  s.root.keyDown(kVkDown, 0, false);
  s.root.keyDown(kVkDown, 0, false);
  s.root.keyDown(kVkReturn, 0, false);
  EXPECT_FALSE(s.select->open());
  EXPECT_EQ(s.select->value(), "c");
  EXPECT_EQ(s.changes, (std::vector<std::string>{"c"}));
}

TEST(Select, KeyboardNavigationWrapsAndHomeEndJump) {
  Scene s;
  s.root.keyDown(kVkTab, 0, false);
  s.root.keyDown(kVkDown, 0, false);
  s.root.keyDown(kVkUp, 0, false);
  s.root.keyDown(kVkReturn, 0, false);
  EXPECT_EQ(s.select->value(), "c");
  s.root.keyDown(kVkDown, 0, false);
  s.root.keyDown(kVkHome, 0, false);
  s.root.keyDown(kVkReturn, 0, false);
  EXPECT_EQ(s.select->value(), "a");
  s.root.keyDown(kVkDown, 0, false);
  s.root.keyDown(kVkEnd, 0, false);
  s.root.keyDown(kVkReturn, 0, false);
  EXPECT_EQ(s.select->value(), "c");
}

TEST(Select, EscapeClosesWithoutChanging) {
  Scene s;
  s.root.keyDown(kVkTab, 0, false);
  s.root.keyDown(kVkDown, 0, false);
  s.root.keyDown(kVkDown, 0, false);
  s.root.keyDown(kVkEscape, 0, false);
  EXPECT_FALSE(s.select->open());
  EXPECT_TRUE(s.select->focused());
  EXPECT_TRUE(s.changes.empty());
}

TEST(Select, TabClosesThePopupAndMovesFocus) {
  Scene s;
  s.root.keyDown(kVkTab, 0, false);
  s.root.keyDown(kVkDown, 0, false);
  ASSERT_TRUE(s.select->open());
  s.root.keyDown(kVkTab, 0, false);
  EXPECT_FALSE(s.select->open());
  EXPECT_TRUE(s.other->focused());
}

TEST(Select, MovingFocusAwayClosesThePopup) {
  Scene s;
  s.click(s.triggerCenter());
  ASSERT_TRUE(s.select->open());
  const Rect other = s.other->bounds();
  s.click(Point{other.x + other.width / 2, other.y + other.height / 2});
  EXPECT_FALSE(s.select->open());
  EXPECT_TRUE(s.other->focused());
}

TEST(Select, ResizingTheWindowClosesThePopup) {
  Scene s;
  s.click(s.triggerCenter());
  ASSERT_TRUE(s.select->open());
  s.root.layout(Size{320, 240});
  EXPECT_FALSE(s.select->open());
}

TEST(Select, PopupOpensBelowWhenThereIsRoomAndAboveWhenThereIsNot) {
  Scene below(10);
  below.click(below.triggerCenter());
  ASSERT_TRUE(below.select->open());
  EXPECT_GT(below.select->optionRect(0)->y, below.select->bounds().bottom());

  Scene above(200, Size{300, 240});
  above.click(above.triggerCenter());
  ASSERT_TRUE(above.select->open());
  EXPECT_LT(above.select->optionRect(2)->bottom(), above.select->bounds().y);
}

TEST(Select, PopupIsAtLeastAsWideAsTheTrigger) {
  Scene s;
  s.click(s.triggerCenter());
  const Rect box = s.select->bounds();
  const Rect row = *s.select->optionRect(0);
  EXPECT_GE(row.width + 2 * 4 + 2, box.width - 0.01);
}

TEST(Select, PlaceholderEntryClearsTheValueWhenEmptyIsAllowed) {
  Scene s;
  s.select->setAllowEmpty(true);
  s.select->setValue("b");
  s.click(s.triggerCenter());
  // 占位项排在第一位
  s.click(s.optionCenter(0));
  EXPECT_EQ(s.select->value(), "");
  EXPECT_EQ(s.changes, (std::vector<std::string>{""}));
}

TEST(Select, SetValueDoesNotReportAndSelectedOptionIsHighlightedOnOpen) {
  Scene s;
  s.select->setValue("c");
  EXPECT_TRUE(s.changes.empty());
  s.root.keyDown(kVkTab, 0, false);
  s.root.keyDown(kVkDown, 0, false);
  s.root.keyDown(kVkReturn, 0, false);
  // 键盘展开时高亮落在已选项上，直接回车不改变值
  EXPECT_EQ(s.select->value(), "c");
  EXPECT_TRUE(s.changes.empty());
}

TEST(Select, DisabledSelectDoesNotOpenAndIsSkippedByTab) {
  Scene s;
  s.select->setEnabled(false);
  s.click(s.triggerCenter());
  EXPECT_FALSE(s.select->open());
  s.root.keyDown(kVkTab, 0, false);
  EXPECT_FALSE(s.select->focused());
}

TEST(Select, DisablingWhileOpenClosesThePopup) {
  Scene s;
  s.click(s.triggerCenter());
  ASSERT_TRUE(s.select->open());
  s.select->setEnabled(false);
  EXPECT_FALSE(s.select->open());
}

TEST(Select, MeasureNeverGoesBelowTheMinimumWidthAndTakesTheGivenWidth) {
  Scene s;
  const Size natural = s.select->measure(Size{});
  EXPECT_GE(natural.width, 88.0);
  EXPECT_DOUBLE_EQ(s.select->measure(Size{200, 0}).width, 200.0);
  EXPECT_DOUBLE_EQ(natural.height, 24.0);
}

TEST(Select, DestroyingTheRootWhileOpenIsSafe) {
  auto scene = std::make_unique<Scene>();
  scene->click(scene->triggerCenter());
  ASSERT_TRUE(scene->select->open());
  scene.reset();
  SUCCEED();
}
