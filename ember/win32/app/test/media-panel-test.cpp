/*
 * ------------------------------------------------------------------------
 * 名称：media-panel-test.cpp
 * 说明：设备面板内容的测试：每个设备一行（软驱 A、磁带机、打印机）、文字与按钮可用状态取自模型、点击按钮调用模型的对应操作、写保护是按下态的按钮、展开后立即读取并每 500 毫秒读取一次、销毁后取消订阅。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-19
 * 备注：用控件库的离屏根容器测试，模型是假的；点击用根容器的鼠标事件，缩放 1.0。
 * ------------------------------------------------------------------------
 */

#include <gtest/gtest.h>

#include <map>
#include <memory>

#include "contracts/media/media-model.hpp"
#include "rujian/ui/root.hpp"
#include "ui/media/media-panel-content.hpp"

namespace ember::win32 {
namespace {

/** 假模型：记下收到的操作，内容由测试直接改。 */
class FakeMediaModel final : public contracts::MediaModel {
 public:
  contracts::MediaView current;
  std::vector<std::string> calls;
  std::map<int, std::function<void()>> handlers;
  int refreshes{0};

  FakeMediaModel() {
    contracts::MediaFloppyRow row;
    row.drive = 0;
    row.label = "软驱 A";
    row.disk = "空";
    row.state = "无盘片";
    row.canInsert = true;
    current.floppies.push_back(row);
    current.tape = {"空", "—", "无磁带", false, true, false, false};
    current.printer = "空闲";
  }

  [[nodiscard]] contracts::MediaView view() const override { return current; }
  void insertFloppy(int drive) override { calls.push_back("insert " + std::to_string(drive)); }
  void ejectFloppy(int drive) override { calls.push_back("eject " + std::to_string(drive)); }
  void setWriteProtected(int drive, bool on) override { calls.push_back("protect " + std::to_string(drive) + (on ? " on" : " off")); }
  void ejectTape() override { calls.push_back("tapeEject"); }
  void playTape() override { calls.push_back("tapePlay"); }
  void recordTape() override { calls.push_back("tapeRecord"); }
  void stopTape() override { calls.push_back("tapeStop"); }
  void refresh() override { ++refreshes; }
  [[nodiscard]] std::function<void()> subscribe(std::function<void()> handler) override {
    const int id = static_cast<int>(handlers.size()) + 1;
    handlers[id] = std::move(handler);
    return [this, id] { handlers.erase(id); };
  }
  void notify() {
    const auto copy = handlers;
    for (const auto& [id, handler] : copy) handler();
  }
};

struct Scene {
  // 模型要比根容器活得久：控件销毁时还要取消订阅
  FakeMediaModel model;
  rujian::ui::Root root{rujian::ui::ThemeName::Dark, 1.0};
  ui::MediaPanelContent* content{nullptr};

  Scene() {
    auto created = std::make_unique<ui::MediaPanelContent>(model);
    content = created.get();
    root.setContent(std::move(created));
    root.layout({524, 140});
  }

  void click(const rujian::ui::Button& button) {
    const auto& box = button.bounds();
    const rujian::ui::Point at{box.x + box.width / 2, box.y + box.height / 2};
    root.mouseDown(at, rujian::ui::MouseButton::Left, 0, 0);
    root.mouseUp(at, rujian::ui::MouseButton::Left, 0);
  }
};

TEST(MediaPanelContent, ListsFloppyATapeAndPrinterAndReadsStatusAtOnce) {
  Scene s;
  ASSERT_EQ(s.content->floppies().size(), 1U);
  EXPECT_NE(s.content->floppies()[0].item, nullptr);
  EXPECT_NE(s.content->tape().item, nullptr);
  EXPECT_GE(s.model.refreshes, 1);
  // 三行从上到下排列，行高相同
  const double first = s.content->floppies()[0].item->bounds().y;
  const double second = s.content->tape().item->bounds().y;
  const double third = s.content->printerRow().bounds().y;
  EXPECT_LT(first, second);
  EXPECT_LT(second, third);
  EXPECT_DOUBLE_EQ(second - first, third - second);
}

TEST(MediaPanelContent, ButtonsFollowTheModel) {
  Scene s;
  const auto& floppy = s.content->floppies()[0];
  EXPECT_TRUE(floppy.insert->enabled());
  EXPECT_FALSE(floppy.eject->enabled());
  EXPECT_FALSE(floppy.writeProtect->enabled());
  EXPECT_TRUE(s.content->tape().play->enabled());
  EXPECT_FALSE(s.content->tape().eject->enabled() || s.content->tape().record->enabled() || s.content->tape().stop->enabled());
  s.model.current.floppies[0].disk = "a.img";
  s.model.current.floppies[0].state = "空闲";
  s.model.current.floppies[0].canEject = true;
  s.model.current.floppies[0].canWriteProtect = true;
  s.model.current.floppies[0].writeProtected = true;
  s.model.current.tape = {"t.wav", "0:03 / 0:10", "播放中", true, false, false, true};
  s.model.notify();
  EXPECT_TRUE(floppy.eject->enabled());
  EXPECT_TRUE(floppy.writeProtect->enabled());
  EXPECT_TRUE(s.content->tape().stop->enabled());
  EXPECT_FALSE(s.content->tape().play->enabled());
}

TEST(MediaPanelContent, ClickingButtonsCallsTheModel) {
  Scene s;
  s.model.current.floppies[0].canEject = true;
  s.model.current.floppies[0].canWriteProtect = true;
  s.model.current.tape = {"t.wav", "0:00", "已停止", true, true, true, false};
  s.model.notify();
  const auto& floppy = s.content->floppies()[0];
  s.click(*floppy.insert);
  s.click(*floppy.eject);
  s.click(*floppy.writeProtect);
  s.click(*s.content->tape().eject);
  s.click(*s.content->tape().play);
  s.click(*s.content->tape().record);
  EXPECT_EQ(s.model.calls, (std::vector<std::string>{"insert 0", "eject 0", "protect 0 on", "tapeEject", "tapePlay", "tapeRecord"}));
  // 已经写保护时再点就是取消
  s.model.current.floppies[0].writeProtected = true;
  s.model.notify();
  s.click(*floppy.writeProtect);
  EXPECT_EQ(s.model.calls.back(), "protect 0 off");
}

TEST(MediaPanelContent, ReadsEvery500MillisecondsWhileShown) {
  Scene s;
  const int before = s.model.refreshes;
  const auto pending = s.root.pendingTimers();
  ASSERT_EQ(pending.size(), 1U);
  s.root.fireTimer(pending[0]);
  EXPECT_EQ(s.model.refreshes, before + 1);
  // 读完立刻排下一次
  EXPECT_EQ(s.root.pendingTimers().size(), 1U);
}

TEST(MediaPanelContent, UnsubscribesWhenDestroyed) {
  Scene s;
  EXPECT_EQ(s.model.handlers.size(), 1U);
  s.root.setContent(std::make_unique<rujian::ui::Widget>());
  EXPECT_TRUE(s.model.handlers.empty());
  EXPECT_TRUE(s.root.pendingTimers().empty());
}

}  // namespace
}  // namespace ember::win32
