/*
 * ------------------------------------------------------------------------
 * 名称：app-updater.hpp
 * 说明：应用升级的抽象：读取升级状态、开始检查更新、重启后使用已下载好的新版本。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-19
 * 备注：状态的取值见 schema/system.schema.json 的 UpdateStatus；
 *       原生外壳的升级引擎在阶段 11 加入，在此之前由“不支持升级”的实现提供，状态固定是 unsupported。
 * ------------------------------------------------------------------------
 */

#pragma once

#include <functional>
#include <optional>
#include <string>

namespace ember::win32::contracts {

/** 升级状态。 */
struct UpdateStatus {
  /** unsupported、idle、checking、downloading、ready、upToDate、error。 */
  std::string state;
  /** 新版本的版本号，只在 ready 时有。 */
  std::optional<std::string> version;
  /** 失败的原因，只在 error 时有。 */
  std::optional<std::string> message;
};

/** 应用升级。 */
class AppUpdater {
 public:
  virtual ~AppUpdater() = default;

  /** 当前状态。 */
  [[nodiscard]] virtual UpdateStatus status() const = 0;

  /** 开始检查更新；返回开始后的状态。 */
  virtual UpdateStatus check() = 0;

  /** 重启并使用已下载好的新版本；没有下载好的新版本时返回 false。 */
  virtual bool install() = 0;

  /** 订阅状态变化（在界面线程调用）；返回取消订阅的函数。 */
  [[nodiscard]] virtual std::function<void()> subscribe(std::function<void(const UpdateStatus&)> handler) = 0;
};

}  // namespace ember::win32::contracts
