/*
 * ------------------------------------------------------------------------
 * 名称：update-engine.hpp
 * 说明：升级引擎的抽象：检查有没有新版本并下载好（回调在界面线程），以及启动已下载好的新版本接替当前程序。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-21
 * 备注：应用升级服务（AppUpdater）用它维护升级状态；一次检查的结果只会是下面四个回调之一的序列：available 之后是 downloaded 或 failed；notAvailable；failed。
 * ------------------------------------------------------------------------
 */

#pragma once

#include <functional>
#include <string>

namespace ember::win32::contracts {

/** 一次检查的回调，都在界面线程调用。 */
struct UpdateEngineHandlers {
  /** 发现了新版本，开始下载。 */
  std::function<void()> available;
  /** 已经是最新版本。 */
  std::function<void()> notAvailable;
  /** 新版本已经下载好并通过校验。 */
  std::function<void(const std::string& version)> downloaded;
  /** 检查或下载失败。 */
  std::function<void(const std::string& message)> failed;
};

/** 升级引擎。 */
class UpdateEngine {
 public:
  virtual ~UpdateEngine() = default;

  /** 当前运行方式是否支持升级。 */
  [[nodiscard]] virtual bool supported() const = 0;
  /** 开始一次检查（后台进行）；上一次检查还没结束时不要再调用。 */
  virtual void check(UpdateEngineHandlers handlers) = 0;
  /** 启动已下载好的新版本接替当前程序，成功后退出当前程序；启动不了时返回 false。 */
  virtual bool restart() = 0;
};

}  // namespace ember::win32::contracts
