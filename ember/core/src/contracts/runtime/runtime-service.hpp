/*
 * ------------------------------------------------------------------------
 * 名称：runtime-service.hpp
 * 说明：runtime 契约：机器的运行服务。电源、复位、暂停与继续，装入与取出卡带，按键输入，运行状态。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-14
 * 备注：实现由一条模拟线程拥有全部硬件状态（见 docs/01-软件架构设计.md 第 8 节）；改变硬件状态的方法把命令投递给模拟线程并等它执行完，
 *       所以可以在任意线程调用（不要在模拟线程的回调里调用）；核心停止后调用抛出 core.unavailable。数据类型由 schema/runtime.schema.json 生成。
 * ------------------------------------------------------------------------
 */

#pragma once

#include <cstdint>
#include <functional>
#include <future>
#include <optional>
#include <string>
#include <vector>

#include "contracts/hardware/input-event.hpp"
#include "contracts/hardware/machine.hpp"
#include "contracts/mappers/cartridge-image.hpp"
#include "contracts/mappers/mapper-cartridge.hpp"
#include "contracts/mappers/mapper-factory.hpp"
#include "contracts/runtime/generated/runtime.hpp"

namespace ember::contracts::runtime {

/** 复位方式。 */
enum class ResetKind : std::uint8_t {
  /** 只发复位信号，电源、运行模式与存储区保持。 */
  Soft,
  /** 断电后再上电，运行模式保持。 */
  Hard
};

/** 装入卡带的请求。 */
struct CartridgeInsertion {
  /** 卡带映像。 */
  mappers::CartridgeImage image;
  /** 匹配到的插件与要装配的主机型号。 */
  mappers::MapperMatch match;
  /** NES 文件的路径，原样放进运行状态里的卡带描述。 */
  std::string source;
  /** 装入后是否上电。 */
  bool powerOn{true};
  /** 上电后是否直接运行；powerOn 为 false 时无效。 */
  bool resume{true};
};

/** 装入卡带的结果。 */
struct CartridgeInsertionResult {
  CartridgeSummary cartridge;
  RuntimeState state;
};

/** 整机机型开机的请求。 */
struct MachineBoot {
  /** 机型标识，如 sb2000。 */
  std::string machine;
  /** BIOS ROM 的内容（不含文件头）。 */
  std::vector<std::uint8_t> bios;
  /** 装配后是否上电。 */
  bool powerOn{true};
  /** 上电后是否直接运行；powerOn 为 false 时无效。 */
  bool resume{true};
};

/** 一次授权运行（runtime.run）的结果。 */
struct RunOutcome {
  /** 实际运行的帧数。 */
  std::int64_t frames{0};
  /** 实际经过的 CPU 周期数。 */
  std::int64_t cycles{0};
  /** 停止原因：completed 运行完授权的帧数；interrupted 被暂停、继续、复位、换卡带或新的授权运行打断；powerOff 断电；error 运行出错。 */
  std::string stopped;
};

/** 一次单步（runtime.step）的结果。 */
struct StepOutcome {
  std::int64_t instructions{0};
  std::int64_t cycles{0};
  std::int64_t frames{0};
  /** 停下时的 PC。 */
  std::uint16_t pc{0};
  RuntimeState state;
};

/** 调试访问时，模拟线程交给回调的对象；只在回调里有效。 */
struct DebugContext {
  hardware::Machine& machine;
  hardware::MachineDebugger& debugger;
  /** 插着的卡带；整机机型没有卡带时为空。 */
  mappers::MapperCartridge* cartridge;
};

/** 一帧画面：像素按行从上到下，每像素 4 字节，内存顺序 R、G、B、A；没有画面时宽高为 0。 */
struct FrameImage {
  std::uint32_t width{0};
  std::uint32_t height{0};
  std::vector<std::uint8_t> rgba;
};

/** 运行服务。 */
class RuntimeService {
 public:
  virtual ~RuntimeService() = default;

  /** 当前的运行状态；直接读模拟线程最近发布的快照，不经命令队列。 */
  [[nodiscard]] virtual RuntimeStatusResult status() const = 0;

  /** 当前插着的卡带；没有时为空。 */
  [[nodiscard]] virtual std::optional<CartridgeSummary> cartridge() const = 0;

  /**
   * 上电：已通电时等同硬复位。
   * @param resume 上电后是否直接运行。
   * @throws shared::CoreError runtime.noCartridge。
   */
  virtual RuntimeState powerOn(bool resume) = 0;

  /** 断电；已断电或没有主机时没有效果。 */
  virtual RuntimeState powerOff() = 0;

  /**
   * 复位。
   * @throws shared::CoreError runtime.noCartridge；软复位时断电为 runtime.notPowered。
   */
  virtual RuntimeState reset(ResetKind kind) = 0;

  /** 暂停；已暂停时没有效果。@throws shared::CoreError runtime.notPowered。 */
  virtual RuntimeState pause() = 0;

  /** 继续运行；已在运行时没有效果。@throws shared::CoreError runtime.notPowered。 */
  virtual RuntimeState resume() = 0;

  /**
   * 装配主机并插入卡槽，替换已有的卡带与主机；任何一步失败都不改变已有的主机。
   * @throws shared::CoreError runtime.hostUnsupported，以及 mappers.notFound、mappers.invalidPackage、mappers.limitExceeded。
   */
  virtual CartridgeInsertionResult insertCartridge(CartridgeInsertion insertion) = 0;

  /** 取出卡带并拆除主机，电源变为断电；没有卡带时没有效果。 */
  virtual RuntimeState ejectCartridge() = 0;

  /**
   * 装配整机机型的主机（没有卡带），替换已有的卡带与主机；任何一步失败都不改变已有的主机。
   * @throws shared::CoreError runtime.hostUnsupported，以及 hardware.invalidArgument（BIOS 大小不对）。
   */
  virtual RuntimeState bootMachine(MachineBoot boot) = 0;

  /**
   * 在模拟线程上用主机做一件事，等它执行完；不要求通电（用于盘片等与电源无关的操作），回调里的异常原样抛给调用方。
   * @return 有主机并执行了回调返回 true；还没有主机时返回 false。
   */
  virtual bool useMachine(const std::function<void(hardware::Machine&)>& action) = 0;

  /**
   * 把用户输入（键盘、鼠标、游戏手柄的原始事件）经命令队列交给模拟线程，不等待执行；没有主机时丢弃。
   * 主机型号没有自己的键位，事件交给卡带的 mapper 脚本处理。
   */
  virtual void sendInput(hardware::InputEvent event) = 0;

  /** 设置 runtime.frame 事件的发布间隔，每 frames 帧一次；0 表示不发布（默认）。 */
  virtual void setFrameEventInterval(std::uint32_t frames) = 0;

  /**
   * 授权运行：不限速地运行指定的帧数后暂停（运行中不输出声音）。已在授权运行中时旧的被打断（interrupted），由新的替代。
   * 运行期间收到暂停、继续、硬复位、断电、换卡带会结束它；完成或结束时发 runtime.runFinished。
   * @param frames 要运行的帧数，正整数。
   * @returns 结果：运行结束后就绪；调用方可以等，也可以不等。
   * @throws shared::CoreError runtime.noCartridge、runtime.notPowered。
   */
  virtual std::shared_future<RunOutcome> startRun(std::int64_t frames) = 0;

  /** 结束正在进行的授权运行（interrupted），机器暂停；没有授权运行时没有效果。 */
  virtual void cancelRun() = 0;

  /** 手柄口的个数；没有主机时为 0。 */
  virtual unsigned padPortCount() = 0;

  /** 直接设置某个手柄口的按键（不经 mapper 脚本）；没有主机或没有这个口时丢弃。 */
  virtual void setPadButtons(unsigned port, std::uint8_t buttons) = 0;

  /** 最近一次完成的画面的副本（暂停、断电后仍然有上一帧）；没有主机或还没有完成过一帧时宽高为 0。 */
  virtual FrameImage captureFrame() = 0;

  /**
   * 单步：先暂停（打断正在进行的授权运行），再按单位运行指定的数量后保持暂停；经过的帧照常发布。
   * @throws shared::CoreError runtime.noCartridge、runtime.notPowered、runtime.debugUnsupported。
   */
  virtual StepOutcome step(hardware::StepUnit unit, std::int64_t count) = 0;

  /**
   * 调试访问：在模拟线程上执行回调（运行中在下一帧边界，暂停时立即），等它执行完；回调里的异常原样抛给调用方。
   * @throws shared::CoreError runtime.noCartridge、runtime.notPowered、runtime.debugUnsupported。
   */
  virtual void inspect(const std::function<void(DebugContext&)>& action) = 0;

  /**
   * 设置模拟线程上的观察者（替换已有的，传空函数取消）。主机通电时，每帧完成后、单步后、命中断点后、每次检查请求执行完后调用；
   * 回调里不要调用本服务的方法，不要抛出异常（抛出的异常只写日志）。
   */
  virtual void setObserver(std::function<void(DebugContext&)> observer) = 0;
};

}  // namespace ember::contracts::runtime
