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
#include <optional>
#include <string>

#include "contracts/mappers/cartridge-image.hpp"
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
   * 设置手柄的按键，经命令队列在模拟线程里生效，不等待执行；没有主机时记住，装入主机后生效。
   * @param port 手柄口编号，0 或 1。
   * @param buttons 按键掩码，位 0 为 A，位 7 为右。
   * @throws shared::CoreError core.invalidParams：口号超出范围。
   */
  virtual void setButtons(unsigned port, std::uint8_t buttons) = 0;

  /** 设置 runtime.frame 事件的发布间隔，每 frames 帧一次；0 表示不发布（默认）。 */
  virtual void setFrameEventInterval(std::uint32_t frames) = 0;
};

}  // namespace ember::contracts::runtime
