/*
 * ------------------------------------------------------------------------
 * 名称：preferences-controller.hpp
 * 说明：首选项模块的控制器：核心就绪后读取全部分组与设置项，订阅 preferences.changed 保持取值最新（在别处修改也会反映），核心重启后重新读取；执行修改、恢复默认与选择文件夹。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-20
 * 备注：对应 Electron 外壳的 use-preferences.ts；修改与恢复默认不在本地先改取值：请求成功后以核心给出的取值为准，失败时取值保持原样并用通知给出核心的说明；
 *       每次操作结束都通知订阅者（失败时取值没变，界面据此把控件恢复成模型里的值）；规则见 docs/05-界面.md 第 4.5 节。
 * ------------------------------------------------------------------------
 */

#pragma once

#include <cstdint>
#include <functional>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "contracts/core-gateway/core-gateway.hpp"
#include "contracts/preferences/preferences-model.hpp"
#include "contracts/shell/request-broker.hpp"
#include "contracts/shell/shell-registry.hpp"
#include "logic/preferences/preferences-rules.hpp"

namespace ember::win32::logic {

/** 首选项模块的控制器。 */
class PreferencesController final : public contracts::PreferencesModel {
 public:
  /**
   * @param broker 请求中转：核心的请求与外壳自己处理的请求（选择文件夹）都经它。
   * @param core 核心网关：订阅核心事件与核心运行状态。
   */
  PreferencesController(contracts::RequestBroker& broker, contracts::CoreGateway& core);
  ~PreferencesController() override;
  PreferencesController(const PreferencesController&) = delete;
  PreferencesController& operator=(const PreferencesController&) = delete;

  /**
   * 接上外壳：订阅核心事件与核心运行状态，并读取全部设置项。
   * @returns 断开的函数。
   */
  std::function<void()> attach(contracts::ShellApi& shell);

  // ---- PreferencesModel ----
  [[nodiscard]] contracts::PreferencesView view() const override;
  void setChoice(const std::string& id, const std::string& value) override;
  void setSwitch(const std::string& id, bool value) override;
  std::string submitText(const std::string& id, const std::string& text) override;
  void reset(const std::string& id) override;
  void pickDirectory(const std::string& id) override;
  [[nodiscard]] std::function<void()> subscribe(std::function<void()> handler) override;

 private:
  void load();
  void handleEvent(const std::string& name, const nlohmann::json& data);
  void handleCoreStatus(const contracts::CoreStatus& status);
  /** 保存新值：成功后以核心给出的取值为准，失败时用通知给出原因。 */
  void save(const std::string& id, nlohmann::json value);
  void report(const std::string& text);
  void changed();
  /** 请求：回复只在仍接着外壳时处理。 */
  void ask(const std::string& name, nlohmann::json params, std::function<void(const contracts::CoreReply&)> done);
  [[nodiscard]] const Setting* find(const std::string& id) const;

  contracts::RequestBroker& broker_;
  contracts::CoreGateway& core_;
  contracts::ShellApi* shell_{nullptr};
  std::vector<std::function<void()>> stops_;
  /** 控制器存在期间为 true：回复到达时控制器已经销毁就不处理。 */
  std::shared_ptr<bool> alive_{std::make_shared<bool>(true)};
  /** 每次接上外壳加一：上一次接上时发出的请求，回复到了也不处理。 */
  std::uint64_t generation_{0};
  /** 最近一次读取的编号：迟到的旧结果不覆盖新结果。 */
  std::uint64_t loadRequest_{0};

  contracts::PreferencesLoad load_{contracts::PreferencesLoad::Loading};
  std::string message_;
  std::optional<SettingListing> listing_;
  std::map<int, std::function<void()>> listeners_;
  int nextListener_{1};
};

}  // namespace ember::win32::logic
