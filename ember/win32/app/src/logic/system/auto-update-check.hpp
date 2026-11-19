/*
 * ------------------------------------------------------------------------
 * 名称：auto-update-check.hpp
 * 说明：启动时自动检查更新：核心就绪后读取首选项 update.autoCheck，开启时等一小会儿再检查一次；之后设置项变化会立即生效（检查前关闭就取消，检查前才开启就安排检查）。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-21
 * 备注：每次运行只自动检查一次，之后只有用户在“帮助”菜单里手动检查；不支持升级的运行方式不做任何事；延迟是为了不与启动抢资源；核心重启后会重新读取设置项；
 *       延时任务由调用方提供（界面线程的延时任务），便于测试。
 * ------------------------------------------------------------------------
 */

#pragma once

#include <functional>
#include <memory>

#include "contracts/core-gateway/core-gateway.hpp"
#include "contracts/shell/request-broker.hpp"
#include "contracts/system/app-updater.hpp"

namespace ember::win32::logic {

/** 自动检查更新。 */
class AutoUpdateCheck {
 public:
  /** 安排一个延时任务，返回取消它的函数。 */
  using Schedule = std::function<std::function<void()>(unsigned delayMilliseconds, std::function<void()> task)>;

  /**
   * @param broker 请求中转：读取首选项经它。
   * @param core 核心网关：订阅核心事件与运行状态。
   * @param updater 升级服务。
   * @param schedule 延时任务。
   * @param delayMilliseconds 从开启自动检查到真正检查之间等待的毫秒数。
   */
  AutoUpdateCheck(contracts::RequestBroker& broker, contracts::CoreGateway& core, contracts::AppUpdater& updater, Schedule schedule, unsigned delayMilliseconds);
  ~AutoUpdateCheck();
  AutoUpdateCheck(const AutoUpdateCheck&) = delete;
  AutoUpdateCheck& operator=(const AutoUpdateCheck&) = delete;

  /** 开始：不支持升级时什么都不做。 */
  void start();
  /** 停止：取消还没到时间的检查，之后不再响应设置项的变化。 */
  void stop();

 private:
  void refresh();
  void apply(const nlohmann::json& value);
  void cancel();

  contracts::RequestBroker& broker_;
  contracts::CoreGateway& core_;
  contracts::AppUpdater& updater_;
  Schedule schedule_;
  unsigned delay_;
  std::function<void()> stopEvents_;
  std::function<void()> stopStatus_;
  std::function<void()> cancelTimer_;
  bool timerPending_{false};
  bool finished_{false};
  bool started_{false};
  std::shared_ptr<bool> alive_{std::make_shared<bool>(true)};
};

}  // namespace ember::win32::logic
