/*
 * ------------------------------------------------------------------------
 * 名称：sb2000.hpp
 * 说明：主机型号 sb2000（小霸王 2000 学习机，PAL）的装配：标准 6502、6576 的 PPU 与 PSG 及全部功能模块、内部 RAM、外部 RAM、BIOS ROM、扩展 RAM、显存、键盘、鼠标、手柄口、并口。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-15
 * 备注：时钟：主时钟 26,601,712 Hz，CPU 15 分频（约 1.7734 MHz），PPU 5 分频（每个 CPU 周期 3 个点），每行 341 点、每帧 312 行（50 Hz）；
 *       CPU 分频以真机 hwtest 实测为准：定时器选 CPU 时钟、预分频 16、预置 $FF 时 120 帧下溢约 1040 次（16 分频只有 975 次）；
 *       CPU 总线的译码见 sb2000.cpp；整机机型没有卡带，Machine 契约要求的卡槽是一个空的、不挂载的卡槽；
 *       并口接一台虚拟针式打印机（有输出方时），打印机 6 秒没有数据就输出当前页；
 *       具体实现只在这里创建并连线，模块之间不互相引用。
 * ------------------------------------------------------------------------
 */

#pragma once

#include <array>
#include <cstdint>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#include "bootstrap/machines/cpu-debug-kit.hpp"
#include "bootstrap/machines/sb2000-board.hpp"
#include "contracts/hardware/dma-memory.hpp"
#include "contracts/hardware/machine.hpp"
#include "contracts/hardware/parallel-port.hpp"
#include "logic/hardware/bus/mapped-bus.hpp"
#include "logic/hardware/cartridge-slot/standard-cartridge-slot.hpp"
#include "logic/hardware/cpu-6502/cpu-6502.hpp"
#include "logic/hardware/dac-um6576/dac-um6576.hpp"
#include "logic/hardware/dma-um6576/dma-um6576.hpp"
#include "logic/hardware/expio-um6576/expio-um6576.hpp"
#include "logic/hardware/fdc-upd765/fdc-upd765.hpp"
#include "logic/hardware/floppy-drive/standard-floppy-drive.hpp"
#include "logic/hardware/irq-um6576/irq-um6576.hpp"
#include "logic/hardware/joypad/joypad.hpp"
#include "logic/hardware/keyboard-sb2000/keyboard-sb2000.hpp"
#include "logic/hardware/keyboard-um6576/keyboard-um6576.hpp"
#include "logic/hardware/lines/line-output.hpp"
#include "logic/hardware/lines/wired-or-line.hpp"
#include "logic/hardware/mouse-sb2000/mouse-sb2000.hpp"
#include "logic/hardware/mouse-um6576/mouse-um6576.hpp"
#include "logic/hardware/pad-um6576/pad-um6576.hpp"
#include "logic/hardware/paging-um6576/paging-um6576.hpp"
#include "logic/hardware/ppu-um6576/ppu-um6576.hpp"
#include "logic/hardware/printer-um6576/printer-um6576.hpp"
#include "logic/hardware/psg-um6576/psg-um6576.hpp"
#include "logic/hardware/printer-escp/printer-escp.hpp"
#include "logic/hardware/ram/ram-block.hpp"
#include "logic/hardware/rtc-um6576/rtc-um6576.hpp"
#include "logic/hardware/scheduler/master-scheduler.hpp"
#include "logic/hardware/sysinit-um6576/sysinit-um6576.hpp"
#include "logic/hardware/timer-um6576/timer-um6576.hpp"

namespace ember::bootstrap {

/** 小霸王 2000 学习机（PAL）。 */
class Sb2000Machine final : public contracts::hardware::Machine, private contracts::hardware::MachineDebugger, private contracts::hardware::DmaMemory {
 public:
  /** 主时钟频率（赫兹）。 */
  static constexpr double MASTER_CLOCK_HZ = 26'601'712.0;
  /** 每个 CPU 周期的主时钟周期数。 */
  static constexpr contracts::hardware::MasterTime CPU_TICKS_PER_CYCLE = 15;
  /** 每个 PPU 点的主时钟周期数。 */
  static constexpr contracts::hardware::MasterTime PPU_TICKS_PER_DOT = 5;
  /** 一毫秒的主时钟周期数（取整）。 */
  static constexpr contracts::hardware::MasterTime MILLISECOND_TICKS = 26'602;
  /** BIOS ROM 的容量（字节）。 */
  static constexpr std::size_t BIOS_SIZE = Sb2000ProgramMemory::ROM_SIZE;
  /** 软驱控制器状态机每一步的主时钟周期数（约 2.4 微秒；软驱自己的节拍，与 CPU 分频无关）。 */
  static constexpr contracts::hardware::MasterTime FDC_STEP_TICKS = 64;
  /** 软驱控制器轮询一个驱动器占的步数（约 220 微秒）。 */
  static constexpr unsigned FDC_POLL_STEPS = 91;
  /** 打印机没有数据多久之后输出当前页（6 秒，主时钟周期数）。 */
  static constexpr contracts::hardware::MasterTime PRINTER_TIMEOUT_TICKS = 6 * 26'601'712;

  /**
   * @param bios BIOS ROM 的内容，必须是 BIOS_SIZE 字节（不含 iNES 文件头）。
   * @param printerOutput 打印机输出的页交给它，必须活到主机销毁；为空时并口不接打印机。
   * @throws shared::CoreError hardware.invalidArgument：BIOS 大小不对。
   */
  explicit Sb2000Machine(std::vector<std::uint8_t> bios, contracts::hardware::PrinterOutput* printerOutput = nullptr);

  Sb2000Machine(const Sb2000Machine&) = delete;
  Sb2000Machine& operator=(const Sb2000Machine&) = delete;

  void powerOn() override;
  void powerOff() override;
  void softReset() override;
  [[nodiscard]] bool isPoweredOn() const override { return poweredOn_; }
  void run(contracts::hardware::MasterTime ticks) override;
  void runFrame() override;
  [[nodiscard]] contracts::hardware::FrameBuffer frame() const override { return ppu_.frame(); }
  [[nodiscard]] unsigned audioSampleRate() const override { return logic::hardware::PsgUm6576::SAMPLE_RATE; }
  void takeAudio(std::vector<float>& samples) override;
  void setButtons(unsigned port, std::uint8_t buttons) override;
  [[nodiscard]] unsigned padPortCount() const override { return static_cast<unsigned>(joypads_.size()); }
  void input(const contracts::hardware::InputEvent& event) override;
  [[nodiscard]] contracts::hardware::CpuRegisters cpuRegisters() const override { return cpu_.registers(); }
  [[nodiscard]] const contracts::hardware::Clock& clock() const override { return scheduler_; }
  contracts::hardware::CartridgeSlot& cartridgeSlot() override { return slot_; }
  contracts::hardware::IrqOutput& cartridgeIrq() override { return cartridgeIrq_; }
  contracts::hardware::MachineDebugger* debugger() override { return this; }
  [[nodiscard]] unsigned floppyDriveCount() const override { return 1; }
  contracts::hardware::FloppyDrive* floppyDrive(unsigned index) override { return index == 0 ? &floppy_ : nullptr; }
  contracts::hardware::PrinterControl* printer() override { return printer_.get(); }

  // 测试与调试用
  [[nodiscard]] logic::hardware::PpuUm6576& ppu() { return ppu_; }
  [[nodiscard]] const logic::hardware::PpuUm6576& ppu() const { return ppu_; }
  [[nodiscard]] logic::hardware::PsgUm6576& psg() { return psg_; }
  [[nodiscard]] logic::hardware::KeyboardSb2000& keyboard() { return keyboardDevice_; }
  [[nodiscard]] logic::hardware::MouseSb2000& mouse() { return mouseDevice_; }
  /** 2K 内部 RAM。 */
  [[nodiscard]] const contracts::hardware::StorageRegion& internalRam() const { return ram_; }
  /** 8K 外部 RAM（$6000-$7FFF）。 */
  [[nodiscard]] const contracts::hardware::StorageRegion& externalRam() const { return externalRam_; }
  /** 扩展 RAM（512KB）。 */
  [[nodiscard]] const std::vector<std::uint8_t>& extendedRam() const { return program_.extendedRam(); }
  /** 显存。 */
  [[nodiscard]] const Sb2000VideoMemory& video() const { return video_; }
  /** 不经 CPU 读 CPU 总线，读不改变状态。 */
  std::uint8_t peekCpuBus(std::uint16_t address) { return cpuBus_.peek(address); }

 private:
  // DmaMemory
  std::uint8_t readSource(bool extendedRam, std::uint32_t offset) override;
  void writeVideo(std::uint16_t address, std::uint8_t value) override { videoBus_.write(address, value); }
  void writeWork(std::uint16_t address, std::uint8_t value) override;

  // MachineDebugger
  [[nodiscard]] std::vector<contracts::hardware::MemorySpace> memorySpaces() const override;
  void readMemory(std::string_view space, std::uint32_t address, std::span<std::uint8_t> out) override;
  void writeMemory(std::string_view space, std::uint32_t address, std::span<const std::uint8_t> bytes) override;
  [[nodiscard]] contracts::hardware::AddressTarget resolve(std::string_view space, std::uint32_t address) const override;
  [[nodiscard]] std::vector<std::string> stateModules() const override;
  [[nodiscard]] std::vector<contracts::hardware::StateField> readState(std::string_view module) const override;
  void writeState(std::string_view module, std::span<const contracts::hardware::StateField> fields) override;
  contracts::hardware::StepResult step(contracts::hardware::StepUnit unit, std::uint64_t count) override;
  contracts::hardware::Breakpoint addBreakpoint(contracts::hardware::BreakpointKind kind, std::uint16_t first, std::uint16_t last) override { return debug_.addBreakpoint(kind, first, last); }
  bool removeBreakpoint(std::uint32_t id) override { return debug_.removeBreakpoint(id); }
  std::size_t clearBreakpoints() override { return debug_.clearBreakpoints(); }
  [[nodiscard]] std::vector<contracts::hardware::Breakpoint> breakpoints() const override { return debug_.breakpoints(); }
  std::optional<contracts::hardware::BreakpointHit> takeBreakpointHit() override { return debug_.takeBreakpointHit(); }
  std::uint32_t startRecorder(const contracts::hardware::RecorderSpec& spec) override { return debug_.startRecorder(spec); }
  contracts::hardware::RecorderOutput stopRecorder(std::uint32_t id) override { return debug_.stopRecorder(id); }
  [[nodiscard]] std::vector<contracts::hardware::DisassembledInstruction> disassemble(std::uint16_t address, std::uint32_t count) override { return debug_.disassemble(address, count); }
  void startCdl(std::size_t size, contracts::hardware::CdlResolver resolver) override { debug_.startCdl(size, std::move(resolver)); }
  void stopCdl() override { debug_.stopCdl(); }
  [[nodiscard]] std::optional<std::vector<std::uint8_t>> cdlFlags() const override { return debug_.cdlFlags(); }

  /** 所有要在上电或复位时复位的功能模块。 */
  void resetDevices(bool power);
  /** 把游戏手柄的状态换算成手柄口的按键。 */
  void applyGamepad(const contracts::hardware::GamepadInput& gamepad);
  /** 把鼠标事件换算成串口鼠标的位移与按键。 */
  void applyMouse(const contracts::hardware::MouseInput& mouse);

  logic::hardware::MasterScheduler scheduler_;
  logic::hardware::WiredOrIrqLine irq_;
  logic::hardware::WiredOrIrqLine nmi_;
  logic::hardware::WiredOrRdyLine rdy_;
  /** 卡槽的 IRQ 引脚（整机机型没有卡带，只为满足 Machine 契约）。 */
  logic::hardware::LineIrqOutput cartridgeIrq_;
  /** 空卡槽：不挂载到任何总线。 */
  logic::hardware::StandardCartridgeSlot slot_;
  logic::hardware::CpuAddressBus cpuBus_;
  logic::hardware::MappedBus<contracts::hardware::PpuBus, 16> videoBus_;

  /** BIOS ROM。 */
  std::vector<std::uint8_t> bios_;
  logic::hardware::RamBlock ram_;
  logic::hardware::MirroredRam ramDevice_;
  logic::hardware::RamBlock externalRam_;
  logic::hardware::MirroredRam externalRamDevice_;
  Sb2000VideoMemory video_;
  logic::hardware::PagingUm6576 paging_;
  Sb2000ProgramMemory program_;
  logic::hardware::SysinitUm6576 sysinit_;
  Sb2000WorkWindow workWindow_;
  /** 没有接打印机的并口。 */
  contracts::hardware::NullParallelPort nullParallel_;
  /** 虚拟针式打印机；没有输出方时为空。 */
  std::unique_ptr<logic::hardware::PrinterEscp> printer_;
  /** 并口：接着打印机，没有打印机时是空口。 */
  contracts::hardware::ParallelPort& parallel_;

  logic::hardware::IrqUm6576 irqController_;
  logic::hardware::StandardFloppyDrive floppy_;
  logic::hardware::FdcUpd765 fdc_;
  Sb2000SuperIo superIo_;
  logic::hardware::TimerUm6576 timer_;
  logic::hardware::RtcUm6576 rtc_;
  logic::hardware::ExpioUm6576 expio_;
  logic::hardware::DacUm6576 dac_;
  logic::hardware::DmaUm6576 dma_;
  logic::hardware::KeyboardSb2000 keyboardDevice_;
  logic::hardware::MouseSb2000 mouseDevice_;
  logic::hardware::KeyboardUm6576 keyboardPort_;
  logic::hardware::MouseUm6576 mousePort_;
  logic::hardware::PrinterUm6576 printerPort_;
  logic::hardware::PadUm6576 pad_;
  logic::hardware::PsgUm6576 psg_;
  logic::hardware::PpuUm6576 ppu_;
  std::array<logic::hardware::Joypad, logic::hardware::PadUm6576::PORT_COUNT> joypads_;
  logic::hardware::Cpu6502 cpu_;
  /** 断点、记录、代码数据标记与反汇编。 */
  CpuDebugKit debug_;

  /** 鼠标位移里还没有凑够 1 个单位的小数部分。 */
  double mouseRemainderX_{0};
  double mouseRemainderY_{0};
  bool poweredOn_{false};
};

}  // namespace ember::bootstrap
