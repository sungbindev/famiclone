/*
 * ------------------------------------------------------------------------
 * 名称：media-tape-test.cpp
 * 说明：介质服务里磁带机的测试：没有主机与没有磁带接口时磁带照样可以载入与取出、播放与录音需要通电的主机、载入、播放、停止、取出的状态与错误码、播放时不能载入但可以取出、录音停止后文件自动成为磁带、断电与换卡带后磁带留在里面、media.changed 事件。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-16
 * 备注：用真实的运行服务与 fc 主机（真实的 nrom 插件装配的空转卡带）、内存里的磁带存取替身；还有用真实的 FFmpeg 库解码的端到端用例。
 * ------------------------------------------------------------------------
 */

#include <gtest/gtest.h>

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <map>
#include <memory>
#include <string>
#include <thread>
#include <utility>
#include <vector>

#include "../support/runtime-test-support.hpp"
#include "bootstrap/machines/machine-catalog.hpp"
#include "contracts/media/media-error.hpp"
#include "contracts/media/tape-store.hpp"
#include "infra/media/disk-tape-store.hpp"
#include "logic/cartridges/ines-cartridge-loader.hpp"
#include "logic/media/media-manager.hpp"
#include "logic/runtime/simulation-runtime.hpp"
#include "shared/errors/core-error.hpp"

namespace {

namespace api = ember::contracts::media;
namespace hw = ember::contracts::hardware;
namespace rt = ember::contracts::runtime;
using ember::test::SyncEventSink;

/** 内存里的磁带：每帧 (左, 右)。 */
class FakeSource final : public hw::TapeSource {
 public:
  FakeSource(std::vector<std::pair<std::int16_t, std::int16_t>> frames, bool stereoTape) : frames_(std::move(frames)), stereo_(stereoTape) {}
  std::size_t read(std::int16_t* interleaved, std::size_t frames) override {
    std::size_t count = 0;
    while (count < frames && cursor_ < frames_.size()) {
      interleaved[count * 2] = frames_[cursor_].first;
      interleaved[count * 2 + 1] = frames_[cursor_].second;
      ++count;
      ++cursor_;
    }
    return count;
  }
  void rewind() override { cursor_ = 0; }
  [[nodiscard]] double durationSeconds() const override { return static_cast<double>(frames_.size()) / SAMPLE_RATE; }
  [[nodiscard]] bool stereo() const override { return stereo_; }

 private:
  std::vector<std::pair<std::int16_t, std::int16_t>> frames_;
  bool stereo_;
  std::size_t cursor_{0};
};

/** 磁带存取的替身：按路径找内存里的磁带；录音写完后同一路径变成可播放的磁带。 */
class FakeTapeStore final : public api::TapeStore {
 public:
  [[nodiscard]] std::unique_ptr<hw::TapeSource> open(const std::string& path) const override {
    const auto found = tapes.find(path);
    if (found == tapes.end()) throw ember::shared::CoreError(api::ERROR_FILE_NOT_FOUND, "找不到文件：" + path);
    if (failOpenFrom > 0 && ++opens >= failOpenFrom) throw ember::shared::CoreError(api::ERROR_TAPE_UNSUPPORTED, "无法解码：" + path);
    return std::make_unique<FakeSource>(found->second, true);
  }

  [[nodiscard]] std::unique_ptr<hw::TapeSink> create(const std::string& path) const override {
    if (path.find("readonly") != std::string::npos) throw ember::shared::CoreError(api::ERROR_SAVE_FAILED, "无法创建录音文件：" + path);
    struct Sink final : hw::TapeSink {
      Sink(const FakeTapeStore* owner, std::string target) : store(owner), path(std::move(target)) {}
      void write(const std::int16_t* data, std::size_t count) override { samples.insert(samples.end(), data, data + count); }
      void finish() override {
        std::vector<std::pair<std::int16_t, std::int16_t>> frames;
        for (const std::int16_t value : samples) frames.emplace_back(0, value);
        if (frames.empty()) frames.emplace_back(0, 0);
        store->tapes[path] = frames;
      }
      const FakeTapeStore* store;
      std::string path;
      std::vector<std::int16_t> samples;
    };
    return std::make_unique<Sink>(this, path);
  }

  [[nodiscard]] std::unique_ptr<hw::TapeSink> overwrite(const std::string& path, double startSeconds) const override {
    if (path.size() < 4 || path.substr(path.size() - 4) != ".wav") throw ember::shared::CoreError(api::ERROR_TAPE_NOT_WRITABLE, "不是 WAV：" + path);
    const auto found = tapes.find(path);
    if (found == tapes.end()) throw ember::shared::CoreError(api::ERROR_FILE_NOT_FOUND, "找不到文件：" + path);
    overwriteStarts.push_back(startSeconds);
    // 录完后把原磁带的前后内容和新录的内容（每个采样占 3 帧）接起来
    struct Sink final : hw::TapeSink {
      Sink(const FakeTapeStore* owner, std::string target, std::size_t start) : store(owner), path(std::move(target)), startFrame(start) {}
      void write(const std::int16_t* data, std::size_t count) override { samples.insert(samples.end(), data, data + count); }
      void finish() override {
        const auto& old = store->tapes[path];
        std::vector<std::pair<std::int16_t, std::int16_t>> frames(old.begin(), old.begin() + static_cast<std::ptrdiff_t>(std::min(startFrame, old.size())));
        for (const std::int16_t value : samples) frames.insert(frames.end(), 3, {value, value});
        for (std::size_t index = startFrame + samples.size() * 3; index < old.size(); ++index) frames.push_back(old[index]);
        store->tapes[path] = frames;
      }
      const FakeTapeStore* store;
      std::string path;
      std::size_t startFrame;
      std::vector<std::int16_t> samples;
    };
    return std::make_unique<Sink>(this, path, static_cast<std::size_t>(std::llround(startSeconds * hw::TapeSource::SAMPLE_RATE)));
  }

  mutable std::map<std::string, std::vector<std::pair<std::int16_t, std::int16_t>>> tapes;
  /** 每次 overwrite 的录音开始位置（秒）。 */
  mutable std::vector<double> overwriteStarts;
  /** 从第几次打开起都失败；0 表示不失败。 */
  int failOpenFrom{0};
  mutable int opens{0};
};

/** 真实的介质服务加运行服务。 */
struct TapeRig {
  explicit TapeRig(std::shared_ptr<api::TapeStore> tapeStore = std::make_shared<FakeTapeStore>()) : store(std::move(tapeStore)) {
    registry = std::make_shared<ember::infra::mappers::PluginRegistry>(ember::test::fcRegistryOptions());
    registry->installPackage(ember::test::officialPluginPackage("nrom"));
    ember::logic::runtime::SimulationRuntimeOptions options;
    auto catalog = std::make_shared<ember::bootstrap::MachineCatalog>();
    deck = catalog->tapeControl();
    options.machines = catalog;
    options.mappers = registry;
    options.frames = frames;
    options.events = events;
    options.errors = errors;
    options.framesPerSecond = 0;
    runtime = std::make_shared<ember::logic::runtime::SimulationRuntime>(std::move(options));
    manager = std::make_unique<ember::logic::media::MediaManager>(nullptr, store, runtime, events, nullptr, deck, std::vector<std::shared_ptr<hw::FloppyDrive>>{});
  }

  /** 装一台断电的 fc（空转卡带）。 */
  void insertFc() {
    ember::test::Assembler main(0x8000);
    main.jmpSelf();
    const ember::logic::cartridges::InesCartridgeLoader loader(registry);
    auto plan = loader.plan(ember::test::makeInesFile(main.bytes()));
    rt::CartridgeInsertion insertion;
    insertion.image = std::move(plan.image);
    insertion.match = std::move(plan.match);
    insertion.source = "idle.nes";
    insertion.powerOn = false;
    insertion.resume = false;
    runtime->insertCartridge(std::move(insertion));
  }

  /** 装一台断电的 sb2000（ROM 是合成的），它没有磁带数据线。 */
  void bootSb2000() {
    rt::MachineBoot boot;
    boot.machine = "sb2000";
    boot.rom.assign(512 * 1024, 0xEA);
    boot.powerOn = false;
    runtime->bootMachine(std::move(boot));
  }

  /** 给装好的主机通电（暂停状态，不走帧），磁带机随之接上主机。 */
  void powerOn() { runtime->powerOn(false); }

  std::shared_ptr<hw::TapeControl> deck;
  std::shared_ptr<api::TapeStore> store;
  std::shared_ptr<ember::infra::mappers::PluginRegistry> registry;
  std::shared_ptr<ember::test::FrameCapture> frames = std::make_shared<ember::test::FrameCapture>();
  std::shared_ptr<SyncEventSink> events = std::make_shared<SyncEventSink>();
  std::shared_ptr<ember::test::RecordingErrorLog> errors = std::make_shared<ember::test::RecordingErrorLog>();
  std::shared_ptr<ember::logic::runtime::SimulationRuntime> runtime;
  std::unique_ptr<ember::logic::media::MediaManager> manager;
};

/** 调用并取出错误码；没有抛出时返回空串。 */
template <typename Function>
std::string errorCodeOf(Function&& call) {
  try {
    call();
  } catch (const ember::shared::CoreError& error) {
    return error.code();
  }
  return {};
}

std::vector<std::pair<std::int16_t, std::int16_t>> silence(std::size_t frames) { return std::vector<std::pair<std::int16_t, std::int16_t>>(frames, {0, 0}); }

api::MediaTapeLoadParams loadParams(const std::string& path) {
  api::MediaTapeLoadParams params;
  params.path = path;
  return params;
}

api::MediaTapeRecordParams recordParams(const std::string& path) {
  api::MediaTapeRecordParams params;
  params.path = path;
  return params;
}

/** 没有文件路径的录音：在磁带机里已有的磁带上从当前位置开始录。 */
api::MediaTapeRecordParams recordOverParams() { return api::MediaTapeRecordParams{}; }

TEST(MediaTape, WithoutAMachineTheDeckStillTakesAndReleasesTapesButCannotPlayOrRecord) {
  TapeRig rig;
  dynamic_cast<FakeTapeStore&>(*rig.store).tapes["a.wav"] = silence(100);
  auto status = rig.manager->tapeStatus().status;
  EXPECT_FALSE(status.connected);
  EXPECT_FALSE(status.loaded);
  EXPECT_EQ(status.mode, "stopped");
  EXPECT_EQ(errorCodeOf([&] { rig.manager->tapePlay(); }), "media.noTape");
  status = rig.manager->tapeLoad(loadParams("a.wav")).status;
  EXPECT_TRUE(status.loaded) << "没有主机也能载入磁带";
  EXPECT_FALSE(status.connected);
  EXPECT_EQ(errorCodeOf([&] { rig.manager->tapePlay(); }), "media.tapeNotConnected");
  EXPECT_EQ(errorCodeOf([&] { rig.manager->tapeRecord(recordParams("r.wav")); }), "media.tapeNotConnected");
  EXPECT_EQ(errorCodeOf([&] { rig.manager->tapeStop(); }), "");
  status = rig.manager->tapeEject().status;
  EXPECT_FALSE(status.loaded);
}

TEST(MediaTape, AnyMachineConnectsTheDeckOnceItIsPoweredOnWhateverItsTapePort) {
  TapeRig rig;
  dynamic_cast<FakeTapeStore&>(*rig.store).tapes["a.wav"] = silence(100);
  rig.bootSb2000();
  EXPECT_FALSE(rig.manager->tapeStatus().status.connected) << "没通电时没有接上";
  EXPECT_TRUE(rig.manager->tapeLoad(loadParams("a.wav")).status.loaded);
  EXPECT_EQ(errorCodeOf([&] { rig.manager->tapePlay(); }), "media.tapeNotConnected");
  rig.powerOn();
  EXPECT_TRUE(rig.manager->tapeStatus().status.connected) << "学习机没有数据线，通电后磁带机照样接上";
  EXPECT_EQ(rig.manager->tapePlay().status.mode, "playing");
  rig.manager->tapeStop();
  EXPECT_EQ(rig.manager->tapeRecord(recordParams("r.wav")).status.mode, "recording");
}

TEST(MediaTape, AnUnpoweredFcDoesNotConnectTheDeckUntilItIsPoweredOn) {
  TapeRig rig;
  dynamic_cast<FakeTapeStore&>(*rig.store).tapes["a.wav"] = silence(100);
  rig.insertFc();
  EXPECT_FALSE(rig.manager->tapeStatus().status.connected);
  rig.manager->tapeLoad(loadParams("a.wav"));
  EXPECT_EQ(errorCodeOf([&] { rig.manager->tapePlay(); }), "media.tapeNotConnected") << "没通电不能播放";
  rig.powerOn();
  EXPECT_TRUE(rig.manager->tapeStatus().status.connected);
  EXPECT_EQ(rig.manager->tapePlay().status.mode, "playing");
}

TEST(MediaTape, LoadPlayStopAndEjectReportTheStatusAndPublishChanges) {
  TapeRig rig;
  auto& store = dynamic_cast<FakeTapeStore&>(*rig.store);
  store.tapes["D:/tapes/lesson.m4a"] = silence(96000);
  rig.insertFc();
  rig.powerOn();

  auto status = rig.manager->tapeStatus().status;
  EXPECT_TRUE(status.connected);
  EXPECT_FALSE(status.loaded);

  status = rig.manager->tapeLoad(loadParams("D:/tapes/lesson.m4a")).status;
  EXPECT_TRUE(status.loaded);
  ASSERT_TRUE(status.fileName.has_value());
  EXPECT_EQ(*status.fileName, "lesson.m4a") << "只给文件名，不含路径";
  EXPECT_EQ(status.mode, "stopped");
  EXPECT_NEAR(status.duration, 1.0, 1e-9);
  EXPECT_TRUE(status.stereo);

  status = rig.manager->tapePlay().status;
  EXPECT_EQ(status.mode, "playing");
  EXPECT_EQ(errorCodeOf([&] { rig.manager->tapeLoad(loadParams("D:/tapes/lesson.m4a")); }), "media.tapeBusy") << "播放时不能载入";
  EXPECT_EQ(errorCodeOf([&] { rig.manager->tapeRecord(recordParams("r.wav")); }), "media.tapeBusy") << "播放时不能录音";

  status = rig.manager->tapeStop().status;
  EXPECT_EQ(status.mode, "stopped");
  EXPECT_TRUE(status.loaded);

  status = rig.manager->tapeEject().status;
  EXPECT_FALSE(status.loaded);
  EXPECT_FALSE(status.fileName.has_value());

  const auto changes = rig.events->all("media.changed");
  ASSERT_GE(changes.size(), 4U);
  for (const auto& change : changes) {
    EXPECT_EQ(change["device"], "tape");
    EXPECT_EQ(change["drive"], 0);
  }
}

TEST(MediaTape, EjectingWhilePlayingStopsTheTapeAndTakesItOut) {
  TapeRig rig;
  dynamic_cast<FakeTapeStore&>(*rig.store).tapes["a.wav"] = silence(96000);
  rig.insertFc();
  rig.powerOn();
  rig.manager->tapeLoad(loadParams("a.wav"));
  ASSERT_EQ(rig.manager->tapePlay().status.mode, "playing");
  const auto status = rig.manager->tapeEject().status;
  EXPECT_FALSE(status.loaded);
  EXPECT_EQ(status.mode, "stopped");
  EXPECT_EQ(errorCodeOf([&] { rig.manager->tapePlay(); }), "media.noTape");
}

TEST(MediaTape, LoadingAMissingFileOrPlayingAnEmptyDeckFails) {
  TapeRig rig;
  rig.insertFc();
  rig.powerOn();
  EXPECT_EQ(errorCodeOf([&] { rig.manager->tapeLoad(loadParams("nope.wav")); }), "media.fileNotFound");
  EXPECT_EQ(errorCodeOf([&] { rig.manager->tapePlay(); }), "media.noTape");
  EXPECT_FALSE(rig.manager->tapeStatus().status.loaded);
}

TEST(MediaTape, StoppingARecordingMakesTheRecordedFileTheTape) {
  TapeRig rig;
  rig.insertFc();
  rig.powerOn();
  auto status = rig.manager->tapeRecord(recordParams("D:/tapes/out.wav")).status;
  EXPECT_EQ(status.mode, "recording");
  EXPECT_TRUE(status.loaded);
  EXPECT_EQ(errorCodeOf([&] { rig.manager->tapePlay(); }), "") << "录音时播放没有效果，不是错误";
  EXPECT_EQ(rig.manager->tapeStatus().status.mode, "recording");
  status = rig.manager->tapeStop().status;
  EXPECT_EQ(status.mode, "stopped");
  EXPECT_TRUE(status.loaded) << "录好的文件自动成为磁带";
  ASSERT_TRUE(status.fileName.has_value());
  EXPECT_EQ(*status.fileName, "out.wav");
  status = rig.manager->tapePlay().status;
  EXPECT_EQ(status.mode, "playing");
}

TEST(MediaTape, RecordingOnALoadedTapeStartsAtThePositionAndStopsWhereItWasStopped) {
  TapeRig rig;
  auto& store = dynamic_cast<FakeTapeStore&>(*rig.store);
  store.tapes["a.wav"] = silence(96000 * 2);
  rig.insertFc();
  rig.powerOn();
  rig.manager->tapeLoad(loadParams("a.wav"));
  rig.manager->tapeSeek(api::MediaTapeSeekParams{0.5});
  auto status = rig.manager->tapeRecord(recordOverParams()).status;
  EXPECT_EQ(status.mode, "recording");
  EXPECT_NEAR(status.position, 0.5, 1e-6) << "录音的位置从磁带当前的位置算起";
  ASSERT_EQ(store.overwriteStarts.size(), 1U);
  EXPECT_NEAR(store.overwriteStarts[0], 0.5, 1e-6);
  status = rig.manager->tapeStop().status;
  EXPECT_EQ(status.mode, "stopped");
  EXPECT_TRUE(status.loaded) << "改写后的磁带自动载入";
  EXPECT_EQ(*status.fileName, "a.wav");
  EXPECT_NEAR(status.position, 0.5, 1e-6) << "位置停在录音停止的位置";
  EXPECT_NEAR(status.duration, 2.0, 1e-6);
}

TEST(MediaTape, RecordingWithoutAFileNeedsATapeAndAWavTapeAndAPoweredMachine) {
  TapeRig rig;
  auto& store = dynamic_cast<FakeTapeStore&>(*rig.store);
  store.tapes["a.m4a"] = silence(100);
  store.tapes["b.wav"] = silence(100);
  rig.insertFc();
  rig.powerOn();
  EXPECT_EQ(errorCodeOf([&] { rig.manager->tapeRecord(recordOverParams()); }), "media.noTape");
  rig.manager->tapeLoad(loadParams("a.m4a"));
  EXPECT_EQ(errorCodeOf([&] { rig.manager->tapeRecord(recordOverParams()); }), "media.tapeNotWritable") << "不是 WAV 不能改写";
  auto status = rig.manager->tapeStatus().status;
  EXPECT_TRUE(status.loaded) << "出错后磁带还在机器里";
  EXPECT_EQ(status.mode, "stopped");
  rig.runtime->powerOff();
  rig.manager->tapeLoad(loadParams("b.wav"));
  EXPECT_EQ(errorCodeOf([&] { rig.manager->tapeRecord(recordOverParams()); }), "media.tapeNotConnected");
  EXPECT_TRUE(rig.manager->tapeStatus().status.loaded);
}

TEST(MediaTape, ARecordingFileThatCannotBeCreatedLeavesTheDeckAlone) {
  TapeRig rig;
  rig.insertFc();
  rig.powerOn();
  EXPECT_EQ(errorCodeOf([&] { rig.manager->tapeRecord(recordParams("D:/readonly/out.wav")); }), "media.saveFailed");
  EXPECT_EQ(rig.manager->tapeStatus().status.mode, "stopped");
  EXPECT_FALSE(rig.manager->tapeStatus().status.loaded);
}

TEST(MediaTape, ReplacingTheCartridgeKeepsTheTapeInTheDeck) {
  TapeRig rig;
  dynamic_cast<FakeTapeStore&>(*rig.store).tapes["a.wav"] = silence(100);
  rig.insertFc();
  rig.manager->tapeLoad(loadParams("a.wav"));
  ASSERT_TRUE(rig.manager->tapeStatus().status.loaded);
  rig.insertFc();
  EXPECT_TRUE(rig.manager->tapeStatus().status.loaded) << "磁带机不属于主机，换卡带不会把磁带带走";
  rig.runtime->ejectCartridge();
  EXPECT_TRUE(rig.manager->tapeStatus().status.loaded) << "取出卡带也一样";
}

TEST(MediaTape, PoweringOffStopsThePlaybackAndKeepsTheTape) {
  TapeRig rig;
  dynamic_cast<FakeTapeStore&>(*rig.store).tapes["a.wav"] = silence(96000);
  rig.insertFc();
  rig.powerOn();
  rig.manager->tapeLoad(loadParams("a.wav"));
  ASSERT_EQ(rig.manager->tapePlay().status.mode, "playing");
  rig.runtime->powerOff();
  const auto status = rig.manager->tapeStatus().status;
  EXPECT_EQ(status.mode, "stopped");
  EXPECT_TRUE(status.loaded);
  EXPECT_FALSE(status.connected);
}

/** 追加一段数据线（右声道）固定为 right 的帧；左声道固定为满幅的一半，用来证明分析只看右声道。 */
void appendRun(std::vector<std::pair<std::int16_t, std::int16_t>>& frames, double seconds, std::int16_t right) {
  frames.insert(frames.end(), static_cast<std::size_t>(std::llround(seconds * hw::TapeSource::SAMPLE_RATE)), {16384, right});
}

/** 等后台分析结束（不再是分析中），最多等 10 秒。 */
api::TapeTimeline waitForTimeline(TapeRig& rig) {
  const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(10);
  for (;;) {
    auto timeline = rig.manager->tapeTimeline().timeline;
    if (timeline.state != "analyzing" || std::chrono::steady_clock::now() > deadline) return timeline;
    std::this_thread::sleep_for(std::chrono::milliseconds(5));
  }
}

TEST(MediaTape, SeekingMovesThePositionWhileStoppedOrPlayingAndNeedsATape) {
  TapeRig rig;
  dynamic_cast<FakeTapeStore&>(*rig.store).tapes["a.wav"] = silence(96000 * 2);
  EXPECT_EQ(errorCodeOf([&] { rig.manager->tapeSeek(api::MediaTapeSeekParams{1.0}); }), "media.noTape");
  rig.manager->tapeLoad(loadParams("a.wav"));
  auto status = rig.manager->tapeSeek(api::MediaTapeSeekParams{1.25}).status;
  EXPECT_NEAR(status.position, 1.25, 1e-6) << "没有主机、停止时也能定位";
  EXPECT_EQ(status.mode, "stopped");
  rig.insertFc();
  rig.powerOn();
  ASSERT_EQ(rig.manager->tapePlay().status.mode, "playing");
  status = rig.manager->tapeSeek(api::MediaTapeSeekParams{0.5}).status;
  EXPECT_EQ(status.mode, "playing") << "播放时定位后继续播放";
  EXPECT_NEAR(status.position, 0.5, 0.01);
  const std::size_t changes = rig.events->all("media.changed").size();
  rig.manager->tapeSeek(api::MediaTapeSeekParams{0.75});
  EXPECT_EQ(rig.events->all("media.changed").size(), changes + 1) << "定位后发 media.changed";
}

TEST(MediaTape, SeekingIsRefusedWhileRecording) {
  TapeRig rig;
  rig.insertFc();
  rig.powerOn();
  rig.manager->tapeRecord(recordParams("D:/tapes/out.wav"));
  EXPECT_EQ(errorCodeOf([&] { rig.manager->tapeSeek(api::MediaTapeSeekParams{1.0}); }), "media.tapeBusy");
  EXPECT_EQ(rig.manager->tapeStatus().status.mode, "recording");
}

TEST(MediaTape, TheTimelineFindsTheBlanksOnTheDataLineAndKeepsShortGaps) {
  TapeRig rig;
  std::vector<std::pair<std::int16_t, std::int16_t>> frames;
  appendRun(frames, 1.0, 20000);
  appendRun(frames, 0.3, 0);  // 不到 0.5 秒的间隔不算空白
  appendRun(frames, 1.0, -20000);
  appendRun(frames, 1.0, 100);  // 低于满幅 3% 算静音
  appendRun(frames, 0.5, 20000);
  dynamic_cast<FakeTapeStore&>(*rig.store).tapes["data.wav"] = frames;
  const auto loaded = rig.manager->tapeLoad(loadParams("data.wav")).status;
  const auto timeline = waitForTimeline(rig);
  ASSERT_EQ(timeline.state, "ready");
  EXPECT_EQ(timeline.serial, loaded.serial);
  EXPECT_NEAR(timeline.duration, 3.8, 1e-6);
  EXPECT_NEAR(timeline.step, 0.1, 1e-9);
  EXPECT_EQ(timeline.progress, 1.0);
  ASSERT_EQ(timeline.peaks.size(), 38U);
  EXPECT_EQ(timeline.peaks[0], 62) << "20000 占满幅的 61%，向上取整";
  EXPECT_EQ(timeline.peaks[11], 0) << "1.1 秒处是静音";
  EXPECT_EQ(timeline.peaks[25], 1) << "幅度 100 虽然低于静音线，但有声音时至少是 1";
  ASSERT_EQ(timeline.blanks.size(), 1U);
  EXPECT_NEAR(timeline.blanks[0].start, 2.3, 1e-6);
  EXPECT_NEAR(timeline.blanks[0].end, 3.3, 1e-6);
  const auto events = rig.events->all("media.tapeAnalyzed");
  ASSERT_EQ(events.size(), 1U);
  EXPECT_EQ(events[0]["serial"], loaded.serial);
}

TEST(MediaTape, TheLeadingAndTrailingSilenceCountAsBlanks) {
  TapeRig rig;
  std::vector<std::pair<std::int16_t, std::int16_t>> frames;
  appendRun(frames, 0.6, 0);
  appendRun(frames, 0.4, 20000);
  appendRun(frames, 0.75, 0);
  dynamic_cast<FakeTapeStore&>(*rig.store).tapes["a.wav"] = frames;
  rig.manager->tapeLoad(loadParams("a.wav"));
  const auto timeline = waitForTimeline(rig);
  ASSERT_EQ(timeline.state, "ready");
  ASSERT_EQ(timeline.blanks.size(), 2U);
  EXPECT_NEAR(timeline.blanks[0].start, 0.0, 1e-6);
  EXPECT_NEAR(timeline.blanks[0].end, 0.6, 1e-6);
  EXPECT_NEAR(timeline.blanks[1].start, 1.0, 1e-6);
  EXPECT_NEAR(timeline.blanks[1].end, 1.75, 1e-6);
}

TEST(MediaTape, ChangingTheTapeRestartsTheTimelineAndEjectingClearsIt) {
  TapeRig rig;
  auto& store = dynamic_cast<FakeTapeStore&>(*rig.store);
  store.tapes["a.wav"] = silence(96000);
  store.tapes["b.wav"] = silence(96000 * 2);
  EXPECT_EQ(rig.manager->tapeTimeline().timeline.state, "none") << "没有磁带时没有时间轴";
  const auto first = rig.manager->tapeLoad(loadParams("a.wav")).status;
  ASSERT_EQ(waitForTimeline(rig).state, "ready");
  const auto second = rig.manager->tapeLoad(loadParams("b.wav")).status;
  EXPECT_GT(second.serial, first.serial) << "换磁带序号加 1";
  auto timeline = waitForTimeline(rig);
  EXPECT_EQ(timeline.serial, second.serial);
  EXPECT_NEAR(timeline.duration, 2.0, 1e-6);
  const auto ejected = rig.manager->tapeEject().status;
  EXPECT_GT(ejected.serial, second.serial) << "取出序号也加 1";
  timeline = rig.manager->tapeTimeline().timeline;
  EXPECT_EQ(timeline.state, "none");
  EXPECT_EQ(timeline.serial, ejected.serial);
  EXPECT_TRUE(timeline.peaks.empty());
  EXPECT_EQ(rig.manager->tapeEject().status.serial, ejected.serial) << "没有磁带时取出没有效果，序号不变";
}

TEST(MediaTape, TheRecordedFileGetsAnalyzedAfterTheRecordingStops) {
  TapeRig rig;
  rig.insertFc();
  rig.powerOn();
  const auto recording = rig.manager->tapeRecord(recordParams("D:/tapes/out.wav")).status;
  EXPECT_EQ(rig.manager->tapeTimeline().timeline.state, "none") << "录音时没有时间轴";
  const auto stopped = rig.manager->tapeStop().status;
  EXPECT_GT(stopped.serial, recording.serial);
  const auto timeline = waitForTimeline(rig);
  EXPECT_EQ(timeline.state, "ready");
  EXPECT_EQ(timeline.serial, stopped.serial);
}

TEST(MediaTape, AFileThatCannotBeReadAgainMakesTheTimelineFail) {
  auto store = std::make_shared<FakeTapeStore>();
  store->tapes["a.wav"] = silence(96000);
  store->failOpenFrom = 2;
  TapeRig failing(store);
  failing.manager->tapeLoad(loadParams("a.wav"));
  const auto timeline = waitForTimeline(failing);
  EXPECT_EQ(timeline.state, "failed");
  EXPECT_TRUE(timeline.peaks.empty());
  EXPECT_TRUE(failing.manager->tapeStatus().status.loaded) << "分析失败不影响磁带本身";
}

// ---------------------------------------------------------------- 真实的 FFmpeg 库端到端

/** 在临时目录里写一个 8 kHz 单声道 16 位的 WAV：前半秒低电平，后半秒高电平。 */
std::string writeSquareWav(const ember::test::TempDir& dir) {
  constexpr std::uint32_t RATE = 8000;
  constexpr std::uint32_t COUNT = RATE;
  std::vector<std::uint8_t> bytes(44 + COUNT * 2);
  const auto put32 = [&](std::size_t at, std::uint32_t value) {
    for (unsigned index = 0; index < 4; ++index) bytes[at + index] = static_cast<std::uint8_t>(value >> (index * 8));
  };
  std::copy_n("RIFF", 4, bytes.begin());
  put32(4, 36 + COUNT * 2);
  std::copy_n("WAVEfmt ", 8, bytes.begin() + 8);
  put32(16, 16);
  bytes[20] = 1;
  bytes[22] = 1;
  put32(24, RATE);
  put32(28, RATE * 2);
  bytes[32] = 2;
  bytes[34] = 16;
  std::copy_n("data", 4, bytes.begin() + 36);
  put32(40, COUNT * 2);
  for (std::uint32_t index = 0; index < COUNT; ++index) {
    const std::int16_t value = index < COUNT / 2 ? std::int16_t{-12000} : std::int16_t{12000};
    bytes[44 + index * 2] = static_cast<std::uint8_t>(static_cast<std::uint16_t>(value) & 0xFF);
    bytes[45 + index * 2] = static_cast<std::uint8_t>(static_cast<std::uint16_t>(value) >> 8);
  }
  const auto text = dir.write("square.wav", bytes).u8string();
  return std::string(text.begin(), text.end());
}

TEST(MediaTape, RealDecoderDecodesAWavIntoThe96KilohertzStreamTheDeckPlays) {
  ember::test::TempDir dir;
  const std::string file = writeSquareWav(dir);
  ember::infra::media::DiskTapeStore store(nullptr);
  const auto source = store.open(file);
  EXPECT_NEAR(source->durationSeconds(), 1.0, 0.05);
  EXPECT_FALSE(source->stereo()) << "单声道磁带整路是数据";

  std::vector<std::int16_t> frames(96000 * 2 + 8000);
  std::size_t total = 0;
  for (;;) {
    const std::size_t count = source->read(frames.data() + total * 2, 4096);
    if (count == 0) break;
    total += count;
  }
  EXPECT_NEAR(static_cast<double>(total), 96000.0, 2000.0) << "8 kHz 的 1 秒被重采样成 96 kHz";
  EXPECT_LT(frames[1000 * 2 + 1], -8000) << "前半秒低电平（右声道）";
  EXPECT_GT(frames[(total - 1000) * 2 + 1], 8000) << "后半秒高电平";
  EXPECT_EQ(frames[1000 * 2], frames[1000 * 2 + 1]) << "单声道复制到两个声道";

  source->rewind();
  EXPECT_GT(source->read(frames.data(), 100), 0U) << "倒带后重新从头读";
}

TEST(MediaTape, RealStoreReportsAMissingFileAndAFileWithoutSound) {
  ember::test::TempDir dir;
  const ember::infra::media::DiskTapeStore store(nullptr);
  EXPECT_EQ(errorCodeOf([&] { static_cast<void>(store.open((dir.path() / "none.wav").string())); }), "media.fileNotFound");
  const auto notSound = dir.write("notes.txt", std::vector<std::uint8_t>{'h', 'e', 'l', 'l', 'o'}).u8string();
  EXPECT_EQ(errorCodeOf([&] { static_cast<void>(store.open(std::string(notSound.begin(), notSound.end()))); }), "media.tapeUnsupported");
}

TEST(MediaTape, RealWavSinkWritesAValidWavFile) {
  ember::test::TempDir dir;
  const auto path = dir.path() / "out.wav";
  const auto text = path.u8string();
  const std::string utf8(text.begin(), text.end());
  {
    ember::infra::media::DiskTapeStore store(nullptr);
    auto sink = store.create(utf8);
    const std::vector<std::int16_t> samples = {1, -2, 300};
    sink->write(samples.data(), samples.size());
    sink->finish();
  }
  std::ifstream stream(path, std::ios::binary);
  std::vector<char> bytes((std::istreambuf_iterator<char>(stream)), std::istreambuf_iterator<char>());
  ASSERT_EQ(bytes.size(), 44U + 6U);
  EXPECT_EQ(std::string(bytes.begin(), bytes.begin() + 4), "RIFF");
  EXPECT_EQ(std::string(bytes.begin() + 8, bytes.begin() + 16), "WAVEfmt ");
  EXPECT_EQ(static_cast<std::uint8_t>(bytes[4]), 36 + 6);
  EXPECT_EQ(static_cast<std::uint8_t>(bytes[40]), 6);
  EXPECT_EQ(static_cast<std::uint8_t>(bytes[24]), 32000 & 0xFF);
  EXPECT_EQ(static_cast<std::uint8_t>(bytes[25]), (32000 >> 8) & 0xFF);
  EXPECT_EQ(static_cast<std::uint8_t>(bytes[44]), 1);
  EXPECT_EQ(static_cast<std::uint8_t>(bytes[47]), 0xFF) << "-2 的高字节";
}

}  // namespace
