/*
 * ------------------------------------------------------------------------
 * 名称：machines-content-test.cpp
 * 说明：“机型与 ROM”对话框内容的测试：说明与“打开 ROM 目录”链接、每个机型一行（未添加只有添加按钮，已添加有替换与移除，暂未支持置灰）、按钮调用模型、模型变化时更新、读取失败的说明、销毁后取消订阅。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-21
 * 备注：用控件库的离屏根容器测试，模型是假的；点击用根容器的事件，缩放 1.0。
 * ------------------------------------------------------------------------
 */

#include <gtest/gtest.h>

#include <map>
#include <memory>

#include "contracts/machines/machines-model.hpp"
#include "rujian/ui/root.hpp"
#include "ui/machines/machines-content.hpp"

namespace ember::win32 {
namespace {

/** 假的机型模型：记下操作，内容由测试直接改。 */
class FakeRom final : public contracts::RomModel {
 public:
  contracts::RomView current;
  std::vector<std::string> calls;
  std::map<int, std::function<void()>> handlers;

  [[nodiscard]] contracts::RomView view() const override { return current; }
  void choose(const std::string& machine) override { calls.push_back("choose " + machine); }
  void chooseDirectory() override { calls.push_back("chooseDirectory"); }
  void remove(const std::string& machine) override { calls.push_back("remove " + machine); }
  void openDirectory() override { calls.push_back("openDirectory"); }
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
  FakeRom rom;
  rujian::ui::Root root{rujian::ui::ThemeName::Dark, 1.0};
  ui::MachinesContent* content{nullptr};

  Scene() {
    rom.current.load = contracts::MachinesLoad::Ready;
    rom.current.rows = {{"sb97", "小霸王 97", "未添加", false, true}, {"sb2000", "小霸王 2000", "SB2K_ROM.NES（512 KB）", true, true}, {"bbg1", "步步高 软驱一号", "暂未支持", false, false}};
  }

  void open() {
    auto created = std::make_unique<ui::MachinesContent>(rom);
    content = created.get();
    root.setContent(std::move(created));
    root.layout({640, 420});
  }

  void click(const rujian::ui::Widget& widget) {
    const auto& box = widget.bounds();
    const rujian::ui::Point at{box.x + box.width / 2, box.y + box.height / 2};
    root.mouseDown(at, rujian::ui::MouseButton::Left, 0, 0);
    root.mouseUp(at, rujian::ui::MouseButton::Left, 0);
  }
};

TEST(MachinesContent, ShowsTheIntroTheLinkAndOneRowPerMachine) {
  Scene s;
  s.open();
  ASSERT_NE(s.content->openDirectoryLink(), nullptr);
  ASSERT_EQ(s.content->romRows().size(), 3U);
  // 未添加：只有“添加 ROM 文件”；已添加：“替换”和“移除”；暂未支持：按钮置灰
  const auto& first = s.content->romRows()[0];
  EXPECT_NE(first.add, nullptr);
  EXPECT_EQ(first.replace, nullptr);
  EXPECT_TRUE(first.add->enabled());
  const auto& second = s.content->romRows()[1];
  EXPECT_EQ(second.add, nullptr);
  ASSERT_NE(second.replace, nullptr);
  ASSERT_NE(second.remove, nullptr);
  const auto& third = s.content->romRows()[2];
  ASSERT_NE(third.add, nullptr);
  EXPECT_FALSE(third.add->enabled());
}

TEST(MachinesContent, TheAddFromDirectoryButtonSitsBetweenTheIntroAndTheList) {
  Scene s;
  s.open();
  const auto* button = s.content->addFromDirectoryButton();
  ASSERT_NE(button, nullptr);
  EXPECT_GT(button->bounds().width, 0);
  EXPECT_GT(button->bounds().height, 0);
  EXPECT_GT(button->bounds().y, s.content->openDirectoryLink()->bounds().y);
  EXPECT_LT(button->bounds().bottom(), s.content->romRows()[0].add->bounds().y);
}

TEST(MachinesContent, ButtonsAndTheLinkCallTheModel) {
  Scene s;
  s.open();
  s.click(*s.content->openDirectoryLink());
  s.click(*s.content->addFromDirectoryButton());
  s.click(*s.content->romRows()[0].add);
  s.click(*s.content->romRows()[1].replace);
  s.click(*s.content->romRows()[1].remove);
  EXPECT_EQ(s.rom.calls, (std::vector<std::string>{"openDirectory", "chooseDirectory", "choose sb97", "choose sb2000", "remove sb2000"}));
}

TEST(MachinesContent, RowsFollowTheModel) {
  Scene s;
  s.open();
  s.rom.current.rows = {{"sb97", "小霸王 97", "A.ROM（512 KB）", true, true}};
  s.rom.notify();
  ASSERT_EQ(s.content->romRows().size(), 1U);
  EXPECT_NE(s.content->romRows()[0].replace, nullptr);
}

TEST(MachinesContent, NoRowsWhileLoadingAndTheLinkIsStillThere) {
  Scene s;
  s.rom.current = contracts::RomView{};
  s.open();
  EXPECT_TRUE(s.content->romRows().empty());
  EXPECT_NE(s.content->openDirectoryLink(), nullptr);
  EXPECT_NE(s.content->addFromDirectoryButton(), nullptr);
  s.rom.current.load = contracts::MachinesLoad::Failed;
  s.rom.current.message = "核心未启动";
  s.rom.notify();
  EXPECT_TRUE(s.content->romRows().empty());
}

TEST(MachinesContent, DestroyingUnsubscribesFromTheModel) {
  Scene s;
  s.open();
  EXPECT_EQ(s.rom.handlers.size(), 1U);
  s.root.setContent(nullptr);
  EXPECT_TRUE(s.rom.handlers.empty());
}

TEST(MachinesContent, MinimumWidthKeepsEachRowOnOneLineAndHasNoHeightRequirement) {
  Scene s;
  s.open();
  const rujian::ui::Size minimum = s.content->minimumSize();
  EXPECT_GE(minimum.width, 460);
  EXPECT_DOUBLE_EQ(minimum.height, 0);
  s.root.layout({minimum.width, 420});
  const auto& second = s.content->romRows()[1];
  // 按钮在同一行，并靠列表的右边
  EXPECT_NEAR(second.replace->bounds().y, second.remove->bounds().y, 0.01);
  EXPECT_GT(second.replace->bounds().x, minimum.width / 2);
}

TEST(MachinesContent, MinimumWidthKeepsTheIntroReadableWhileThereIsNoList) {
  Scene s;
  s.rom.current = contracts::RomView{};
  s.open();
  EXPECT_GE(s.content->minimumSize().width, 460);
}

}  // namespace
}  // namespace ember::win32
