/*
 * ------------------------------------------------------------------------
 * 名称：main-request-test.cpp
 * 说明：请求路由与外壳自己处理的 15 个请求的测试：路由的拒绝规则、参数校验、取消与失败的结果形状、先记入再加载、拖入文件的核对。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-19
 * 备注：结果的形状与 schema 里 mainRequests 的定义一致。
 * ------------------------------------------------------------------------
 */

#include <gtest/gtest.h>

#include "contracts/core-gateway/request-lookup.hpp"
#include "entry/cartridges/cartridge-handlers.hpp"
#include "entry/machines/machine-handlers.hpp"
#include "entry/main-requests/main-request-router.hpp"
#include "entry/mappers/mapper-handlers.hpp"
#include "entry/media/media-handlers.hpp"
#include "entry/preferences/preference-handlers.hpp"
#include "entry/system/system-handlers.hpp"
#include "infra/dropped-files/memory-dropped-files-service.hpp"
#include "support/fakes.hpp"

namespace {

using namespace ember::win32;
using nlohmann::json;

/** 一套完整登记好的路由与它的假依赖。 */
struct Fixture {
  test::FakeCoreGateway core;
  test::FakeFileDialog dialog;
  test::FakeDirectory logDirectory;
  test::FakeDirectory romDirectory;
  test::FakeRecentCartridges recent;
  test::FakeErrorLogger logger;
  test::FakeUpdater updater;
  test::FakeHelp help;
  infra::MemoryDroppedFilesService dropped;
  entry::MainRequestRouter router{core, logger};

  Fixture() {
    router.add(entry::createCartridgeHandlers({core, dialog, dropped, recent}));
    router.add(entry::createMachineHandlers({core, dialog, romDirectory}));
    router.add(entry::createMapperHandlers({core, dialog}));
    router.add(entry::createMediaHandlers({core, dialog, dropped}));
    router.add(entry::createPreferenceHandlers({dialog}));
    router.add(entry::createSystemHandlers({logDirectory, help, updater}));
  }

  /** 发出请求并取回复（处理函数都是同步给出回复的）。 */
  contracts::CoreReply call(const std::string& name, json params = json::object()) {
    std::optional<contracts::CoreReply> reply;
    router.request(name, std::move(params), [&](contracts::CoreReply value) { reply = std::move(value); });
    EXPECT_TRUE(reply.has_value()) << name;
    return reply.value_or(contracts::CoreReply{});
  }
};

TEST(RequestCatalog, ComesFromTheSchemaAndHasTheSixteenShellRequests) {
  EXPECT_EQ(std::size(contracts::kShellRequests), 16u);
  EXPECT_EQ(contracts::coreRequestKind("runtime.status"), contracts::RequestKind::Ui);
  EXPECT_EQ(contracts::coreRequestKind("cartridges.load"), contracts::RequestKind::MainOnly);
  EXPECT_EQ(contracts::coreRequestKind("inspector.readMemory"), contracts::RequestKind::DebugOnly);
  EXPECT_FALSE(contracts::coreRequestKind("nope.nothing"));
  EXPECT_TRUE(contracts::isShellRequest("cartridges.open"));
  EXPECT_FALSE(contracts::isShellRequest("runtime.status"));
}

TEST(MainRequestRouter, ForwardsUiRequestsToTheCore) {
  Fixture f;
  f.core.replies["runtime.status"] = [](const json&) { return contracts::CoreReply::success({{"power", "off"}}); };
  const auto reply = f.call("runtime.status", {{"a", 1}});
  EXPECT_TRUE(reply.ok);
  EXPECT_EQ(reply.result["power"], "off");
  ASSERT_EQ(f.core.calls.size(), 1u);
  EXPECT_EQ(f.core.calls[0].params["a"], 1);
}

TEST(MainRequestRouter, RejectsUnknownMainOnlyAndDebugOnlyRequests) {
  Fixture f;
  for (const char* name : {"nope.nothing", "cartridges.load", "inspector.readMemory", "noDot", ""}) {
    const auto reply = f.call(name);
    EXPECT_FALSE(reply.ok) << name;
    EXPECT_EQ(reply.error.code, "core.unknownRequest") << name;
  }
  EXPECT_TRUE(f.core.calls.empty());
}

TEST(MainRequestRouter, ShellRequestsNeverReachTheCoreDirectly) {
  Fixture f;
  f.call("preferences.pickDirectory");
  EXPECT_TRUE(f.core.calls.empty());
}

TEST(MainRequestRouter, RegistrationRejectsForeignNamesAndDuplicates) {
  test::FakeCoreGateway core;
  test::FakeErrorLogger logger;
  entry::MainRequestRouter router(core, logger);
  entry::MainRequestHandlers foreign;
  foreign["runtime.status"] = [](const json&, contracts::CoreReplyHandler) {};
  EXPECT_THROW(router.add(foreign), std::logic_error);
  entry::MainRequestHandlers once;
  once["system.openHelp"] = [](const json&, contracts::CoreReplyHandler) {};
  router.add(once);
  EXPECT_THROW(router.add(once), std::logic_error);
}

TEST(MainRequestRouter, ShellRequestWithoutAHandlerIsUnknown) {
  test::FakeCoreGateway core;
  test::FakeErrorLogger logger;
  entry::MainRequestRouter router(core, logger);
  std::optional<contracts::CoreReply> reply;
  router.request("system.openHelp", json::object(), [&](contracts::CoreReply value) { reply = std::move(value); });
  ASSERT_TRUE(reply);
  EXPECT_EQ(reply->error.code, "core.unknownRequest");
}

TEST(MainRequestRouter, BadParametersGiveInvalidParamsAndUnexpectedExceptionsAreLogged) {
  Fixture f;
  const auto unknownField = f.call("cartridges.open", {{"path", "C:\\x.nes"}});
  EXPECT_EQ(unknownField.error.code, "core.invalidParams");
  EXPECT_NE(unknownField.error.message.find("请求参数不正确"), std::string::npos);
  EXPECT_EQ(f.call("cartridges.open", {{"droppedId", 0}}).error.code, "core.invalidParams");
  EXPECT_EQ(f.call("cartridges.open", {{"droppedId", "1"}}).error.code, "core.invalidParams");
  EXPECT_EQ(f.call("media.openFloppy", {{"drive", 4}}).error.code, "core.invalidParams");
  EXPECT_EQ(f.call("media.openFloppy").error.code, "core.invalidParams");
  EXPECT_EQ(f.call("machines.openRom", {{"machine", ""}}).error.code, "core.invalidParams");
  EXPECT_EQ(f.call("system.openHelp", {{"page", "other"}}).error.code, "core.invalidParams");
  EXPECT_EQ(f.call("cartridges.recent", json::array()).error.code, "core.invalidParams");
  EXPECT_TRUE(f.dialog.fileCalls.empty());

  entry::MainRequestHandlers broken;
  broken["system.openHelp"] = [](const json&, contracts::CoreReplyHandler) { throw std::runtime_error("boom"); };
  test::FakeCoreGateway core;
  test::FakeErrorLogger logger;
  entry::MainRequestRouter router(core, logger);
  router.add(broken);
  std::optional<contracts::CoreReply> reply;
  router.request("system.openHelp", json::object(), [&](contracts::CoreReply value) { reply = std::move(value); });
  ASSERT_TRUE(reply);
  EXPECT_EQ(reply->error.code, "core.internal");
  ASSERT_EQ(logger.entries.size(), 1u);
  EXPECT_EQ(logger.entries[0].context["name"], "system.openHelp");
}

TEST(CartridgeHandlers, OpenLoadsThePickedFileAndRemembersIt) {
  Fixture f;
  f.dialog.results.push_back("D:\\roms\\mario.nes");
  f.core.replies["cartridges.load"] = [](const json&) { return contracts::CoreReply::success({{"name", "mario"}}); };
  const auto reply = f.call("cartridges.open");
  ASSERT_TRUE(reply.ok);
  EXPECT_EQ(reply.result["canceled"], false);
  EXPECT_EQ(reply.result["loaded"]["name"], "mario");
  ASSERT_EQ(f.core.calls.size(), 1u);
  EXPECT_EQ(f.core.calls[0].name, "cartridges.load");
  EXPECT_EQ(f.core.calls[0].params["path"], "D:\\roms\\mario.nes");
  EXPECT_EQ(f.recent.paths, std::vector<std::string>{"D:\\roms\\mario.nes"});
  ASSERT_EQ(f.dialog.fileCalls.size(), 1u);
  EXPECT_EQ(f.dialog.fileCalls[0].purpose, "cartridge");
  EXPECT_EQ(f.dialog.fileCalls[0].title, "打开卡带");
  EXPECT_EQ(f.dialog.fileCalls[0].filters[0].extensions, std::vector<std::string>{"nes"});
}

TEST(CartridgeHandlers, OpenCanceledIsNotAnErrorAndLoadsNothing) {
  Fixture f;
  const auto reply = f.call("cartridges.open");
  ASSERT_TRUE(reply.ok);
  EXPECT_EQ(reply.result, json({{"canceled", true}}));
  EXPECT_TRUE(f.core.calls.empty());
  EXPECT_TRUE(f.recent.paths.empty());
}

TEST(CartridgeHandlers, FailedLoadRemovesTheFileFromRecentAndKeepsTheCoreError) {
  Fixture f;
  f.dialog.results.push_back("D:\\roms\\bad.nes");
  f.core.replies["cartridges.load"] = [](const json&) { return contracts::CoreReply::failure("cartridges.invalidHeader", "文件头不对"); };
  const auto reply = f.call("cartridges.open");
  EXPECT_FALSE(reply.ok);
  EXPECT_EQ(reply.error.code, "cartridges.invalidHeader");
  EXPECT_EQ(reply.error.message, "文件头不对");
  EXPECT_TRUE(f.recent.paths.empty());
}

TEST(CartridgeHandlers, RecentIsAddedBeforeTheCoreIsAskedToLoad) {
  Fixture f;
  f.dialog.results.push_back("D:\\roms\\a.nes");
  std::vector<std::string> seenWhileLoading;
  f.core.replies["cartridges.load"] = [&](const json&) {
    seenWhileLoading = f.recent.paths;
    return contracts::CoreReply::success(json::object());
  };
  f.call("cartridges.open");
  EXPECT_EQ(seenWhileLoading, std::vector<std::string>{"D:\\roms\\a.nes"});
}

TEST(CartridgeHandlers, DroppedFileIsUsedWithoutADialogAndCheckedByExtension) {
  Fixture f;
  const auto files = f.dropped.registerFiles({"D:\\drop\\game.NES", "D:\\drop\\readme.txt"});
  const auto ok = f.call("cartridges.open", {{"droppedId", files[0].id}});
  EXPECT_TRUE(ok.ok);
  EXPECT_TRUE(f.dialog.fileCalls.empty());
  EXPECT_EQ(f.core.calls[0].params["path"], "D:\\drop\\game.NES");

  const auto wrongType = f.call("cartridges.open", {{"droppedId", files[1].id}});
  EXPECT_EQ(wrongType.error.code, "cartridges.droppedNotFound");
  const auto missing = f.call("cartridges.open", {{"droppedId", 999}});
  EXPECT_EQ(missing.error.code, "cartridges.droppedNotFound");
  EXPECT_EQ(f.core.calls.size(), 1u);
}

TEST(CartridgeHandlers, RecentListsNamesAndIdsOnly) {
  Fixture f;
  f.recent.paths = {"D:\\roms\\a.nes", "D:\\roms\\b.nes"};
  const auto reply = f.call("cartridges.recent");
  ASSERT_TRUE(reply.ok);
  ASSERT_EQ(reply.result["items"].size(), 2u);
  EXPECT_EQ(reply.result["items"][0], json({{"id", 1}, {"name", "a.nes"}}));
  EXPECT_EQ(reply.result.dump().find("D:\\\\roms"), std::string::npos);
}

TEST(CartridgeHandlers, OpenRecentLoadsByIdAndRejectsUnknownIds) {
  Fixture f;
  f.recent.paths = {"D:\\roms\\a.nes", "D:\\roms\\b.nes"};
  f.core.replies["cartridges.load"] = [](const json&) { return contracts::CoreReply::success({{"name", "b"}}); };
  const auto reply = f.call("cartridges.openRecent", {{"id", 2}});
  ASSERT_TRUE(reply.ok);
  EXPECT_EQ(reply.result["loaded"]["name"], "b");
  EXPECT_EQ(f.core.calls[0].params["path"], "D:\\roms\\b.nes");
  EXPECT_EQ(f.recent.paths.front(), "D:\\roms\\b.nes");
  EXPECT_EQ(f.call("cartridges.openRecent", {{"id", 50}}).error.code, "cartridges.recentNotFound");
  EXPECT_EQ(f.call("cartridges.openRecent").error.code, "core.invalidParams");
}

TEST(MachineHandlers, OpenRomPassesTheMachineAndThePickedFileToTheCore) {
  Fixture f;
  f.dialog.results.push_back("D:\\rom\\sb2000_bios.bin");
  f.core.replies["machines.setRom"] = [](const json&) { return contracts::CoreReply::success({{"machine", {{"id", "sb2000"}}}}); };
  const auto reply = f.call("machines.openRom", {{"machine", "sb2000"}});
  ASSERT_TRUE(reply.ok);
  EXPECT_EQ(reply.result["canceled"], false);
  EXPECT_EQ(reply.result["machine"]["id"], "sb2000");
  EXPECT_EQ(f.core.calls[0].params, json({{"machine", "sb2000"}, {"path", "D:\\rom\\sb2000_bios.bin"}}));
  EXPECT_EQ(f.dialog.fileCalls[0].purpose, "rom");
  EXPECT_EQ(f.dialog.fileCalls[0].filters[0].extensions, (std::vector<std::string>{"bin"}));
}

TEST(MachineHandlers, OpenRomsPassesThePickedDirectoryToTheCoreAndReturnsTheSummary) {
  Fixture f;
  f.dialog.results.push_back("D:\\rom");
  f.core.replies["machines.setRoms"] = [](const json&) { return contracts::CoreReply::success({{"added", json::array({{{"id", "sb2000"}}})}, {"skipped", json::array({{{"fileName", "a.bin"}, {"reason", "unrecognized"}}})}}); };
  const auto reply = f.call("machines.openRoms");
  ASSERT_TRUE(reply.ok);
  EXPECT_EQ(reply.result["canceled"], false);
  EXPECT_EQ(reply.result["added"][0]["id"], "sb2000");
  EXPECT_EQ(reply.result["skipped"][0]["reason"], "unrecognized");
  EXPECT_EQ(f.core.calls[0].params, json({{"path", "D:\\rom"}}));
  EXPECT_EQ(f.dialog.directoryCalls[0].purpose, "romFolder");
}

TEST(MachineHandlers, OpenRomsCanceledAndFailed) {
  Fixture f;
  EXPECT_EQ(f.call("machines.openRoms").result, json({{"canceled", true}}));
  EXPECT_TRUE(f.core.calls.empty());
  f.dialog.results.push_back("D:\\nowhere");
  f.core.replies["machines.setRoms"] = [](const json&) { return contracts::CoreReply::failure("machines.fileNotFound", "找不到目录"); };
  EXPECT_EQ(f.call("machines.openRoms").error.code, "machines.fileNotFound");
}

TEST(MachineHandlers, OpenRomCanceledAndFailedRom) {
  Fixture f;
  EXPECT_EQ(f.call("machines.openRom", {{"machine", "fc"}}).result, json({{"canceled", true}}));
  f.dialog.results.push_back("D:\\x.bin");
  f.core.replies["machines.setRom"] = [](const json&) { return contracts::CoreReply::failure("machines.romInvalid", "无效"); };
  EXPECT_EQ(f.call("machines.openRom", {{"machine", "fc"}}).error.code, "machines.romInvalid");
}

TEST(MachineHandlers, OpenRomDirectoryReportsFailureWithTheModuleCode) {
  Fixture f;
  EXPECT_TRUE(f.call("machines.openRomDirectory").ok);
  EXPECT_EQ(f.romDirectory.opened, 1);
  f.romDirectory.error = "拒绝访问";
  const auto reply = f.call("machines.openRomDirectory");
  EXPECT_EQ(reply.error.code, "machines.romDirectoryUnavailable");
  EXPECT_EQ(reply.error.message, "无法打开 ROM 目录：拒绝访问");
}

TEST(MapperHandlers, OpenPackageInstallsThePickedPackage) {
  Fixture f;
  f.dialog.results.push_back("D:\\plugins\\mmc3.emap");
  f.core.replies["mappers.install"] = [](const json&) { return contracts::CoreReply::success({{"plugin", {{"id", "mmc3"}}}}); };
  const auto reply = f.call("mappers.openPackage");
  ASSERT_TRUE(reply.ok);
  EXPECT_EQ(reply.result["plugin"]["id"], "mmc3");
  EXPECT_EQ(f.core.calls[0].params["path"], "D:\\plugins\\mmc3.emap");
  EXPECT_EQ(f.dialog.fileCalls[0].purpose, "mapperPackage");
  EXPECT_EQ(f.call("mappers.openPackage").result, json({{"canceled", true}}));
}

TEST(MediaHandlers, OpenFloppyInsertsIntoTheRequestedDrive) {
  Fixture f;
  f.dialog.results.push_back("D:\\disks\\a.img");
  f.core.replies["media.floppyInsert"] = [](const json&) { return contracts::CoreReply::success({{"status", {{"drive", 1}}}}); };
  const auto reply = f.call("media.openFloppy", {{"drive", 1}});
  ASSERT_TRUE(reply.ok);
  EXPECT_EQ(reply.result["status"]["drive"], 1);
  EXPECT_EQ(f.core.calls[0].params, json({{"drive", 1}, {"path", "D:\\disks\\a.img"}}));
  EXPECT_EQ(f.dialog.fileCalls[0].purpose, "floppy");
  EXPECT_EQ(f.call("media.openFloppy", {{"drive", 0}}).result, json({{"canceled", true}}));
}

TEST(MediaHandlers, OpenFloppyAndTapeCheckDroppedFilesByTheirOwnExtensions) {
  Fixture f;
  const auto files = f.dropped.registerFiles({"D:\\d\\a.IMG", "D:\\d\\song.mp3", "D:\\d\\game.nes"});
  EXPECT_TRUE(f.call("media.openFloppy", {{"drive", 0}, {"droppedId", files[0].id}}).ok);
  EXPECT_EQ(f.core.calls.back().params["path"], "D:\\d\\a.IMG");
  EXPECT_EQ(f.call("media.openFloppy", {{"drive", 0}, {"droppedId", files[1].id}}).error.code, "media.droppedNotFound");
  EXPECT_TRUE(f.call("media.openTape", {{"droppedId", files[1].id}}).ok);
  EXPECT_EQ(f.core.calls.back().name, "media.tapeLoad");
  EXPECT_EQ(f.call("media.openTape", {{"droppedId", files[2].id}}).error.code, "media.droppedNotFound");
  EXPECT_TRUE(f.dialog.fileCalls.empty());
}

TEST(MediaHandlers, OpenTapeAndRecordingUseTheirOwnDialogs) {
  Fixture f;
  f.dialog.results.push_back("D:\\tapes\\a.wav");
  f.core.replies["media.tapeLoad"] = [](const json&) { return contracts::CoreReply::success({{"status", {{"state", "loaded"}}}}); };
  EXPECT_EQ(f.call("media.openTape").result["status"]["state"], "loaded");
  EXPECT_EQ(f.dialog.fileCalls[0].purpose, "tape");
  EXPECT_EQ(f.dialog.fileCalls[0].filters.back().extensions, std::vector<std::string>{"*"});

  f.dialog.results.push_back("D:\\tapes\\rec.wav");
  f.core.replies["media.tapeStatus"] = [](const json&) { return contracts::CoreReply::success({{"status", {{"loaded", false}}}}); };
  const auto recording = f.call("media.openTapeRecording");
  ASSERT_TRUE(recording.ok);
  EXPECT_EQ(f.dialog.saveCalls[0].purpose, "tapeRecording");
  EXPECT_EQ(f.core.calls.back().name, "media.tapeRecord");
  EXPECT_EQ(f.core.calls.back().params["path"], "D:\\tapes\\rec.wav");
  EXPECT_EQ(f.call("media.openTapeRecording").result, json({{"canceled", true}}));
}

TEST(MediaHandlers, RecordingOnALoadedTapeSkipsTheDialogAndRecordsOverTheTape) {
  Fixture f;
  f.core.replies["media.tapeStatus"] = [](const json&) { return contracts::CoreReply::success({{"status", {{"loaded", true}}}}); };
  f.core.replies["media.tapeRecord"] = [](const json&) { return contracts::CoreReply::success({{"status", {{"mode", "recording"}}}}); };
  const auto reply = f.call("media.openTapeRecording");
  ASSERT_TRUE(reply.ok);
  EXPECT_EQ(reply.result["status"]["mode"], "recording");
  EXPECT_TRUE(f.dialog.saveCalls.empty()) << "已有磁带时不弹保存窗口";
  EXPECT_EQ(f.core.calls.back().name, "media.tapeRecord");
  EXPECT_FALSE(f.core.calls.back().params.contains("path")) << "没有文件路径：在已有的磁带上录";
  f.core.replies["media.tapeRecord"] = [](const json&) { return contracts::CoreReply::failure("media.tapeNotWritable", "磁带文件不是 WAV"); };
  EXPECT_EQ(f.call("media.openTapeRecording").error.code, "media.tapeNotWritable");
}

TEST(PreferenceHandlers, PickDirectoryReturnsThePathOrCanceled) {
  Fixture f;
  f.dialog.results.push_back("D:\\out");
  EXPECT_EQ(f.call("preferences.pickDirectory").result, json({{"canceled", false}, {"path", "D:\\out"}}));
  EXPECT_EQ(f.dialog.directoryCalls[0].purpose, "folder");
  EXPECT_EQ(f.call("preferences.pickDirectory").result, json({{"canceled", true}}));
}

TEST(SystemHandlers, LogDirectoryAndHelpReportFailuresWithTheModuleCodes) {
  Fixture f;
  EXPECT_TRUE(f.call("system.openLogDirectory").ok);
  f.logDirectory.error = "没有权限";
  const auto log = f.call("system.openLogDirectory");
  EXPECT_EQ(log.error.code, "system.logDirectoryUnavailable");
  EXPECT_EQ(log.error.message, "无法打开日志目录：没有权限");

  EXPECT_TRUE(f.call("system.openHelp").ok);
  EXPECT_TRUE(f.call("system.openHelp", {{"page", "changelog"}}).ok);
  ASSERT_EQ(f.help.pages.size(), 2u);
  EXPECT_FALSE(f.help.pages[0]);
  EXPECT_EQ(f.help.pages[1], "changelog");
  f.help.error = "浏览器过旧";
  EXPECT_EQ(f.call("system.openHelp").error.code, "system.helpUnavailable");
}

TEST(SystemHandlers, UpdateStatusCheckAndInstall) {
  Fixture f;
  f.updater.current = {"ready", std::string("1.2.0"), std::nullopt};
  EXPECT_EQ(f.call("system.updateStatus").result, json({{"status", {{"state", "ready"}, {"version", "1.2.0"}}}}));
  const auto checked = f.call("system.checkUpdate");
  EXPECT_EQ(f.updater.checks, 1);
  EXPECT_EQ(checked.result["status"]["state"], "checking");
  EXPECT_EQ(f.call("system.installUpdate").error.code, "system.updateNotReady");
  f.updater.installResult = true;
  EXPECT_TRUE(f.call("system.installUpdate").ok);
}

}  // namespace
