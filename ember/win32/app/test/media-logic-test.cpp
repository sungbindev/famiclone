/*
 * ------------------------------------------------------------------------
 * 名称：media-logic-test.cpp
 * 说明：介质模块逻辑的测试：纯规则（盘片、状态与位置文字，按钮的可用状态，设备面板的内容）、介质模块控制器（初始读取、载入、取出、写保护、磁带的播放与录音、事件、通知、核心重启、断开）与介质模块的登记项（读写灯、软盘名、拖入文件的处理）。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-19
 * 备注：无。
 * ------------------------------------------------------------------------
 */

#include <gtest/gtest.h>

#include "logic/media/media-controller.hpp"
#include "logic/media/media-module.hpp"
#include "logic/media/media-rules.hpp"
#include "support/fakes.hpp"

namespace ember::win32 {
namespace {

using contracts::CoreReply;
using nlohmann::json;
using logic::FloppyState;
using logic::PrinterMode;
using logic::TapeMode;
using logic::TapeState;

FloppyState floppy(int drive, bool inserted, const std::string& file = "", bool motor = false, int cylinder = 0) {
  FloppyState state;
  state.drive = drive;
  state.inserted = inserted;
  state.fileName = file;
  state.motor = motor;
  state.cylinder = cylinder;
  return state;
}

json floppyJson(int drive, bool inserted, const std::string& file = "", bool motor = false, int cylinder = 0, bool modified = false, bool protectedDisk = false) {
  json item = {{"drive", drive}, {"inserted", inserted}, {"writeProtected", protectedDisk}, {"modified", modified}, {"size", 0}, {"cylinder", cylinder}, {"motor", motor}, {"faults", json::object()}};
  if (!file.empty()) item["fileName"] = file;
  return item;
}

json tapeJson(bool connected, bool loaded, const std::string& mode = "stopped", const std::string& file = "", double position = 0, double duration = 0) {
  json status = {{"connected", connected}, {"loaded", loaded}, {"mode", mode}, {"position", position}, {"duration", duration}, {"stereo", false}};
  if (!file.empty()) status["fileName"] = file;
  return status;
}

TEST(MediaRules, ParsesStatusesAndIgnoresBadShapes) {
  const auto floppies = logic::parseFloppies({{"drives", json::array({floppyJson(1, false), floppyJson(0, true, "a.img", true, 12)})}});
  ASSERT_EQ(floppies.size(), 2U);
  EXPECT_EQ(floppies[0].drive, 0);
  EXPECT_EQ(floppies[0].fileName, "a.img");
  EXPECT_TRUE(floppies[0].motor);
  EXPECT_EQ(floppies[0].cylinder, 12);
  EXPECT_TRUE(logic::parseFloppies(json::object()).empty());
  EXPECT_TRUE(logic::parseFloppies(json::array()).empty());
  EXPECT_FALSE(logic::parseTape(json::object()).has_value());
  const auto tape = logic::parseTape(tapeJson(true, true, "playing", "a.wav", 5.5, 61));
  ASSERT_TRUE(tape.has_value());
  EXPECT_TRUE(tape->connected && tape->loaded);
  EXPECT_EQ(tape->mode, TapeMode::Playing);
  EXPECT_FALSE(logic::parsePrinter(json::object()).has_value());
  EXPECT_EQ(logic::parsePrinter({{"status", {{"state", "done"}}}}), PrinterMode::Done);
  EXPECT_EQ(logic::parsePrinter({{"status", {{"state", "printing"}}}}), PrinterMode::Printing);
  EXPECT_EQ(logic::parsePrinter({{"status", {{"state", "idle"}}}}), PrinterMode::Idle);
}

TEST(MediaRules, FloppyTexts) {
  EXPECT_EQ(logic::driveLabel(0), "A");
  EXPECT_EQ(logic::driveLabel(1), "B");
  EXPECT_EQ(logic::diskText(std::nullopt), "空");
  EXPECT_EQ(logic::diskText(floppy(0, false)), "空");
  EXPECT_EQ(logic::diskText(floppy(0, true, "a.img")), "a.img");
  auto modified = floppy(0, true, "a.img");
  modified.modified = true;
  EXPECT_EQ(logic::diskText(modified), "a.img（有未保存的修改）");
  EXPECT_EQ(logic::floppyStateText(std::nullopt), "无盘片");
  EXPECT_EQ(logic::floppyStateText(floppy(0, false)), "无盘片");
  EXPECT_EQ(logic::floppyStateText(floppy(0, true, "a.img")), "空闲");
  EXPECT_EQ(logic::floppyStateText(floppy(0, true, "a.img", true, 7)), "读写中（磁道 7）");
}

TEST(MediaRules, TapeAndPrinterTexts) {
  EXPECT_EQ(logic::printerStateText(std::nullopt), "空闲");
  EXPECT_EQ(logic::printerStateText(PrinterMode::Printing), "打印中");
  EXPECT_EQ(logic::printerStateText(PrinterMode::Done), "打印完成");
  EXPECT_EQ(logic::tapeNameText(std::nullopt), "空");
  EXPECT_EQ(logic::tapeStateText(std::nullopt), "无磁带");
  EXPECT_EQ(logic::tapePositionText(std::nullopt), "—");
  const auto noTape = logic::parseTape(tapeJson(true, false));
  EXPECT_EQ(logic::tapeStateText(noTape), "无磁带");
  EXPECT_EQ(logic::tapePositionText(noTape), "—");
  const auto stopped = logic::parseTape(tapeJson(true, true, "stopped", "a.wav", 5, 0));
  EXPECT_EQ(logic::tapeNameText(stopped), "a.wav");
  EXPECT_EQ(logic::tapeStateText(stopped), "已停止");
  EXPECT_EQ(logic::tapePositionText(stopped), "0:05");
  EXPECT_EQ(logic::tapeStateText(logic::parseTape(tapeJson(true, true, "playing", "a.wav"))), "播放中");
  EXPECT_EQ(logic::tapeStateText(logic::parseTape(tapeJson(true, true, "recording", "a.wav"))), "录音中");
  EXPECT_EQ(logic::tapePositionText(logic::parseTape(tapeJson(true, true, "playing", "a.wav", 65, 1689))), "1:05 / 28:09");
  EXPECT_EQ(logic::tapePositionText(logic::parseTape(tapeJson(true, true, "playing", "a.wav", 3725, 7325))), "1:02:05 / 2:02:05");
}

TEST(MediaRules, FloppyButtonsFollowTheDisk) {
  auto none = logic::floppyAvailability(std::nullopt);
  EXPECT_FALSE(none.insert || none.eject || none.writeProtect);
  auto empty = logic::floppyAvailability(floppy(0, false));
  EXPECT_TRUE(empty.insert);
  EXPECT_FALSE(empty.eject || empty.writeProtect);
  auto loaded = logic::floppyAvailability(floppy(0, true, "a.img"));
  EXPECT_TRUE(loaded.insert && loaded.eject && loaded.writeProtect);
}

TEST(MediaRules, TapeButtonsFollowTheTapeAndTheHost) {
  auto none = logic::tapeAvailability(std::nullopt);
  EXPECT_FALSE(none.eject || none.play || none.record || none.stop);
  // 没有磁带：播放始终可点（选磁带），录音要接上主机
  auto emptyOff = logic::tapeAvailability(logic::parseTape(tapeJson(false, false)));
  EXPECT_TRUE(emptyOff.play);
  EXPECT_FALSE(emptyOff.eject || emptyOff.record || emptyOff.stop);
  EXPECT_TRUE(logic::tapeAvailability(logic::parseTape(tapeJson(true, false))).record);
  // 有磁带但没通电：只能取出
  auto loadedOff = logic::tapeAvailability(logic::parseTape(tapeJson(false, true, "stopped", "a.wav")));
  EXPECT_TRUE(loadedOff.eject);
  EXPECT_FALSE(loadedOff.play || loadedOff.record || loadedOff.stop);
  auto loadedOn = logic::tapeAvailability(logic::parseTape(tapeJson(true, true, "stopped", "a.wav")));
  EXPECT_TRUE(loadedOn.eject && loadedOn.play && loadedOn.record);
  EXPECT_FALSE(loadedOn.stop);
  // 播放或录音中只有停止与取出
  auto playing = logic::tapeAvailability(logic::parseTape(tapeJson(true, true, "playing", "a.wav")));
  EXPECT_TRUE(playing.stop && playing.eject);
  EXPECT_FALSE(playing.play || playing.record);
}

TEST(MediaRules, ViewAlwaysListsFloppyATapeAndPrinter) {
  const auto empty = logic::buildMediaView({}, std::nullopt, std::nullopt);
  ASSERT_EQ(empty.floppies.size(), 1U);
  EXPECT_EQ(empty.floppies[0].label, "软驱 A");
  EXPECT_EQ(empty.floppies[0].disk, "空");
  EXPECT_FALSE(empty.floppies[0].canInsert);
  EXPECT_EQ(empty.tape.disk, "空");
  EXPECT_EQ(empty.printer, "空闲");
  auto locked = floppy(0, true, "a.img");
  locked.writeProtected = true;
  const auto view = logic::buildMediaView({locked}, logic::parseTape(tapeJson(true, true, "stopped", "t.wav", 3, 10)), PrinterMode::Printing);
  EXPECT_TRUE(view.floppies[0].writeProtected);
  EXPECT_TRUE(view.floppies[0].canEject);
  EXPECT_EQ(view.tape.position, "0:03 / 0:10");
  EXPECT_EQ(view.printer, "打印中");
}

/** 带假核心的介质模块控制器。 */
struct MediaFixture {
  test::FakeCoreGateway core;
  test::FakeBroker broker{core};
  test::FakeShellApi shell;
  logic::MediaController controller{broker, core};
  std::function<void()> detach;
  std::function<void()> unsubscribe;
  int changes{0};

  MediaFixture() {
    reply("media.floppyStatus", {{"drives", json::array({floppyJson(0, true, "a.img")})}});
    reply("media.printerStatus", {{"status", {{"state", "idle"}, {"hasContent", false}, {"pages", 0}}}});
    reply("media.tapeStatus", {{"status", tapeJson(true, false)}});
    unsubscribe = controller.subscribe([this] { ++changes; });
  }

  void attach() { detach = controller.attach(shell); }
  void reply(const std::string& name, const json& result) {
    core.replies[name] = [result](const json&) { return CoreReply::success(result); };
  }
  void fail(const std::string& name, const std::string& message) {
    core.replies[name] = [message](const json&) { return CoreReply::failure("x.failed", message); };
  }
  [[nodiscard]] int count(const std::string& name) const {
    int total = 0;
    for (const auto& call : core.calls) total += call.name == name ? 1 : 0;
    return total;
  }
  [[nodiscard]] const test::FakeCoreGateway::Call* last(const std::string& name) const {
    for (auto it = core.calls.rbegin(); it != core.calls.rend(); ++it) {
      if (it->name == name) return &*it;
    }
    return nullptr;
  }
};

TEST(MediaController, ReadsFloppiesPrinterAndTapeWhenAttached) {
  MediaFixture f;
  EXPECT_EQ(f.controller.view().floppies[0].canInsert, false);
  f.attach();
  ASSERT_EQ(f.controller.floppies().size(), 1U);
  EXPECT_EQ(f.controller.view().floppies[0].disk, "a.img");
  EXPECT_TRUE(f.controller.view().floppies[0].canEject);
  EXPECT_TRUE(f.controller.tape().has_value());
  EXPECT_GE(f.changes, 1);
  EXPECT_GE(f.shell.refreshes, 1);
}

TEST(MediaController, FloppyOperationsSendTheirRequestsAndRefresh) {
  MediaFixture f;
  f.attach();
  const int before = f.count("media.floppyStatus");
  f.controller.insertFloppy(0);
  ASSERT_NE(f.last("media.openFloppy"), nullptr);
  EXPECT_EQ(f.last("media.openFloppy")->params, (json{{"drive", 0}}));
  f.controller.insertDroppedFloppy(0, 7);
  EXPECT_EQ(f.last("media.openFloppy")->params, (json{{"drive", 0}, {"droppedId", 7}}));
  f.controller.ejectFloppy(0);
  EXPECT_EQ(f.last("media.floppyEject")->params, (json{{"drive", 0}}));
  f.controller.setWriteProtected(0, true);
  EXPECT_EQ(f.last("media.floppyWriteProtect")->params, (json{{"drive", 0}, {"writeProtected", true}}));
  EXPECT_EQ(f.count("media.floppyStatus"), before + 4);
}

TEST(MediaController, FailedOperationsAreReportedByNotification) {
  MediaFixture f;
  f.attach();
  f.fail("media.openFloppy", "文件无效");
  f.controller.insertFloppy(0);
  ASSERT_EQ(f.shell.notices.size(), 1U);
  EXPECT_EQ(f.shell.notices[0].first, "载入软盘镜像失败：文件无效");
  EXPECT_EQ(f.shell.notices[0].second, contracts::NotificationLevel::Error);
  f.fail("media.floppyEject", "写回失败");
  f.controller.ejectFloppy(0);
  EXPECT_EQ(f.shell.notices.back().first, "取出软盘失败：写回失败");
}

TEST(MediaController, TapeOperationsSendTheirRequests) {
  MediaFixture f;
  f.reply("media.tapeStatus", {{"status", tapeJson(true, true, "stopped", "a.wav")}});
  f.attach();
  f.controller.playTape();
  EXPECT_EQ(f.count("media.tapePlay"), 1);
  EXPECT_EQ(f.count("media.openTape"), 0);
  f.controller.stopTape();
  EXPECT_EQ(f.count("media.tapeStop"), 1);
  f.controller.ejectTape();
  EXPECT_EQ(f.count("media.tapeEject"), 1);
  f.controller.recordTape();
  EXPECT_EQ(f.count("media.openTapeRecording"), 1);
  f.controller.loadDroppedTape(9);
  EXPECT_EQ(f.last("media.openTape")->params, (json{{"droppedId", 9}}));
}

TEST(MediaController, PlayWithoutATapePicksOneAndPlaysOnlyWhenTheTapeDeckIsConnected) {
  MediaFixture f;
  f.attach();
  // 已通电：选定后立即播放
  f.reply("media.openTape", {{"canceled", false}, {"status", tapeJson(true, true, "stopped", "a.wav")}});
  f.controller.playTape();
  EXPECT_EQ(f.count("media.openTape"), 1);
  EXPECT_EQ(f.count("media.tapePlay"), 1);
  // 没通电：只载入
  f.reply("media.openTape", {{"canceled", false}, {"status", tapeJson(false, true, "stopped", "a.wav")}});
  f.controller.playTape();
  EXPECT_EQ(f.count("media.tapePlay"), 1);
  // 用户取消：什么都不发生
  f.reply("media.openTape", {{"canceled", true}});
  f.controller.playTape();
  EXPECT_EQ(f.count("media.tapePlay"), 1);
  EXPECT_TRUE(f.shell.notices.empty());
  // 选磁带失败：通知
  f.fail("media.openTape", "无法解码");
  f.controller.playTape();
  EXPECT_EQ(f.shell.notices.back().first, "播放磁带失败：无法解码");
}

TEST(MediaController, EventsRefreshStateAndNotify) {
  MediaFixture f;
  f.attach();
  const int floppyReads = f.count("media.floppyStatus");
  f.core.emitEvent("media.changed", {{"device", "floppy"}, {"drive", 0}});
  f.core.emitEvent("runtime.stateChanged", json::object());
  f.core.emitEvent("machines.changed", json::object());
  EXPECT_EQ(f.count("media.floppyStatus"), floppyReads + 3);
  f.core.emitEvent("media.printed", {{"fileName", "p1.png"}, {"page", 3}});
  ASSERT_EQ(f.shell.notices.size(), 1U);
  EXPECT_EQ(f.shell.notices[0].first, "打印机输出了第 3 页：p1.png");
  EXPECT_EQ(f.shell.notices[0].second, contracts::NotificationLevel::Info);
  f.core.emitEvent("media.printFailed", {{"message", "目录无法写入"}});
  EXPECT_EQ(f.shell.notices.back().first, "打印机输出失败：目录无法写入");
  f.core.emitEvent("media.floppySaveFailed", {{"drive", 0}, {"message", "文件被占用"}});
  EXPECT_EQ(f.shell.notices.back().first, "自动保存软盘镜像失败：文件被占用");
  // 事件数据格式不对时不通知
  const auto notices = f.shell.notices.size();
  f.core.emitEvent("media.printed", {{"page", 3}});
  EXPECT_EQ(f.shell.notices.size(), notices);
}

TEST(MediaController, RefreshWithoutChangeDoesNotNotify) {
  MediaFixture f;
  f.attach();
  const int changes = f.changes;
  const int refreshes = f.shell.refreshes;
  f.controller.refresh();
  EXPECT_EQ(f.changes, changes);
  EXPECT_EQ(f.shell.refreshes, refreshes);
  f.reply("media.floppyStatus", {{"drives", json::array({floppyJson(0, true, "a.img", true, 5)})}});
  f.controller.refresh();
  EXPECT_EQ(f.changes, changes + 1);
}

TEST(MediaController, CoreRestartClearsAndRereads) {
  MediaFixture f;
  f.attach();
  f.core.setState(contracts::CoreState::Starting);
  EXPECT_TRUE(f.controller.floppies().empty());
  EXPECT_FALSE(f.controller.tape().has_value());
  EXPECT_FALSE(f.controller.view().floppies[0].canInsert);
  f.core.setState(contracts::CoreState::Running);
  EXPECT_EQ(f.controller.floppies().size(), 1U);
}

TEST(MediaController, DetachingStopsEverything) {
  MediaFixture f;
  f.attach();
  f.detach();
  const int calls = static_cast<int>(f.core.calls.size());
  f.core.emitEvent("media.changed", {{"device", "tape"}, {"drive", 0}});
  f.controller.insertFloppy(0);
  f.controller.refresh();
  EXPECT_EQ(static_cast<int>(f.core.calls.size()), calls);
  EXPECT_EQ(f.core.subscriberCount(), 0U);
}

TEST(MediaModule, PanelLampFollowsTheFloppyMotors) {
  MediaFixture f;
  const auto module = logic::createMediaModule(f.controller);
  ASSERT_EQ(module.bottomPanels.size(), 1U);
  const auto& panel = module.bottomPanels[0];
  EXPECT_EQ(panel.id, "media.devices");
  EXPECT_EQ(panel.title, "设备");
  EXPECT_EQ(panel.icon, contracts::ShellIcon::DeviceFloppy);
  EXPECT_TRUE(panel.autoCollapse);
  // 没有读到软驱：不显示灯
  EXPECT_FALSE(panel.lamp().has_value());
  f.attach();
  auto lamp = panel.lamp();
  ASSERT_TRUE(lamp.has_value());
  EXPECT_FALSE(lamp->lit);
  EXPECT_EQ(lamp->label, "软驱读写灯");
  ASSERT_TRUE(lamp->inIcon.has_value());
  EXPECT_DOUBLE_EQ(lamp->inIcon->first, 50);
  EXPECT_DOUBLE_EQ(lamp->inIcon->second, 60);
  f.reply("media.floppyStatus", {{"drives", json::array({floppyJson(0, true, "a.img", true, 3)})}});
  f.controller.refresh();
  EXPECT_TRUE(panel.lamp()->lit);
}

TEST(MediaModule, StatusItemsShowOnlyLoadedFloppies) {
  MediaFixture f;
  const auto module = logic::createMediaModule(f.controller);
  ASSERT_EQ(module.statusItems.size(), 4U);
  EXPECT_EQ(module.statusItems[0].id, "media.floppy.0");
  for (const auto& item : module.statusItems) EXPECT_FALSE(item.alignEnd);
  EXPECT_FALSE(module.statusItems[0].view().has_value());
  f.attach();
  const auto view = module.statusItems[0].view();
  ASSERT_TRUE(view.has_value());
  EXPECT_EQ(view->text, "a.img");
  EXPECT_EQ(view->title, "软驱 A 的盘片");
  EXPECT_FALSE(module.statusItems[1].view().has_value());
  // 有多个软驱时每项前面带驱动器名；没有盘片的软驱不显示
  f.reply("media.floppyStatus", {{"drives", json::array({floppyJson(0, true, "a.img"), floppyJson(1, true, "b.img"), floppyJson(2, false)})}});
  f.controller.refresh();
  EXPECT_EQ(module.statusItems[0].view()->text, "A: a.img");
  EXPECT_EQ(module.statusItems[1].view()->text, "B: b.img");
  EXPECT_EQ(module.statusItems[1].view()->title, "软驱 B 的盘片");
  EXPECT_FALSE(module.statusItems[2].view().has_value());
}

TEST(MediaModule, DroppedFilesAreClaimedByExtension) {
  MediaFixture f;
  const auto module = logic::createMediaModule(f.controller);
  ASSERT_EQ(module.fileHandlers.size(), 2U);
  EXPECT_EQ(module.fileHandlers[0].id, "media.floppyImage");
  EXPECT_EQ(module.fileHandlers[0].extensions, (std::vector<std::string>{"img", "ima", "vfd", "dsk", "flp"}));
  EXPECT_EQ(module.fileHandlers[1].id, "media.tapeSound");
  EXPECT_NE(std::find(module.fileHandlers[1].extensions.begin(), module.fileHandlers[1].extensions.end(), "wav"), module.fileHandlers[1].extensions.end());
  f.attach();
  module.fileHandlers[0].open({5, "disk.img"});
  EXPECT_EQ(f.last("media.openFloppy")->params, (json{{"drive", 0}, {"droppedId", 5}}));
  module.fileHandlers[1].open({6, "song.wav"});
  EXPECT_EQ(f.last("media.openTape")->params, (json{{"droppedId", 6}}));
}

}  // namespace
}  // namespace ember::win32
