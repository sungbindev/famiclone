/*
 * ------------------------------------------------------------------------
 * 名称：media-manager-test.cpp
 * 说明：介质服务的测试：没有主机也能操作软驱、软盘镜像的插入（含文件错误）、取出、保存（写回文件并清除已修改标志）、状态、换主机不丢盘片、各种故障注入，以及 media.changed 事件。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-16
 * 备注：用真实的运行服务、sb2000 主机（合成的 BIOS，断电状态）与磁盘上的镜像文件；文件写在临时目录里。
 * ------------------------------------------------------------------------
 */

#include <gtest/gtest.h>

#include <atomic>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <regex>
#include <span>
#include <string>
#include <thread>
#include <vector>

#include "../support/runtime-test-support.hpp"
#include "bootstrap/machines/machine-catalog.hpp"
#include "contracts/media/media-error.hpp"
#include "infra/media/disk-floppy-image-store.hpp"
#include "infra/media/disk-tape-store.hpp"
#include "infra/media/png-printer-page-store.hpp"
#include "logic/media/media-manager.hpp"
#include "logic/media/printer-service.hpp"
#include "logic/runtime/simulation-runtime.hpp"
#include "platform/files/file-watcher.hpp"
#include "shared/errors/core-error.hpp"

namespace {

namespace api = ember::contracts::media;
using ember::test::SyncEventSink;

std::uint8_t patternAt(std::size_t offset) { return static_cast<std::uint8_t>(offset * 5 + 1); }

std::vector<std::uint8_t> makeImage(std::size_t size = 1440 * 1024) {
  std::vector<std::uint8_t> image(size);
  for (std::size_t index = 0; index < size; ++index) image[index] = patternAt(index);
  return image;
}

/** 真实的介质服务加运行服务；还没有装主机。打印页保存到临时目录里的 printed。镜像的自动写回与文件监视默认关闭，专门的用例用 autoSave 打开。 */
struct Rig {
  explicit Rig(std::shared_ptr<api::FloppyImageStore> floppyStore = std::make_shared<ember::infra::media::DiskFloppyImageStore>(), bool autoSave = false,
               std::shared_ptr<ember::platform::FileWatchFactory> watchFactory = nullptr, std::chrono::milliseconds retryInterval = std::chrono::seconds(5)) {
    registry = std::make_shared<ember::infra::mappers::PluginRegistry>(ember::test::fcRegistryOptions());
    printer = std::make_shared<ember::logic::media::PrinterService>(
        std::make_shared<ember::infra::media::PngPrinterPageStore>(), [this] { return printDirectory.empty() ? utf8(dir.path() / "printed") : printDirectory; }, events, errors,
        [] { return std::chrono::system_clock::time_point(std::chrono::seconds(1700000000)); });
    ember::logic::runtime::SimulationRuntimeOptions options;
    auto catalog = std::make_shared<ember::bootstrap::MachineCatalog>(printer);
    options.machines = catalog;
    options.mappers = registry;
    options.frames = frames;
    options.events = events;
    options.errors = errors;
    options.framesPerSecond = 0;
    runtime = std::make_shared<ember::logic::runtime::SimulationRuntime>(std::move(options));
    manager = std::make_unique<ember::logic::media::MediaManager>(std::move(floppyStore), std::make_shared<ember::infra::media::DiskTapeStore>(errors),
                                                                  runtime, events, printer, catalog->tapeControl(),
                                                                  std::vector<std::shared_ptr<ember::contracts::hardware::FloppyDrive>>{catalog->floppyDrive()}, errors, std::move(watchFactory), autoSave, retryInterval);
  }

  static std::string utf8(const std::filesystem::path& path) {
    const auto text = path.u8string();
    return std::string(text.begin(), text.end());
  }

  /** 把字节串写进主机的 CPU 总线地址（经调试接口，写到真实的总线上）。 */
  void writeBus(std::uint16_t address, const std::vector<std::uint8_t>& bytes) {
    runtime->useMachine([&](ember::contracts::hardware::Machine& machine) {
      for (const std::uint8_t byte : bytes) machine.debugger()->writeMemory("cpu", address, std::vector<std::uint8_t>{byte});
    });
  }

  /** 装一台断电的 sb2000（BIOS 是合成的）。 */
  void bootMachine() {
    ember::contracts::runtime::MachineBoot boot;
    boot.machine = "sb2000";
    boot.bios.assign(512 * 1024, 0xEA);
    boot.powerOn = false;
    runtime->bootMachine(std::move(boot));
  }

  [[nodiscard]] std::string write(const std::string& name, const std::vector<std::uint8_t>& bytes) const {
    const auto text = dir.write(name, bytes).u8string();
    return std::string(text.begin(), text.end());
  }

  static api::MediaFloppyInsertParams insertParams(const std::string& path, bool writeProtected = false) {
    api::MediaFloppyInsertParams params;
    params.drive = 0;
    params.path = path;
    params.writeProtected = writeProtected;
    return params;
  }

  static api::MediaFloppyFaultParams faultParams(const std::string& fault, std::optional<bool> enabled = std::nullopt, std::optional<std::int64_t> sector = std::nullopt) {
    api::MediaFloppyFaultParams params;
    params.drive = 0;
    params.fault = fault;
    params.enabled = enabled;
    params.sector = sector;
    return params;
  }

  ember::test::TempDir dir;
  /** 打印页的保存目录；为空时用临时目录里的 printed。 */
  std::string printDirectory;
  std::shared_ptr<ember::infra::mappers::PluginRegistry> registry;
  std::shared_ptr<ember::test::FrameCapture> frames = std::make_shared<ember::test::FrameCapture>();
  std::shared_ptr<SyncEventSink> events = std::make_shared<SyncEventSink>();
  std::shared_ptr<ember::test::RecordingErrorLog> errors = std::make_shared<ember::test::RecordingErrorLog>();
  std::shared_ptr<ember::logic::media::PrinterService> printer;
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

/** 读的是真文件、写会失败的镜像存取；failing 为 false 时与真实存取相同。 */
class WriteFailingStore final : public api::FloppyImageStore {
 public:
  [[nodiscard]] std::vector<std::uint8_t> read(const std::string& path) const override { return inner_.read(path); }
  void write(const std::string& path, const std::vector<std::uint8_t>& bytes) override {
    if (failing) throw ember::shared::CoreError(api::ERROR_SAVE_FAILED, "文件被占用");
    inner_.write(path, bytes);
  }
  void writeAt(const std::string& path, std::size_t offset, std::span<const std::uint8_t> bytes) override {
    if (failing) throw ember::shared::CoreError(api::ERROR_SAVE_FAILED, "文件被占用");
    inner_.writeAt(path, offset, bytes);
  }
  std::atomic<bool> failing{true};

 private:
  ember::infra::media::DiskFloppyImageStore inner_;
};

/** 假的文件监视工厂：不跟系统打交道，测试用 trigger 代替系统的“文件变了”通知。 */
class FakeWatchFactory final : public ember::platform::FileWatchFactory {
 public:
  [[nodiscard]] std::unique_ptr<ember::platform::FileWatcher> watch(const std::string& path, std::function<void()> onChanged) override {
    std::lock_guard lock(mutex_);
    callbacks_[path] = std::move(onChanged);
    return std::make_unique<Watch>(*this, path);
  }

  /** 模拟系统报告这个文件变了；没有监视时什么都不发生。 */
  void trigger(const std::string& path) {
    std::function<void()> callback;
    {
      std::lock_guard lock(mutex_);
      const auto found = callbacks_.find(path);
      if (found != callbacks_.end()) callback = found->second;
    }
    if (callback) callback();
  }

  /** 正在监视的文件数。 */
  [[nodiscard]] std::size_t live() {
    std::lock_guard lock(mutex_);
    return callbacks_.size();
  }

 private:
  class Watch final : public ember::platform::FileWatcher {
   public:
    Watch(FakeWatchFactory& owner, std::string path) : owner_(owner), path_(std::move(path)) {}
    ~Watch() override {
      std::lock_guard lock(owner_.mutex_);
      owner_.callbacks_.erase(path_);
    }

   private:
    FakeWatchFactory& owner_;
    std::string path_;
  };

  std::mutex mutex_;
  std::map<std::string, std::function<void()>> callbacks_;
};

/** 把文件里的一个字节改成别的值（模拟别的程序编辑镜像）。 */
void patchFile(const std::string& path, std::size_t offset, std::uint8_t value) {
  std::fstream stream(std::filesystem::path(std::u8string(path.begin(), path.end())), std::ios::binary | std::ios::in | std::ios::out);
  stream.seekp(static_cast<std::streamoff>(offset));
  stream.put(static_cast<char>(value));
}

/** 读镜像里的一个字节（经驱动器，需要先 bootMachine）。 */
std::uint8_t readDisk(Rig& rig, std::size_t offset) {
  std::uint8_t value = 0;
  rig.runtime->useMachine([&](ember::contracts::hardware::Machine& machine) { value = machine.floppyDrive(0)->readByte(offset); });
  return value;
}

/** 读全部字节。 */
std::vector<std::uint8_t> readFile(const std::string& path) {
  std::ifstream stream(std::filesystem::path(std::u8string(path.begin(), path.end())), std::ios::binary);
  return std::vector<std::uint8_t>((std::istreambuf_iterator<char>(stream)), std::istreambuf_iterator<char>());
}

/** 轮询条件，最多等 5 秒。 */
template <typename Condition>
bool waitUntil(Condition&& condition) {
  for (int attempt = 0; attempt < 500; ++attempt) {
    if (condition()) return true;
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
  }
  return condition();
}

/** 学习机写盘：直接改驱动器里的镜像。 */
void writeDisk(Rig& rig, std::size_t offset, std::uint8_t value) {
  rig.runtime->useMachine([&](ember::contracts::hardware::Machine& machine) { machine.floppyDrive(0)->writeByte(offset, value); });
}

TEST(MediaManager, WithoutAMachineTheDriveStillTakesAndReleasesDisks) {
  Rig rig;
  ASSERT_EQ(rig.manager->floppyStatus().drives.size(), 1U) << "软驱 A 一直存在，与主机无关";
  EXPECT_FALSE(rig.manager->floppyStatus().drives[0].inserted);
  const auto path = rig.write("a.img", makeImage());
  const auto inserted = rig.manager->floppyInsert(Rig::insertParams(path));
  EXPECT_TRUE(inserted.status.inserted);
  api::MediaFloppyEjectParams eject;
  eject.drive = 0;
  EXPECT_FALSE(rig.manager->floppyEject(eject).status.inserted);
}

TEST(MediaManager, TheDiskStaysInTheDriveWhenTheMachineIsReplacedOrRemoved) {
  Rig rig;
  const auto path = rig.write("sys.img", makeImage());
  rig.manager->floppyInsert(Rig::insertParams(path));
  rig.bootMachine();
  bool seenByMachine = false;
  rig.runtime->useMachine([&](ember::contracts::hardware::Machine& machine) { seenByMachine = machine.floppyDrive(0)->inserted(); });
  EXPECT_TRUE(seenByMachine) << "先插盘再开机，学习机看得到盘片";
  rig.bootMachine();
  EXPECT_TRUE(rig.manager->floppyStatus().drives[0].inserted) << "换主机不丢盘片";
  rig.runtime->ejectCartridge();
  EXPECT_TRUE(rig.manager->floppyStatus().drives[0].inserted) << "拆除主机也不丢";
  EXPECT_FALSE(rig.manager->floppyStatus().drives[0].motor);
}

TEST(MediaManager, InsertsAnImageEvenWhilePoweredOff) {
  Rig rig;
  rig.bootMachine();
  const auto path = rig.write("sys.img", makeImage());
  const auto result = rig.manager->floppyInsert(Rig::insertParams(path));
  EXPECT_TRUE(result.status.inserted);
  EXPECT_EQ(result.status.fileName, "sys.img");
  EXPECT_EQ(result.status.size, 1440 * 1024);
  EXPECT_FALSE(result.status.writeProtected);
  EXPECT_FALSE(result.status.modified);
  EXPECT_EQ(rig.events->count("media.changed"), 1);
  EXPECT_EQ(rig.events->last("media.changed")["device"], "floppy");
  EXPECT_EQ(rig.events->last("media.changed")["drive"], 0);
  const auto status = rig.manager->floppyStatus();
  ASSERT_EQ(status.drives.size(), 1U);
  EXPECT_TRUE(status.drives[0].inserted);
  EXPECT_EQ(rig.runtime->status().power, "off") << "插盘与电源无关";
}

TEST(MediaManager, InsertCanWriteProtectTheDisk) {
  Rig rig;
  rig.bootMachine();
  const auto result = rig.manager->floppyInsert(Rig::insertParams(rig.write("sys.img", makeImage()), true));
  EXPECT_TRUE(result.status.writeProtected);
}

TEST(MediaManager, InsertRejectsBadFilesAndUnknownDrives) {
  Rig rig;
  rig.bootMachine();
  EXPECT_EQ(errorCodeOf([&] { rig.manager->floppyInsert(Rig::insertParams((rig.dir.path() / "none.img").string())); }), api::ERROR_FILE_NOT_FOUND);
  EXPECT_EQ(errorCodeOf([&] { rig.manager->floppyInsert(Rig::insertParams(rig.write("empty.img", {}))); }), api::ERROR_IMAGE_INVALID);
  auto params = Rig::insertParams(rig.write("ok.img", makeImage()));
  params.drive = 1;
  EXPECT_EQ(errorCodeOf([&] { rig.manager->floppyInsert(params); }), api::ERROR_NO_DRIVE);
  EXPECT_FALSE(rig.manager->floppyStatus().drives[0].inserted) << "失败时驱动器不变";
}

TEST(MediaManager, EjectDropsTheImage) {
  Rig rig;
  rig.bootMachine();
  rig.manager->floppyInsert(Rig::insertParams(rig.write("sys.img", makeImage())));
  api::MediaFloppyEjectParams eject;
  eject.drive = 0;
  const auto result = rig.manager->floppyEject(eject);
  EXPECT_FALSE(result.status.inserted);
  EXPECT_FALSE(result.status.fileName.has_value());
  EXPECT_EQ(result.status.size, 0);
  EXPECT_EQ(rig.events->count("media.changed"), 2);
  EXPECT_NO_THROW(rig.manager->floppyEject(eject)) << "没有盘片时没有效果";
}

TEST(MediaManager, SaveWritesTheImageBackAndClearsTheModifiedFlag) {
  Rig rig;
  rig.bootMachine();
  const auto path = rig.write("sys.img", makeImage());
  rig.manager->floppyInsert(Rig::insertParams(path));
  // 学习机写盘：直接改驱动器里的镜像。
  rig.runtime->useMachine([](ember::contracts::hardware::Machine& machine) { machine.floppyDrive(0)->writeByte(1000, 0xEE); });
  EXPECT_TRUE(rig.manager->floppyStatus().drives[0].modified);
  api::MediaFloppySaveParams save;
  save.drive = 0;
  const auto result = rig.manager->floppySave(save);
  EXPECT_FALSE(result.status.modified);
  std::ifstream stream(std::filesystem::path(std::u8string(path.begin(), path.end())), std::ios::binary);
  std::vector<char> bytes((std::istreambuf_iterator<char>(stream)), std::istreambuf_iterator<char>());
  ASSERT_EQ(bytes.size(), 1440U * 1024U);
  EXPECT_EQ(static_cast<std::uint8_t>(bytes[1000]), 0xEE);
  EXPECT_EQ(static_cast<std::uint8_t>(bytes[1001]), patternAt(1001));
}

TEST(MediaManager, WriteProtectCanBeSwitchedOnAndOff) {
  Rig rig;
  rig.bootMachine();
  rig.manager->floppyInsert(Rig::insertParams(rig.write("sys.img", makeImage())));
  api::MediaFloppyWriteProtectParams params;
  params.drive = 0;
  params.writeProtected = true;
  EXPECT_TRUE(rig.manager->floppyWriteProtect(params).status.writeProtected);
  EXPECT_EQ(rig.events->count("media.changed"), 2);
  params.writeProtected = false;
  EXPECT_FALSE(rig.manager->floppyWriteProtect(params).status.writeProtected);
  params.drive = 1;
  EXPECT_EQ(errorCodeOf([&] { rig.manager->floppyWriteProtect(params); }), api::ERROR_NO_DRIVE);
}

TEST(MediaManager, MotorChangesPublishAMediaChangedEvent) {
  Rig rig;
  rig.bootMachine();
  const auto before = rig.events->count("media.changed");
  rig.manager->motorChanged(0, true);
  EXPECT_EQ(rig.events->count("media.changed"), before + 1);
}

TEST(MediaManager, SaveWithoutADiskFails) {
  Rig rig;
  rig.bootMachine();
  api::MediaFloppySaveParams save;
  save.drive = 0;
  EXPECT_EQ(errorCodeOf([&] { rig.manager->floppySave(save); }), api::ERROR_NO_DISK);
}

TEST(MediaManager, FaultsCanBeSwitchedOnAndOffAndCleared) {
  Rig rig;
  rig.bootMachine();
  rig.manager->floppyInsert(Rig::insertParams(rig.write("sys.img", makeImage())));

  auto status = rig.manager->floppyFault(Rig::faultParams("ejected")).status;
  EXPECT_FALSE(status.inserted);
  EXPECT_TRUE(status.faults.ejected);
  EXPECT_EQ(status.fileName, "sys.img") << "取盘保留镜像";
  status = rig.manager->floppyFault(Rig::faultParams("ejected", false)).status;
  EXPECT_TRUE(status.inserted);
  EXPECT_FALSE(status.faults.ejected);

  EXPECT_TRUE(rig.manager->floppyFault(Rig::faultParams("writeProtected")).status.writeProtected);
  EXPECT_TRUE(rig.manager->floppyFault(Rig::faultParams("noIndex")).status.faults.noIndex);
  EXPECT_TRUE(rig.manager->floppyFault(Rig::faultParams("seekFail")).status.faults.seekFail);
  EXPECT_TRUE(rig.manager->floppyFault(Rig::faultParams("unresponsive")).status.faults.unresponsive);
  status = rig.manager->floppyFault(Rig::faultParams("badSector", true, 7)).status;
  ASSERT_EQ(status.faults.badSectors.size(), 1U);
  EXPECT_EQ(status.faults.badSectors[0], 7);
  status = rig.manager->floppyFault(Rig::faultParams("badSector", true, 7)).status;
  EXPECT_EQ(status.faults.badSectors.size(), 1U) << "同一个扇区不重复登记";
  status = rig.manager->floppyFault(Rig::faultParams("badSector", false, 7)).status;
  EXPECT_TRUE(status.faults.badSectors.empty());

  rig.manager->floppyFault(Rig::faultParams("ejected"));
  rig.manager->floppyFault(Rig::faultParams("badSector", true, 3));
  status = rig.manager->floppyFault(Rig::faultParams("clear")).status;
  EXPECT_TRUE(status.inserted);
  EXPECT_FALSE(status.writeProtected);
  EXPECT_FALSE(status.faults.noIndex);
  EXPECT_FALSE(status.faults.seekFail);
  EXPECT_FALSE(status.faults.unresponsive);
  EXPECT_TRUE(status.faults.badSectors.empty());
}

TEST(MediaManager, BadSectorFaultNeedsASectorNumber) {
  Rig rig;
  rig.bootMachine();
  EXPECT_EQ(errorCodeOf([&] { rig.manager->floppyFault(Rig::faultParams("badSector")); }), api::ERROR_SECTOR_REQUIRED);
}

TEST(MediaManagerAutoSave, GuestWritesGoIntoTheFileByThemselvesWithoutAnyone) {
  Rig rig(std::make_shared<ember::infra::media::DiskFloppyImageStore>(), true);
  rig.bootMachine();
  const auto path = rig.write("sys.img", makeImage());
  rig.manager->floppyInsert(Rig::insertParams(path));
  writeDisk(rig, 1000, 0xEE);
  EXPECT_TRUE(waitUntil([&] { return !rig.manager->floppyStatus().drives[0].modified; })) << "不点保存，改动一出现就写进文件并清除“已修改”";
  const auto bytes = readFile(path);
  ASSERT_EQ(bytes.size(), 1440U * 1024U);
  EXPECT_EQ(bytes[1000], 0xEE);
  EXPECT_EQ(bytes[1001], patternAt(1001)) << "只改写过的那一段，其余字节不变";
  EXPECT_EQ(bytes[999], patternAt(999));
  EXPECT_EQ(rig.events->count("media.floppySaveFailed"), 0);
}

TEST(MediaManagerAutoSave, EveryWriteIsEventuallyInTheFileEvenWhenTheyKeepComing) {
  Rig rig(std::make_shared<ember::infra::media::DiskFloppyImageStore>(), true);
  rig.bootMachine();
  const auto path = rig.write("sys.img", makeImage());
  rig.manager->floppyInsert(Rig::insertParams(path));
  for (std::size_t index = 0; index < 50; ++index) writeDisk(rig, 3000 + index * 7000, 0xA7);
  EXPECT_TRUE(waitUntil([&] { return !rig.manager->floppyStatus().drives[0].modified; }));
  const auto bytes = readFile(path);
  for (std::size_t index = 0; index < 50; ++index) EXPECT_EQ(bytes[3000 + index * 7000], 0xA7) << index;
}

TEST(MediaManagerAutoSync, ChangesMadeToTheFileByAnotherProgramAreMergedIntoTheImage) {
  auto watch = std::make_shared<FakeWatchFactory>();
  Rig rig(std::make_shared<ember::infra::media::DiskFloppyImageStore>(), true, watch);
  rig.bootMachine();
  const auto path = rig.write("sys.img", makeImage());
  rig.manager->floppyInsert(Rig::insertParams(path));
  patchFile(path, 5000, 0x42);
  watch->trigger(path);
  EXPECT_TRUE(waitUntil([&] { return readDisk(rig, 5000) == 0x42; })) << "文件一变就同步进内存里的镜像";
  EXPECT_FALSE(rig.manager->floppyStatus().drives[0].modified) << "外部改动不算“已修改”，不会再写回";
  EXPECT_EQ(readDisk(rig, 5001), patternAt(5001));
}

TEST(MediaManagerAutoSync, OurOwnWritesComingBackAsFileChangesChangeNothing) {
  auto watch = std::make_shared<FakeWatchFactory>();
  Rig rig(std::make_shared<ember::infra::media::DiskFloppyImageStore>(), true, watch);
  rig.bootMachine();
  const auto path = rig.write("sys.img", makeImage());
  rig.manager->floppyInsert(Rig::insertParams(path));
  writeDisk(rig, 1000, 0xEE);
  ASSERT_TRUE(waitUntil([&] { return !rig.manager->floppyStatus().drives[0].modified; }));
  watch->trigger(path);
  std::this_thread::sleep_for(std::chrono::milliseconds(200));
  EXPECT_EQ(readDisk(rig, 1000), 0xEE);
  EXPECT_FALSE(rig.manager->floppyStatus().drives[0].modified);
  EXPECT_EQ(readFile(path)[1000], 0xEE);
}

TEST(MediaManagerAutoSync, BytesTheGuestJustWroteSurviveAnExternalChangeAndOthersAreTaken) {
  auto store = std::make_shared<WriteFailingStore>();
  auto watch = std::make_shared<FakeWatchFactory>();
  Rig rig(store, true, watch, std::chrono::milliseconds(30));
  rig.bootMachine();
  const auto path = rig.write("sys.img", makeImage());
  rig.manager->floppyInsert(Rig::insertParams(path));
  writeDisk(rig, 1000, 0xEE);
  ASSERT_TRUE(waitUntil([&] { return rig.events->count("media.floppySaveFailed") == 1; })) << "写不进文件，学习机的改动留在内存里";
  patchFile(path, 1000, 0x11);
  patchFile(path, 2000, 0x22);
  watch->trigger(path);
  EXPECT_TRUE(waitUntil([&] { return readDisk(rig, 2000) == 0x22; }));
  EXPECT_EQ(readDisk(rig, 1000), 0xEE) << "已修改区间里的字节不被文件里的旧内容盖掉";
  EXPECT_TRUE(rig.manager->floppyStatus().drives[0].modified);
  store->failing = false;
  EXPECT_TRUE(waitUntil([&] { return !rig.manager->floppyStatus().drives[0].modified; })) << "之后写回成功，学习机的字节落到文件里";
  EXPECT_EQ(readFile(path)[1000], 0xEE);
  EXPECT_EQ(readFile(path)[2000], 0x22);
}

TEST(MediaManagerAutoSync, AFileWhoseSizeDiffersFromTheDiskIsIgnored) {
  auto watch = std::make_shared<FakeWatchFactory>();
  Rig rig(std::make_shared<ember::infra::media::DiskFloppyImageStore>(), true, watch);
  rig.bootMachine();
  const auto path = rig.write("sys.img", makeImage());
  rig.manager->floppyInsert(Rig::insertParams(path));
  static_cast<void>(rig.write("sys.img", makeImage(720 * 1024)));
  patchFile(path, 100, 0x42);
  watch->trigger(path);
  std::this_thread::sleep_for(std::chrono::milliseconds(200));
  EXPECT_EQ(readDisk(rig, 100), patternAt(100));
  EXPECT_EQ(rig.manager->floppyStatus().drives[0].size, 1440 * 1024);
}

TEST(MediaManagerAutoSync, TheFileIsWatchedOnlyWhileItsDiskIsInTheDrive) {
  auto watch = std::make_shared<FakeWatchFactory>();
  Rig rig(std::make_shared<ember::infra::media::DiskFloppyImageStore>(), true, watch);
  EXPECT_EQ(watch->live(), 0U);
  const auto first = rig.write("a.img", makeImage());
  const auto second = rig.write("b.img", makeImage());
  rig.manager->floppyInsert(Rig::insertParams(first));
  EXPECT_EQ(watch->live(), 1U);
  rig.manager->floppyInsert(Rig::insertParams(second));
  EXPECT_EQ(watch->live(), 1U) << "换盘后只监视新的文件";
  api::MediaFloppyEjectParams eject;
  eject.drive = 0;
  rig.manager->floppyEject(eject);
  EXPECT_EQ(watch->live(), 0U);
}

TEST(MediaManagerAutoSave, EjectWritesPendingChangesBackFirst) {
  Rig rig;
  rig.bootMachine();
  const auto path = rig.write("sys.img", makeImage());
  rig.manager->floppyInsert(Rig::insertParams(path));
  writeDisk(rig, 1000, 0xEE);
  api::MediaFloppyEjectParams eject;
  eject.drive = 0;
  EXPECT_FALSE(rig.manager->floppyEject(eject).status.inserted);
  EXPECT_EQ(readFile(path)[1000], 0xEE);
}

TEST(MediaManagerAutoSave, InsertingAnotherImageWritesTheOldOneBackFirst) {
  Rig rig;
  rig.bootMachine();
  const auto first = rig.write("a.img", makeImage());
  const auto second = rig.write("b.img", makeImage());
  rig.manager->floppyInsert(Rig::insertParams(first));
  writeDisk(rig, 1000, 0xEE);
  const auto result = rig.manager->floppyInsert(Rig::insertParams(second));
  EXPECT_EQ(result.status.fileName, "b.img");
  EXPECT_FALSE(result.status.modified);
  EXPECT_EQ(readFile(first)[1000], 0xEE);
  EXPECT_NE(readFile(second)[1000], 0xEE);
}

TEST(MediaManagerAutoSave, ShutdownWritesPendingChangesBack) {
  Rig rig;
  rig.bootMachine();
  const auto path = rig.write("sys.img", makeImage());
  rig.manager->floppyInsert(Rig::insertParams(path));
  writeDisk(rig, 1000, 0xEE);
  rig.manager->shutdown();
  EXPECT_EQ(readFile(path)[1000], 0xEE);
  EXPECT_FALSE(rig.manager->floppyStatus().drives[0].modified);
}

TEST(MediaManagerAutoSave, AFailedWriteBackKeepsTheDiskAndTheChangesAndTellsTheUserOnce) {
  auto store = std::make_shared<WriteFailingStore>();
  Rig rig(store, true, nullptr, std::chrono::milliseconds(30));
  rig.bootMachine();
  const auto path = rig.write("sys.img", makeImage());
  rig.manager->floppyInsert(Rig::insertParams(path));
  writeDisk(rig, 1000, 0xEE);
  ASSERT_TRUE(waitUntil([&] { return rig.events->count("media.floppySaveFailed") == 1; }));
  EXPECT_EQ(rig.events->last("media.floppySaveFailed")["drive"], 0);
  EXPECT_FALSE(rig.events->last("media.floppySaveFailed")["message"].get<std::string>().empty());
  std::this_thread::sleep_for(std::chrono::milliseconds(200));
  EXPECT_EQ(rig.events->count("media.floppySaveFailed"), 1) << "成功之前只通知一次，之后的重试不再通知";
  EXPECT_TRUE(rig.manager->floppyStatus().drives[0].modified);
  api::MediaFloppyEjectParams eject;
  eject.drive = 0;
  EXPECT_EQ(errorCodeOf([&] { rig.manager->floppyEject(eject); }), api::ERROR_SAVE_FAILED) << "写回不了就不取出，修改不丢";
  EXPECT_TRUE(rig.manager->floppyStatus().drives[0].inserted);
  EXPECT_TRUE(rig.manager->floppyStatus().drives[0].modified);
  store->failing = false;
  EXPECT_TRUE(waitUntil([&] { return !rig.manager->floppyStatus().drives[0].modified; })) << "文件可写之后自动重试成功";
  EXPECT_EQ(readFile(path)[1000], 0xEE);
  EXPECT_FALSE(rig.manager->floppyEject(eject).status.inserted);
}

/** 经 CPU 总线的 $4025 给打印机发一段字节：ESC @、一列 9 针位图。 */
void printOneDot(Rig& rig) { rig.writeBus(0x4025, {0x1B, '@', 0x1B, '*', 0, 1, 0, 0x80}); }

TEST(MediaPrinter, StatusIsIdleWithoutAMachineOrWhilePoweredOff) {
  Rig rig;
  auto status = rig.manager->printerStatus().status;
  EXPECT_EQ(status.state, "idle");
  EXPECT_FALSE(status.hasContent);
  EXPECT_EQ(status.pages, 0);
  EXPECT_FALSE(status.lastFile.has_value());
  rig.bootMachine();
  EXPECT_EQ(rig.manager->printerStatus().status.state, "idle");
  EXPECT_EQ(errorCodeOf([&] { rig.manager->printerFlush(); }), api::ERROR_PRINTER_EMPTY);
}

TEST(MediaPrinter, FlushWithoutAMachineFails) {
  Rig rig;
  EXPECT_EQ(errorCodeOf([&] { rig.manager->printerFlush(); }), api::ERROR_NO_MACHINE);
}

TEST(MediaPrinter, PrintingThenFlushingSavesAPngAndReportsIt) {
  Rig rig;
  rig.bootMachine();
  rig.runtime->powerOn(false);
  EXPECT_EQ(rig.manager->printerStatus().status.state, "idle");
  printOneDot(rig);
  auto status = rig.manager->printerStatus().status;
  EXPECT_EQ(status.state, "printing");
  EXPECT_TRUE(status.hasContent);
  EXPECT_TRUE(rig.events->last("media.printed").is_null()) << "没有换页之前不输出";

  status = rig.manager->printerFlush().status;
  EXPECT_EQ(status.state, "done");
  EXPECT_FALSE(status.hasContent);
  EXPECT_EQ(status.pages, 1);
  ASSERT_TRUE(status.lastFile.has_value());
  EXPECT_TRUE(std::regex_match(*status.lastFile, std::regex(R"(printer-\d{8}-\d{6}\.png)"))) << "文件名用输出时的本机日期时间：" << *status.lastFile;
  EXPECT_EQ(rig.events->count("media.printed"), 1);
  EXPECT_EQ(rig.events->last("media.printed")["fileName"], *status.lastFile);
  EXPECT_EQ(rig.events->last("media.printed")["page"], 1);

  std::ifstream stream(rig.dir.path() / "printed" / std::filesystem::path(std::u8string(status.lastFile->begin(), status.lastFile->end())), std::ios::binary);
  std::vector<std::uint8_t> png((std::istreambuf_iterator<char>(stream)), std::istreambuf_iterator<char>());
  ASSERT_GT(png.size(), 33U);
  const std::uint8_t signature[8] = {0x89, 'P', 'N', 'G', 0x0D, 0x0A, 0x1A, 0x0A};
  EXPECT_TRUE(std::equal(signature, signature + 8, png.begin()));
  const auto bigEndian = [&](std::size_t at) { return (png[at] << 24) | (png[at + 1] << 16) | (png[at + 2] << 8) | png[at + 3]; };
  EXPECT_EQ(bigEndian(16), 1660) << "IHDR 的宽";
  EXPECT_EQ(bigEndian(20), 2340) << "IHDR 的高";
  EXPECT_EQ(png[24], 1) << "位深 1";
  EXPECT_EQ(png[25], 0) << "灰度";
}

TEST(MediaPrinter, PagesWithinTheSameSecondGetASerialSuffix) {
  Rig rig;
  rig.bootMachine();
  rig.runtime->powerOn(false);
  printOneDot(rig);
  rig.manager->printerFlush();
  printOneDot(rig);
  const auto status = rig.manager->printerFlush().status;
  EXPECT_EQ(status.pages, 2);
  EXPECT_TRUE(std::regex_match(*status.lastFile, std::regex(R"(printer-\d{8}-\d{6}-2\.png)"))) << *status.lastFile;
}

TEST(MediaPrinter, ASavingFailureDiscardsThePageAndReportsIt) {
  Rig rig;
  // 保存目录是一个已经存在的普通文件：无法在里面建文件。
  rig.printDirectory = rig.write("not-a-directory", {1, 2, 3});
  rig.bootMachine();
  rig.runtime->powerOn(false);
  printOneDot(rig);
  const auto status = rig.manager->printerFlush().status;
  EXPECT_EQ(status.pages, 0) << "这一页丢弃了";
  EXPECT_EQ(rig.events->count("media.printed"), 0);
  EXPECT_EQ(rig.events->count("media.printFailed"), 1);
  EXPECT_FALSE(rig.events->last("media.printFailed")["message"].get<std::string>().empty());
}

TEST(MediaPrinter, PoweringOffOutputsTheOpenPage) {
  Rig rig;
  rig.bootMachine();
  rig.runtime->powerOn(false);
  printOneDot(rig);
  rig.runtime->powerOff();
  rig.runtime->powerOn(false);
  EXPECT_EQ(rig.manager->printerStatus().status.pages, 1) << "硬复位时打印机把已经画的页输出";
}

}  // namespace
