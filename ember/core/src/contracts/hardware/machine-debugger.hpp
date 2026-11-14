/*
 * ------------------------------------------------------------------------
 * 名称：machine-debugger.hpp
 * 说明：硬件契约：主机的调试接口：内存空间的无副作用读与写、地址解析、CPU 与芯片状态的读写、按指令或周期或扫描线或帧单步。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-14
 * 备注：只在模拟线程使用；读不改变主机的任何状态（寄存器类地址读不到内容，读出 0）；
 *       卡带上的存储区不在这里，由卡带自己提供（见 contracts/mappers/mapper-cartridge.hpp），空间名 cart.<存储区标识> 由调用方合成。
 * ------------------------------------------------------------------------
 */

#pragma once

#include <cstddef>
#include <cstdint>
#include <functional>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#include "contracts/hardware/cpu.hpp"

namespace ember::contracts::hardware {

/** 一个内存空间。 */
struct MemorySpace {
  /** 空间标识，如 cpu、ppu、board.ram、chip.oam。 */
  std::string id;
  /** 说明。 */
  std::string description;
  /** 大小（字节）；地址从 0 到大小减 1。 */
  std::size_t size{0};
  /** 能否写入。 */
  bool writable{false};
};

/** 单步的单位。 */
enum class StepUnit : std::uint8_t {
  /** 一条指令（含随后响应的中断序列）。 */
  Instruction,
  /** 至少这么多 CPU 周期；指令不被打断，所以实际周期数可能多几个。 */
  Cycle,
  /** PPU 的扫描线推进这么多行。 */
  Scanline,
  /** PPU 完成这么多帧画面。 */
  Frame
};

/** 单步的结果。 */
struct StepResult {
  /** 实际执行的指令数。 */
  std::uint64_t instructions{0};
  /** 实际经过的 CPU 周期数。 */
  std::uint64_t cycles{0};
  /** 期间完成的帧数。 */
  std::uint64_t frames{0};
};

/** 一个总线地址对应的东西。 */
struct AddressTarget {
  /** 种类：ram、rom、register（芯片或设备的寄存器）、cartridge（由卡带响应）、none（没有设备）。 */
  std::string kind;
  /** 所在的存储区或芯片的标识；没有时为空。 */
  std::string region;
  /** 在存储区里的偏移，或在寄存器组里的序号。 */
  std::size_t offset{0};
  /** 说明。 */
  std::string description;
};

/** 一项状态：名称、取值与说明。 */
struct StateField {
  std::string name;
  std::int64_t value{0};
  std::string description;
};

/** 断点的种类。 */
enum class BreakpointKind : std::uint8_t {
  /** CPU 要执行这个地址上的指令之前停下（指令还没有执行）。 */
  Execute,
  /** 总线读到这个范围里的地址，那条指令执行完后停下。 */
  Read,
  /** 总线写到这个范围里的地址，那条指令执行完后停下。 */
  Write
};

/** 一个断点。 */
struct Breakpoint {
  std::uint32_t id{0};
  BreakpointKind kind{BreakpointKind::Execute};
  /** 地址范围的第一个与最后一个地址（含）。 */
  std::uint16_t first{0};
  std::uint16_t last{0};
  /** 已经命中的次数。 */
  std::uint64_t hits{0};
};

/** 一次断点命中。 */
struct BreakpointHit {
  std::uint32_t id{0};
  BreakpointKind kind{BreakpointKind::Execute};
  /** 执行断点是将要执行的指令地址；读写断点是被访问的地址。 */
  std::uint16_t address{0};
  /** 读写断点读到或写入的值；执行断点为 0。 */
  std::uint8_t value{0};
};

/** 记录的种类。 */
enum class RecorderKind : std::uint8_t {
  /** 每执行一条指令记一行：地址、字节、反汇编与寄存器。 */
  Instruction,
  /** 每次访问芯片寄存器、手柄口、卡带的扩展寄存器（$2000-$5FFF），以及对 $8000 以上的写（mapper 寄存器）记一行。 */
  Io,
  /** 每次 CPU 响应 IRQ、NMI 或 BRK 记一行。 */
  Irq,
  /** 每次执行到指定地址记一行：寄存器、栈顶与内存段。 */
  Call
};

/** 一段内存。 */
struct MemoryRange {
  std::uint16_t address{0};
  std::uint16_t length{0};
};

/** 开始一项记录的参数。 */
struct RecorderSpec {
  RecorderKind kind{RecorderKind::Instruction};
  /** Call：要记录的地址。 */
  std::uint16_t address{0};
  /** Call：每次记录栈顶的字节数。 */
  std::uint8_t stackBytes{0};
  /** Call：每次记录的内存段，CPU 总线上的地址。 */
  std::vector<MemoryRange> ranges;
  /** 最多记录的行数，满了就不再记录。 */
  std::uint32_t maxLines{100000};
  /** Instruction：每个地址只记第一次。 */
  bool unique{false};
};

/** 一项记录的结果。 */
struct RecorderOutput {
  std::vector<std::string> lines;
  /** 因为行数满了而少记了。 */
  bool truncated{false};
};

/** 代码与数据标记（CDL）的标记位，每个 PRG-ROM 字节一个标记字节，位定义同 Mesen。 */
/** 被当作指令执行（含操作数字节）。 */
inline constexpr std::uint8_t CDL_CODE = 0x01;
/** 被指令当数据读取。 */
inline constexpr std::uint8_t CDL_DATA = 0x02;
/** 是 JMP 或成立的分支的目标。 */
inline constexpr std::uint8_t CDL_JUMP_TARGET = 0x04;
/** 是 JSR 的目标（子程序入口）。 */
inline constexpr std::uint8_t CDL_SUB_ENTRY = 0x08;

/** CPU 地址换算成 PRG-ROM 的偏移（按当前的分页）；不在 PRG-ROM 里时返回空。 */
using CdlResolver = std::function<std::optional<std::size_t>(std::uint16_t)>;

/** 一条反汇编的指令。 */
struct DisassembledInstruction {
  std::uint16_t address{0};
  /** 指令字节，十六进制，空格隔开，如 "A9 42"。 */
  std::string bytes;
  /** 助记符与操作数，带有效地址与取值的注释，如 "LDA $0180 = 33"。 */
  std::string text;
  /** 是不是非官方指令。 */
  bool unofficial{false};
};

/** 主机的调试接口。 */
class MachineDebugger {
 public:
  virtual ~MachineDebugger() = default;

  /** 主机自己拥有的内存空间（不含卡带上的存储区）。 */
  [[nodiscard]] virtual std::vector<MemorySpace> memorySpaces() const = 0;

  /**
   * 无副作用地读一段内存。
   * @throws shared::CoreError hardware.unknownSpace、hardware.spaceOutOfRange。
   */
  virtual void readMemory(std::string_view space, std::uint32_t address, std::span<std::uint8_t> out) = 0;

  /**
   * 写一段内存。cpu 空间的写经真实的总线，与 CPU 写一样有副作用。
   * @throws shared::CoreError hardware.unknownSpace、hardware.spaceOutOfRange、hardware.spaceReadOnly。
   */
  virtual void writeMemory(std::string_view space, std::uint32_t address, std::span<const std::uint8_t> bytes) = 0;

  /**
   * 解析总线地址。
   * @param space cpu 或 ppu。
   * @throws shared::CoreError hardware.unknownSpace、hardware.spaceOutOfRange。
   */
  [[nodiscard]] virtual AddressTarget resolve(std::string_view space, std::uint32_t address) const = 0;

  /** 可以读写状态的模块标识，如 cpu、ppu。 */
  [[nodiscard]] virtual std::vector<std::string> stateModules() const = 0;

  /**
   * 读一个模块的状态。
   * @throws shared::CoreError hardware.invalidState：没有这个模块。
   */
  [[nodiscard]] virtual std::vector<StateField> readState(std::string_view module) const = 0;

  /**
   * 修改一个模块的状态；先全部检查，有任何一项不合格就一项也不改。
   * @throws shared::CoreError hardware.invalidState：模块不存在、状态项不存在或只读、取值超出范围。
   */
  virtual void writeState(std::string_view module, std::span<const StateField> fields) = 0;

  /**
   * 单步。
   * @param unit 单位。
   * @param count 数量，至少 1。
   */
  virtual StepResult step(StepUnit unit, std::uint64_t count) = 0;

  /**
   * 添加断点。断点在主机的 runFrame 里生效（自由运行与授权运行）；单步（step）忽略断点。
   * @param first 地址范围的第一个地址。
   * @param last 地址范围的最后一个地址（含），不小于 first。
   * @throws shared::CoreError hardware.invalidBreakpoint、hardware.breakpointLimit。
   */
  virtual Breakpoint addBreakpoint(BreakpointKind kind, std::uint16_t first, std::uint16_t last) = 0;

  /** 删除断点；没有这个编号时返回 false。 */
  virtual bool removeBreakpoint(std::uint32_t id) = 0;

  /** 删除全部断点，返回删除的个数。 */
  virtual std::size_t clearBreakpoints() = 0;

  /** 全部断点，按编号升序。 */
  [[nodiscard]] virtual std::vector<Breakpoint> breakpoints() const = 0;

  /** 取走最近一次运行停下的断点命中；没有命中时为空。 */
  virtual std::optional<BreakpointHit> takeBreakpointHit() = 0;

  /**
   * 开始一项记录，之后运行与单步产生的内容记进内存，直到 stopRecorder 取走。记录属于当前的主机。
   * @return 记录编号。
   * @throws shared::CoreError hardware.recorderLimit。
   */
  virtual std::uint32_t startRecorder(const RecorderSpec& spec) = 0;

  /**
   * 结束一项记录并取走内容。
   * @throws shared::CoreError hardware.noRecorder。
   */
  virtual RecorderOutput stopRecorder(std::uint32_t id) = 0;

  /**
   * 从 address 起反汇编 count 条指令（CPU 总线上的地址，无副作用地读内存，芯片寄存器类地址的取值显示 00；带索引的寻址用当前的 X、Y）。
   */
  [[nodiscard]] virtual std::vector<DisassembledInstruction> disassemble(std::uint16_t address, std::uint32_t count) = 0;

  /**
   * 开始代码与数据标记：标记清零，之后运行与单步执行的每条指令都打标记。
   * @param size PRG-ROM 的字节数。
   * @throws shared::CoreError hardware.cdlActive。
   */
  virtual void startCdl(std::size_t size, CdlResolver resolver) = 0;

  /** 停止标记，已有的标记保留到下一次 startCdl 或换主机。 */
  virtual void stopCdl() = 0;

  /** 标记（每个 PRG 字节一个标记字节）；没有记录过时为空。 */
  [[nodiscard]] virtual std::optional<std::vector<std::uint8_t>> cdlFlags() const = 0;
};

}  // namespace ember::contracts::hardware
