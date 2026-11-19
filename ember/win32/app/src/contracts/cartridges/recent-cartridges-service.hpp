/*
 * ------------------------------------------------------------------------
 * 名称：recent-cartridges-service.hpp
 * 说明：最近打开的卡带服务的抽象：清单里的路径只在外壳里，界面只能拿到显示名与本次运行内有效的编号。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-19
 * 备注：对应 Electron 外壳的 src/contracts/cartridges/recent-cartridges-service.ts；每个路径在本次运行内有固定的编号（从 1 递增）。
 * ------------------------------------------------------------------------
 */

#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace ember::win32::contracts {

/** 清单里的一项：编号与显示名。 */
struct RecentCartridge {
  std::int64_t id{0};
  std::string name;
};

/** 最近打开的卡带服务。 */
class RecentCartridgesService {
 public:
  virtual ~RecentCartridgesService() = default;

  /** 清单，最近的在前。 */
  [[nodiscard]] virtual std::vector<RecentCartridge> list() const = 0;

  /** 把卡带记到清单最前面（UTF-8 路径）。 */
  virtual void add(const std::string& path) = 0;

  /** 把卡带移出清单。 */
  virtual void remove(const std::string& path) = 0;

  /** 按编号取路径；不在清单里时为空。 */
  [[nodiscard]] virtual std::optional<std::string> resolve(std::int64_t id) const = 0;

  /** 立即写盘。 */
  virtual void flush() = 0;
};

}  // namespace ember::win32::contracts
