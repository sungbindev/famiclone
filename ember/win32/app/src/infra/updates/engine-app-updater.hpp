/*
 * ------------------------------------------------------------------------
 * 名称：engine-app-updater.hpp
 * 说明：应用升级服务的实现：用升级引擎检查与下载新版本，维护升级状态（不支持、空闲、检查中、下载中、已下载、已是最新、失败）并通知订阅者。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-21
 * 备注：检查阶段有超时（网络偶尔不给任何回应，不超时的话状态会一直停在“检查中”，再也不能重新检查），下载阶段不设超时，失败由引擎报告；已经下载好（ready）之后不再接受引擎的事件，也不再检查；
 *       所有方法与回调都在界面线程。
 * ------------------------------------------------------------------------
 */

#pragma once

#include <map>

#include "contracts/system/app-updater.hpp"
#include "contracts/system/update-engine.hpp"
#include "platform/threading/ui-thread.hpp"

namespace ember::win32::infra {

/** 基于升级引擎的应用升级服务。 */
class EngineAppUpdater final : public contracts::AppUpdater {
 public:
  /**
   * @param engine 升级引擎。
   * @param ui 界面线程：检查阶段的超时用它的延时任务。
   * @param checkTimeoutMilliseconds 检查阶段的超时（毫秒）。
   */
  EngineAppUpdater(contracts::UpdateEngine& engine, platform::UiThread& ui, unsigned checkTimeoutMilliseconds = 60000);
  ~EngineAppUpdater() override;
  EngineAppUpdater(const EngineAppUpdater&) = delete;
  EngineAppUpdater& operator=(const EngineAppUpdater&) = delete;

  [[nodiscard]] contracts::UpdateStatus status() const override { return current_; }
  contracts::UpdateStatus check() override;
  bool install() override;
  [[nodiscard]] std::function<void()> subscribe(std::function<void(const contracts::UpdateStatus&)> handler) override;

 private:
  /** 换成新状态并通知订阅者；离开检查阶段时撤掉超时。 */
  void update(contracts::UpdateStatus next);

  contracts::UpdateEngine& engine_;
  platform::UiThread& ui_;
  unsigned checkTimeout_;
  contracts::UpdateStatus current_;
  unsigned timer_{0};
  std::map<int, std::function<void(const contracts::UpdateStatus&)>> listeners_;
  int nextListener_{1};
  /** 引擎回调在已销毁的对象上执行时丢弃。 */
  std::shared_ptr<bool> alive_{std::make_shared<bool>(true)};
};

}  // namespace ember::win32::infra
