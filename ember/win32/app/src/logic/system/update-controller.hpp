/*
 * ------------------------------------------------------------------------
 * 名称：update-controller.hpp
 * 说明：应用升级在界面里的控制器：保存升级引擎推送的状态，决定“帮助”菜单里检查更新项的名称与可用状态，执行检查与重启安装，并在合适的时候用通知告知用户。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-21
 * 备注：自动检查（启动时）只在新版本下载好时通知，用户手动检查才通知“已是最新版本”“检查失败”等结果；状态由升级引擎维护，这里只读取、订阅与发起请求（经 system.updateStatus、system.checkUpdate、system.installUpdate）。
 * ------------------------------------------------------------------------
 */

#pragma once

#include <cstdint>
#include <functional>
#include <memory>
#include <string>

#include "contracts/shell/request-broker.hpp"
#include "contracts/shell/shell-registry.hpp"
#include "contracts/system/app-updater.hpp"

namespace ember::win32::logic {

/** 升级控制器。 */
class UpdateController {
 public:
  /**
   * @param broker 请求中转：升级的三个请求经它。
   * @param updater 升级引擎：订阅它推送的状态。
   */
  UpdateController(contracts::RequestBroker& broker, contracts::AppUpdater& updater);
  ~UpdateController();
  UpdateController(const UpdateController&) = delete;
  UpdateController& operator=(const UpdateController&) = delete;

  /**
   * 接上外壳：读取当前状态并订阅变化；状态变化后让外壳重新计算菜单。
   * @returns 断开的函数。
   */
  std::function<void()> attach(contracts::ShellApi& shell);

  /** 菜单项当前显示的名称。 */
  [[nodiscard]] std::string title() const;
  /** 菜单项当前是否可用：检查与下载过程中不能再操作。 */
  [[nodiscard]] bool canRun() const;
  /** 新版本是否已经下载好，可以重启安装（标题栏的“更新”按钮据此显示）。 */
  [[nodiscard]] bool isReady() const { return status_.state == "ready"; }
  /** 执行菜单项：新版本已下载好时重启安装，否则手动检查更新；失败与结果用外壳的通知提示。 */
  void run(contracts::ShellApi& shell);

 private:
  /** 记下新状态，让外壳重算菜单，并按需要通知用户；announce 为假时只记录不通知（读取初始状态）。 */
  void apply(const contracts::UpdateStatus& next, bool announce);
  /** 请求：回复只在仍接着外壳时处理。 */
  void ask(const std::string& name, std::function<void(const contracts::CoreReply&)> done);

  contracts::RequestBroker& broker_;
  contracts::AppUpdater& updater_;
  contracts::ShellApi* shell_{nullptr};
  std::function<void()> stop_;
  std::shared_ptr<bool> alive_{std::make_shared<bool>(true)};
  std::uint64_t generation_{0};
  contracts::UpdateStatus status_{"idle", std::nullopt, std::nullopt};
  /** 当前这次检查是不是用户手动发起的。 */
  bool manual_{false};
};

}  // namespace ember::win32::logic
