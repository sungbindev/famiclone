/*
 * ------------------------------------------------------------------------
 * 名称：notification-test.cpp
 * 说明：通知中心的测试：弹出与堆叠、自动收起（错误通知停留加倍）、上限与历史、未读数、面板的打开与收起（点击外部、再次点击按钮、Esc、窗口失去活动状态）、单条关闭与全部清除、较长的消息点击后打开详情、通知按钮的角标。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-19
 * 备注：窗口 600×400，状态栏高 26；定时器用 Root::setTimerHandlers 记录时长、用 fireTimer 手动触发。
 * ------------------------------------------------------------------------
 */

#include <gtest/gtest.h>

#include <map>
#include <memory>
#include <string>
#include <vector>

#include "rujian/ui/notification.hpp"
#include "rujian/ui/root.hpp"
#include "rujian/ui/status-bar.hpp"

using namespace rujian::ui;

namespace {

constexpr int kVkEscape = 0x1B;

struct Scene {
  Root root{ThemeName::Dark, 1.0};
  std::map<unsigned, unsigned> durations;
  StatusBar* bar{nullptr};
  std::unique_ptr<NotificationCenter> center;
  NotificationButton* button{nullptr};
  std::vector<std::string> details;

  Scene() {
    root.setTimerHandlers([this](unsigned id, unsigned milliseconds) { durations[id] = milliseconds; }, [this](unsigned id) { durations.erase(id); });
    auto statusBar = std::make_unique<StatusBar>();
    bar = statusBar.get();
    root.setContent(std::move(statusBar));
    root.layout(Size{600, 400});
    center = std::make_unique<NotificationCenter>(root);
    center->setTimeSource([] { return std::string("09:05"); });
    center->setOnShowDetail([this](const std::string& text) { details.push_back(text); });
    auto notificationButton = std::make_unique<NotificationButton>(*center);
    button = static_cast<NotificationButton*>(bar->addEndWidget(std::move(notificationButton)));
    root.layout(Size{600, 400});
  }

  // 通知中心要先于 Root 销毁
  ~Scene() { center.reset(); }

  static Point centerOf(const Rect& rect) { return Point{rect.x + rect.width / 2, rect.y + rect.height / 2}; }

  void click(Point point) {
    root.mouseMove(point, 0);
    root.mouseDown(point, MouseButton::Left, 0);
    root.mouseUp(point, MouseButton::Left, 0);
  }
};

}  // namespace

TEST(Notification, NotifyShowsAToastRecordsItAndCountsItAsUnread) {
  Scene s;
  s.center->notify("保存完成", NoticeLevel::Success);
  ASSERT_EQ(s.center->toasts().size(), 1u);
  ASSERT_EQ(s.center->records().size(), 1u);
  EXPECT_EQ(s.center->records()[0].text, "保存完成");
  EXPECT_EQ(s.center->records()[0].time, "09:05");
  EXPECT_EQ(s.center->unread(), 1);
}

TEST(Notification, EmptyTextIsNotSent) {
  Scene s;
  s.center->notify("");
  EXPECT_TRUE(s.center->records().empty());
}

TEST(Notification, ToastIsAnchoredToTheBottomRightWithTheTokenWidth) {
  Scene s;
  s.center->notify("一条通知");
  const Rect toast = s.center->toastRect(0);
  EXPECT_DOUBLE_EQ(toast.width, 340.0);
  EXPECT_DOUBLE_EQ(toast.right(), 600.0 - 10.0);
  EXPECT_DOUBLE_EQ(toast.bottom(), 400.0 - 10.0);
}

TEST(Notification, ToastsStackUpwardsWithAGapAndTheNewestIsAtTheBottom) {
  Scene s;
  s.center->notify("第一条");
  s.center->notify("第二条");
  const Rect first = s.center->toastRect(0);
  const Rect second = s.center->toastRect(1);
  EXPECT_DOUBLE_EQ(second.bottom(), 390.0);
  EXPECT_DOUBLE_EQ(first.bottom(), second.y - 6.0);
}

TEST(Notification, BottomOffsetLiftsTheToasts) {
  Scene s;
  s.center->setBottomOffset(36.0);
  s.center->notify("一条通知");
  EXPECT_DOUBLE_EQ(s.center->toastRect(0).bottom(), 400.0 - 36.0);
}

TEST(Notification, WindowResizeRepositionsTheToasts) {
  Scene s;
  s.center->notify("一条通知");
  s.root.layout(Size{800, 500});
  EXPECT_DOUBLE_EQ(s.center->toastRect(0).right(), 790.0);
  EXPECT_DOUBLE_EQ(s.center->toastRect(0).bottom(), 490.0);
}

TEST(Notification, AtMostThreeToastsAndTheOldestGoesFirst) {
  Scene s;
  for (const char* text : {"一", "二", "三", "四"}) s.center->notify(text);
  ASSERT_EQ(s.center->toasts().size(), 3u);
  EXPECT_EQ(s.center->toasts()[0].text, "二");
  EXPECT_EQ(s.center->records().size(), 4u);
  // 被挤掉的通知的定时器也取消了：只剩 3 个
  EXPECT_EQ(s.root.pendingTimers().size(), 3u);
}

TEST(Notification, ToastsDisappearAfterTheDurationAndErrorsStayTwiceAsLong) {
  Scene s;
  s.center->notify("普通");
  s.center->notify("错误", NoticeLevel::Error);
  std::vector<unsigned> durations;
  for (const unsigned id : s.root.pendingTimers()) durations.push_back(s.durations[id]);
  EXPECT_EQ(durations, (std::vector<unsigned>{6000, 12000}));
  s.root.fireTimer(s.root.pendingTimers().front());
  ASSERT_EQ(s.center->toasts().size(), 1u);
  EXPECT_EQ(s.center->toasts()[0].text, "错误");
  EXPECT_EQ(s.center->records().size(), 2u);
}

TEST(Notification, CustomDurationIsUsed) {
  Scene s;
  s.center->setToastDuration(1000);
  s.center->notify("一条通知");
  EXPECT_EQ(s.durations[s.root.pendingTimers().front()], 1000u);
}

TEST(Notification, HistoryKeepsTheNewestHundredFirst) {
  Scene s;
  for (int i = 1; i <= 105; ++i) s.center->notify(std::to_string(i));
  ASSERT_EQ(s.center->records().size(), 100u);
  EXPECT_EQ(s.center->records().front().text, "105");
  EXPECT_EQ(s.center->records().back().text, "6");
}

TEST(Notification, ClickingTheToastCloseButtonDismissesItButKeepsTheRecord) {
  Scene s;
  s.center->notify("一条通知");
  const Rect toast = s.center->toastRect(0);
  s.click(Point{toast.right() - 10 - 14, toast.y + 6 + 14});
  EXPECT_TRUE(s.center->toasts().empty());
  EXPECT_EQ(s.center->records().size(), 1u);
}

TEST(Notification, OpeningThePanelClearsToastsAndUnread) {
  Scene s;
  s.center->notify("一条通知");
  s.center->setOpen(true);
  EXPECT_TRUE(s.center->toasts().empty());
  EXPECT_EQ(s.center->unread(), 0);
  EXPECT_TRUE(s.root.pendingTimers().empty());
}

TEST(Notification, NoticesWhileThePanelIsOpenGoStraightToHistory) {
  Scene s;
  s.center->setOpen(true);
  s.center->notify("面板打开时的通知");
  EXPECT_TRUE(s.center->toasts().empty());
  EXPECT_EQ(s.center->unread(), 0);
  EXPECT_EQ(s.center->records().size(), 1u);
}

TEST(Notification, PanelIsAnchoredToTheBottomRightAndNoTallerThanTheLimit) {
  Scene s;
  for (int i = 0; i < 30; ++i) s.center->notify("一条通知 " + std::to_string(i));
  s.center->setOpen(true);
  const Rect panel = s.center->panelRect();
  EXPECT_DOUBLE_EQ(panel.width, 340.0);
  EXPECT_DOUBLE_EQ(panel.right(), 590.0);
  EXPECT_DOUBLE_EQ(panel.bottom(), 390.0);
  EXPECT_LE(panel.height, 480.0);
  EXPECT_LE(panel.height, 400.0 - 10.0 - 36.0 - 10.0 + 0.001);
}

TEST(Notification, EmptyPanelIsShorterThanAFullOne) {
  Scene s;
  s.center->setOpen(true);
  const double empty = s.center->panelRect().height;
  s.center->setOpen(false);
  s.center->notify("一条通知");
  s.center->setOpen(true);
  EXPECT_LT(empty, s.center->panelRect().height);
}

TEST(Notification, PanelClosesWhenClickingOutsideButNotOnTheButton) {
  Scene s;
  s.center->setOpen(true);
  s.click(Point{20, 20});
  EXPECT_FALSE(s.center->open());
  s.center->setOpen(true);
  const Point onButton = Scene::centerOf(s.button->bounds());
  s.root.mouseMove(onButton, 0);
  s.root.mouseDown(onButton, MouseButton::Left, 0);
  EXPECT_TRUE(s.center->open());
  s.root.mouseUp(onButton, MouseButton::Left, 0);
  EXPECT_FALSE(s.center->open());
}

TEST(Notification, ClickingInsideThePanelKeepsItOpen) {
  Scene s;
  s.center->notify("一条通知");
  s.center->setOpen(true);
  const Rect panel = s.center->panelRect();
  s.click(Point{panel.x + 20, panel.y + 20});
  EXPECT_TRUE(s.center->open());
}

TEST(Notification, ButtonOpensThePanelOnClick) {
  Scene s;
  s.click(Scene::centerOf(s.button->bounds()));
  EXPECT_TRUE(s.center->open());
}

TEST(Notification, EscapeAndWindowDeactivationClosePanel) {
  Scene s;
  s.center->setOpen(true);
  EXPECT_FALSE(s.center->handleEscape(0x41));
  EXPECT_TRUE(s.center->handleEscape(kVkEscape));
  EXPECT_FALSE(s.center->open());
  EXPECT_FALSE(s.center->handleEscape(kVkEscape));
  s.center->setOpen(true);
  s.center->windowDeactivated();
  EXPECT_FALSE(s.center->open());
}

TEST(Notification, RemovingAndClearingChangeTheHistory) {
  Scene s;
  s.center->notify("一");
  s.center->notify("二");
  const int newest = s.center->records().front().id;
  s.center->remove(newest);
  ASSERT_EQ(s.center->records().size(), 1u);
  EXPECT_EQ(s.center->records()[0].text, "一");
  EXPECT_EQ(s.center->toasts().size(), 1u);
  s.center->clear();
  EXPECT_TRUE(s.center->records().empty());
}

TEST(Notification, LongMessagesAreDetectedByLengthOrNewLine) {
  EXPECT_FALSE(isLongNotice(std::string(60, 'a')));
  EXPECT_TRUE(isLongNotice(std::string(61, 'a')));
  EXPECT_FALSE(isLongNotice("一二三四五六七八九十一二三四五六七八九十一二三四五六七八九十一二三四五六七八九十一二三四五六七八九十一二三四五六七八九十"));
  EXPECT_TRUE(isLongNotice("两行\n第二行"));
}

TEST(Notification, ClickingALongMessageOpensTheDetailAndAShortOneDoesNot) {
  Scene s;
  s.center->notify("短消息");
  Rect toast = s.center->toastRect(0);
  s.click(Point{toast.x + 10 + 14 + 6 + 10, toast.y + 6 + 8});
  EXPECT_TRUE(s.details.empty());
  s.center->notify("很长的消息\n第二行");
  toast = s.center->toastRect(1);
  s.click(Point{toast.x + 10 + 14 + 6 + 10, toast.y + 6 + 8});
  EXPECT_EQ(s.details, (std::vector<std::string>{"很长的消息\n第二行"}));
}

TEST(Notification, LongToastMessageIsLimitedToThreeLines) {
  Scene s;
  s.center->notify(std::string(600, 'x'));
  s.center->notify("短");
  EXPECT_LT(s.center->toastRect(0).height, 3 * 12 * 1.45 + 12 + 2 + 1.0);
  EXPECT_GT(s.center->toastRect(0).height, s.center->toastRect(1).height);
}

TEST(Notification, EmptySpaceInTheLayerLetsEventsThrough) {
  Scene s;
  s.center->notify("一条通知");
  s.root.mouseMove(Point{20, 20}, 0);
  EXPECT_FALSE(s.bar->hovered() && s.center->hovered());
}

TEST(Notification, ButtonShowsTheUnreadCountAndCapsItAt99Plus) {
  Scene s;
  const double base = s.button->measure(Size{}).width;
  EXPECT_DOUBLE_EQ(base, 24.0);
  s.center->notify("一条通知");
  const double one = s.button->measure(Size{}).width;
  EXPECT_GT(one, base);
  for (int i = 0; i < 120; ++i) s.center->notify("x");
  EXPECT_GT(s.button->measure(Size{}).width, one);
  s.center->setOpen(true);
  EXPECT_DOUBLE_EQ(s.button->measure(Size{}).width, 24.0);
}
