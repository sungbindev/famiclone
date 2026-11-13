/*
 * ------------------------------------------------------------------------
 * 名称：mapper-cartridge.hpp
 * 说明：mapper 契约：由插件创建的卡带。在硬件契约的 Cartridge 之上，提供存储区访问、脚本状态与脚本错误的查询。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-14
 * 备注：脚本出错时卡带不抛异常（异常会穿过 CPU 的总线访问），而是记下错误并停止调用脚本，之后的访问按开放总线处理；
 *       运行模块在每次运行之后检查 failure()，发现错误就暂停模拟器并报告，见 docs/03-mapper插件规范.md 第 6.3 节。
 * ------------------------------------------------------------------------
 */

#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "contracts/hardware/cartridge.hpp"
#include "contracts/hardware/storage-region.hpp"

namespace ember::contracts::mappers {

/** 脚本的错误。 */
struct ScriptFailure {
  /** 错误码：mappers.scriptError 或 mappers.limitExceeded。 */
  std::string code;
  /** 出错的插件标识。 */
  std::string pluginId;
  /** 面向用户的说明，含插件标识。 */
  std::string message;
};

/** 卡带上一块存储区的摘要。 */
struct StorageSummary {
  /** 清单里的存储区标识。 */
  std::string id;
  hardware::StorageKind kind{hardware::StorageKind::Rom};
  /** 实际容量（字节），来源为空又没有后备存储时为 0。 */
  std::size_t size{0};
  /** 是否带电池，只对 RAM 有意义。 */
  bool battery{false};
};

/** 由插件创建的卡带。 */
class MapperCartridge : public hardware::Cartridge {
 public:
  /** 创建这个卡带的插件标识。 */
  [[nodiscard]] virtual const std::string& pluginId() const = 0;

  /** 卡带上全部存储区的摘要，按清单声明的顺序。 */
  [[nodiscard]] virtual std::vector<StorageSummary> storageList() const = 0;

  /**
   * 按清单里的标识取存储区，用于调试与测试读取；没有这个存储区时返回 nullptr。
   * 存储区由卡带拥有，只能经它访问。
   */
  [[nodiscard]] virtual const hardware::StorageRegion* storage(std::string_view id) const = 0;

  /** 脚本状态里某项的当前值；没有这项状态时返回空。 */
  [[nodiscard]] virtual std::optional<std::uint32_t> state(std::string_view name) const = 0;

  /** 脚本的错误；没有出错时为空，复位后清除。 */
  [[nodiscard]] virtual std::optional<ScriptFailure> failure() const = 0;

  /** 脚本经 ctx.log.warn 写入的消息，最多保留最近 200 条，一条最长 2000 字节。 */
  [[nodiscard]] virtual std::vector<std::string> scriptLog() const = 0;
};

}  // namespace ember::contracts::mappers
