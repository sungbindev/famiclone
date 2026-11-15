/*
 * ------------------------------------------------------------------------
 * 名称：mapper-chip.hpp
 * 说明：由插件创建的卡带（mapper 芯片加存储芯片）：按清单创建存储区、窗口与译码表，响应卡槽上的 CPU 侧与 PPU 侧访问，在寄存器访问时回调脚本，或按清单的声明由宿主直接处理。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-14
 * 备注：窗口译码在宿主侧完成：每个卡槽地址预先对应到窗口或寄存器，访问时只查表，不为每次访问调用脚本；
 *       寄存器优先于窗口（落在寄存器范围内的读写交给脚本或声明，窗口不再处理）；窗口越过存储区末尾时按存储区容量回绕（镜像）；
 *       声明式的寄存器、查表与磁带计时在宿主里直接执行，不进脚本；查表的表项由脚本的 tables 函数计算一次后按索引缓存；
 *       脚本出错时记下错误并停止调用脚本，之后的访问按开放总线处理（见 mapper-cartridge.hpp）。
 * ------------------------------------------------------------------------
 */

#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <set>
#include <string>
#include <string_view>
#include <vector>

#include "contracts/hardware/bus-device.hpp"
#include "contracts/hardware/button-input.hpp"
#include "contracts/hardware/cartridge.hpp"
#include "contracts/hardware/irq-output.hpp"
#include "contracts/hardware/tape-deck.hpp"
#include "contracts/mappers/cartridge-image.hpp"
#include "contracts/mappers/mapper-cartridge.hpp"
#include "infra/mappers/loaded-plugin.hpp"
#include "infra/mappers/script-runtime.hpp"
#include "infra/mappers/storage-block.hpp"

namespace ember::infra::mappers {

/** 一个插件创建的卡带。 */
class MapperChip final : public contracts::mappers::MapperCartridge, private ScriptBindings {
 public:
  /**
   * @param plugin 插件，卡带持有它的引用计数。
   * @param image 卡带映像，数据被复制。
   * @param irq 主机卡槽上的 IRQ 引脚，必须活到卡带销毁。
   * @param pads 主机上的手柄口，脚本经 ctx.pad 设置按键，必须活到卡带销毁。
   * @param limits 脚本的执行限制。
   * @throws shared::CoreError mappers.invalidPackage：脚本无法加载或没有导出清单声明的函数；mappers.limitExceeded：加载脚本时超限。
   */
  MapperChip(std::shared_ptr<const LoadedPlugin> plugin, const contracts::mappers::CartridgeImage& image, contracts::hardware::IrqOutput& irq, contracts::hardware::PadSink& pads,
             ScriptLimits limits = {});
  ~MapperChip() override;

  MapperChip(const MapperChip&) = delete;
  MapperChip& operator=(const MapperChip&) = delete;

  // Cartridge
  contracts::hardware::BusDevice& cpuSide() override { return cpuSide_; }
  contracts::hardware::BusDevice& ppuSide() override { return ppuSide_; }
  [[nodiscard]] contracts::hardware::CiramControl ciramControl(std::uint16_t ppuAddress) const override;
  void reset(bool power) override;
  void input(const contracts::hardware::InputEvent& event) override;

  // MapperCartridge
  [[nodiscard]] const std::string& pluginId() const override { return plugin_->manifest.id; }
  [[nodiscard]] std::vector<contracts::mappers::StorageSummary> storageList() const override;
  [[nodiscard]] const contracts::hardware::StorageRegion* storage(std::string_view id) const override;
  bool pokeStorage(std::string_view id, std::size_t offset, std::uint8_t value) override;
  [[nodiscard]] std::optional<std::uint32_t> state(std::string_view name) const override;
  [[nodiscard]] std::vector<std::string> stateNames() const override;
  [[nodiscard]] std::optional<contracts::mappers::ScriptFailure> failure() const override { return failure_; }
  [[nodiscard]] std::vector<std::string> scriptLog() const override { return log_; }
  [[nodiscard]] contracts::mappers::AddressInfo describeAddress(bool ppu, std::uint16_t address) const override;

 private:
  /** 窗口：地址范围映射到存储区里的一段，起点由脚本经 setBank、setOffset 改变。 */
  struct Window {
    std::uint16_t first{0};
    std::uint32_t size{0};
    StorageBlock* storage{nullptr};
    std::size_t offset{0};
    bool mapped{true};
    bool writable{false};
    InitialKind initialKind{InitialKind::First};
    std::uint32_t initialBank{0};
  };

  /** 译码表的一项：这个地址由哪个窗口响应，是否处理寄存器（脚本或宿主的声明）。 */
  struct Slot {
    std::int16_t window{-1};
    /** 寄存器声明的下标，对应 registers_。 */
    std::int16_t reg{-1};
    bool readRegister{false};
    bool writeRegister{false};
  };

  /** 查表索引的一个来源（已把状态名解析成下标）。 */
  struct TableSource {
    IndexFrom from{IndexFrom::State};
    std::size_t state{0};
    unsigned shift{0};
    unsigned bits{0};
  };

  /** 查表：索引来源、表项位宽，以及按索引缓存的脚本计算结果。 */
  struct Table {
    std::string id;
    std::vector<TableSource> sources;
    std::uint32_t valueMask{0};
    std::vector<std::uint32_t> values;
    std::vector<std::uint8_t> known;
  };

  /** 寄存器声明里的状态名、窗口标识、表标识解析成下标后的样子。 */
  struct ResolvedRegister {
    struct Bank {
      std::size_t window{0};
      unsigned shift{0};
      unsigned mask{0};
    };
    ReadKind readKind{ReadKind::None};
    std::uint8_t readValue{0};
    std::size_t readState{0};
    bool releaseIrq{false};
    bool writeScript{false};
    std::optional<std::size_t> writeState;
    std::vector<Bank> banks;
    std::optional<std::size_t> table;
    std::size_t into{0};
  };

  class Side final : public contracts::hardware::BusDevice {
   public:
    Side(MapperChip& owner, bool ppu) : owner_(owner), ppu_(ppu) {}
    contracts::hardware::BusRead read(std::uint16_t address) override;
    void write(std::uint16_t address, std::uint8_t value) override;
    contracts::hardware::BusRead peek(std::uint16_t address) override;

   private:
    MapperChip& owner_;
    bool ppu_;
  };

  // ScriptBindings
  void windowSetBank(std::size_t window, std::int64_t bank) override;
  void windowSetOffset(std::size_t window, std::int64_t offset) override;
  void windowUnmap(std::size_t window) override;
  void nametableSet(std::string_view mode) override;
  void irqAssert() override { irq_.set(true); }
  void irqRelease() override { irq_.set(false); }
  [[nodiscard]] std::size_t storageSize(std::size_t storage) const override { return storages_.at(storage)->size(); }
  std::uint8_t storageRead(std::size_t storage, std::int64_t offset) override;
  bool storageWrite(std::size_t storage, std::int64_t offset, std::uint8_t value) override;
  [[nodiscard]] std::uint32_t stateGet(std::size_t state) const override { return stateValues_.at(state); }
  void stateSet(std::size_t state, std::uint32_t value) override { stateValues_.at(state) = value & stateMasks_.at(state); }
  void log(std::string_view message) override;
  [[nodiscard]] bool keyDown(std::string_view code) const override { return heldKeys_.contains(std::string(code)); }
  void padSet(std::size_t port, std::string_view button, bool down) override;
  void padRelease(std::size_t port) override;
  [[nodiscard]] bool tapeLevel() override;
  void tapeSetComparator(std::int16_t low, std::int16_t high, bool invert) override;

  [[nodiscard]] contracts::hardware::BusRead readBus(bool ppu, std::uint16_t address);
  [[nodiscard]] contracts::hardware::BusRead peekBus(bool ppu, std::uint16_t address) const;
  void writeBus(bool ppu, std::uint16_t address, std::uint8_t value);
  /** 读寄存器：脚本回调，或按声明由宿主直接回答。 */
  [[nodiscard]] contracts::hardware::BusRead readRegister(bool ppu, std::uint16_t address, std::int16_t index);
  /** 写寄存器：脚本回调，或按声明由宿主直接执行（存状态、切 bank、查表）。 */
  void writeRegister(bool ppu, std::uint16_t address, std::uint8_t value, std::int16_t index);
  /** 查表：按索引来源拼出索引，命中缓存直接返回，否则调脚本计算一次并缓存；脚本失败时返回空。 */
  [[nodiscard]] std::optional<std::uint32_t> lookup(std::size_t table, std::uint16_t address, std::uint8_t value);
  /** 磁带数据线翻转：先做清单声明的边沿计时，再通知脚本。 */
  void onTapeEdge(bool level, std::uint32_t samples);
  /** 复位时执行清单声明的部分：写了 initial 的状态回到初值、边沿计时撤销 IRQ、设置比较器。 */
  void applyDeclaredReset();
  [[nodiscard]] contracts::hardware::BusRead readWindow(const Window& window, std::uint16_t address) const;
  void writeWindow(const Window& window, std::uint16_t address, std::uint8_t value) const;
  void applyInitial(Window& window) const;
  void powerCycle();

  /** 执行一次脚本回调；出错时记下错误、停止以后的回调并返回 false。 */
  template <typename Call>
  bool runScript(Call&& call);

  std::shared_ptr<const LoadedPlugin> plugin_;
  contracts::hardware::IrqOutput& irq_;
  contracts::hardware::PadSink& pads_;
  /** 主机上的磁带机数据线；没有磁带机时为空。脚本导出 onTapeEdge 时卡带在它上面挂了边沿通知，销毁时摘掉。 */
  contracts::hardware::TapeLine* tape_{nullptr};
  bool tapeListening_{false};
  /** 各手柄口当前按下的按键掩码，由脚本设置。 */
  std::vector<std::uint8_t> padMasks_;
  /** 现在按着的物理键（KeyboardEvent.code）。 */
  std::set<std::string> heldKeys_;
  std::vector<std::unique_ptr<StorageBlock>> storages_;
  std::vector<Window> windows_;
  std::vector<Slot> cpuSlots_;
  std::vector<Slot> ppuSlots_;
  std::vector<std::uint32_t> stateValues_;
  std::vector<std::uint32_t> stateMasks_;
  std::vector<ResolvedRegister> registers_;
  std::vector<Table> tables_;
  /** 清单声明的磁带边沿计时的两个状态下标；没有声明时 timerSamples_ 为空。 */
  std::optional<std::size_t> timerSamples_;
  std::size_t timerTicks_{0};
  NametableMode nametable_{NametableMode::Horizontal};
  NametableMode initialNametable_{NametableMode::Horizontal};
  /** 四屏时卡带自带的名称表显存。 */
  std::array<std::uint8_t, 4096> ownNametable_{};
  std::optional<contracts::mappers::ScriptFailure> failure_;
  std::vector<std::string> log_;
  Side cpuSide_;
  Side ppuSide_;
  /** 脚本运行时；声明在最后，最先销毁（它持有 *this 作为宿主对象的实现）。 */
  std::unique_ptr<ScriptRuntime> script_;
};

}  // namespace ember::infra::mappers
