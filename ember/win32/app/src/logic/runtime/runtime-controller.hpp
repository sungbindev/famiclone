/*
 * ------------------------------------------------------------------------
 * 名称：runtime-controller.hpp
 * 说明：运行模块的控制器：保存机器状态、最近打开的清单、机型清单与已选机型、音量与画面设置，执行开机、断电、暂停、复位、取出卡带、打开卡带、调音量等操作，订阅核心事件与核心运行状态。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-19
 * 备注：对应 Electron 外壳的 runtime-controller.ts 与 use-display-settings.ts；状态变化时调用 setOnChanged 设置的回调与外壳的 refreshCommands，所以面板没有展开、显示区没有显示时状态也是最新的。
 *       “选择机型”选中整机机型后直接用它的 BIOS 开机（selectAndBootMachine，选中的机型保存在界面状态里，下次启动沿用，之后点“开机”直接开这个机型），不影响装入的卡带；打开卡带成功（cartridges.loaded）后已选机型自动变成经典 FC。
 *       开机与断电是同一个操作（togglePower）：通电时断电；未通电时，已选机型是整机机型（学习机）就用它的 BIOS 开机（不需要卡带，当前装的是别的主机也用它重新开机），
 *       已选机型是经典 FC 就要求已经装入 FC 卡带（从“文件→打开卡带”选 nes，选后直接开机），断电后再开机直接上电。
 *       操作失败时用通知提示；核心重启时状态先清成“未读到”，核心就绪后重新读取状态、音量、最近打开的清单与设置。
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
#include "contracts/runtime/runtime-model.hpp"
#include "contracts/shell/request-broker.hpp"
#include "contracts/shell/shell-registry.hpp"
#include "contracts/shell/shell-state-service.hpp"
#include "logic/runtime/runtime-rules.hpp"

namespace ember::win32::logic {

/** 运行模块的控制器。 */
class RuntimeController final : public contracts::RuntimeModel {
 public:
  /**
   * @param broker 请求中转：核心的请求与外壳自己处理的请求（打开卡带、最近打开）都经它。
   * @param core 核心网关：订阅核心事件与核心运行状态。
   * @param state 界面状态：保存已选机型。
   */
  RuntimeController(contracts::RequestBroker& broker, contracts::CoreGateway& core, contracts::ShellStateService& state);
  ~RuntimeController() override;
  RuntimeController(const RuntimeController&) = delete;
  RuntimeController& operator=(const RuntimeController&) = delete;

  /**
   * 接上外壳：订阅核心事件与核心运行状态，并读取初始状态。
   * @returns 断开的函数。
   */
  std::function<void()> attach(contracts::ShellApi& shell);

  // ---- 状态 ----
  [[nodiscard]] const std::optional<RuntimeStatus>& status() const { return status_; }
  [[nodiscard]] const std::vector<RecentCartridge>& recent() const { return recent_; }
  [[nodiscard]] const std::vector<MachineEntry>& machines() const { return machines_; }
  [[nodiscard]] const std::string& selectedMachine() const { return selectedMachine_; }
  [[nodiscard]] bool muted() const { return muted_; }
  [[nodiscard]] bool familyKeyboard() const { return familyKeyboard_; }
  [[nodiscard]] contracts::RuntimeControls availability() const;

  // ---- 操作 ----
  /** 选择机型：只改已选机型并保存，不开机；已选机型已经是它时什么都不做。 */
  void selectMachine(const std::string& id);
  /** “选择机型”菜单里选中一个整机机型：记住它并直接开机（已经在运行的同一机型也重新开机）。 */
  void selectAndBootMachine(const std::string& id);
  /** 关闭（取出）卡带；没有卡带时什么都不做。 */
  void ejectCartridge();
  /** 弹出系统文件选择窗口打开卡带（选中后直接开机）；用户取消时什么都不发生。 */
  void openCartridge();
  /** 打开拖入窗口的卡带（编号来自外壳登记的拖入文件），和“打开卡带”一样选中后直接开机。 */
  void openDroppedCartridge(std::int64_t droppedId);
  /** 重新打开最近打开清单里的卡带。 */
  void openRecent(std::int64_t id);
  /** 接入或拔下 Family BASIC 键盘：保存到首选项，核心据此接到经典 FC 的扩展口；状态以首选项变化事件为准。 */
  void toggleFamilyKeyboard();

  // ---- RuntimeModel ----
  [[nodiscard]] contracts::RuntimeView view() const override;
  void togglePower() override;
  void togglePause() override;
  void softReset() override;
  void hardReset() override;
  void setVolume(int value) override;
  void toggleMute() override;
  void setScreenFocus(std::function<void()> focus) override { screenFocus_ = std::move(focus); }
  [[nodiscard]] std::function<void()> subscribe(std::function<void()> handler) override;

 private:
  void perform(const std::string& action, const std::string& request, nlohmann::json params, bool refreshRecentAfter = false);
  void report(const std::string& action, const contracts::CoreReply& reply);
  void bootMachine(const std::string& id);
  void adjustSound(nlohmann::json params);
  void handleEvent(const std::string& name, const nlohmann::json& data);
  void handleCoreStatus(const contracts::CoreStatus& status);
  void refreshAll();
  void refreshStatus();
  void refreshRecent();
  void refreshMachines();
  void refreshVolume();
  void refreshSettings();
  void applySetting(const std::string& id, const nlohmann::json& value);
  void changed();
  /** 请求：回复只在仍接着外壳时处理。 */
  void ask(const std::string& name, nlohmann::json params, std::function<void(const contracts::CoreReply&)> done);

  contracts::RequestBroker& broker_;
  contracts::CoreGateway& core_;
  contracts::ShellStateService& state_;
  contracts::ShellApi* shell_{nullptr};
  std::vector<std::function<void()>> stops_;
  /** 控制器存在期间为 true：回复到达时控制器已经销毁就不处理。 */
  std::shared_ptr<bool> alive_{std::make_shared<bool>(true)};
  /** 每次接上外壳加一：上一次接上时发出的请求，回复到了也不处理。 */
  std::uint64_t generation_{0};

  std::optional<RuntimeStatus> status_;
  std::vector<RecentCartridge> recent_;
  std::vector<MachineEntry> machines_;
  std::string selectedMachine_{kClassicMachine};
  int volume_{100};
  bool muted_{false};
  bool familyKeyboard_{false};
  contracts::ScreenSettings screen_;
  /** 最近一次读取状态的编号：迟到的旧结果不覆盖新结果。 */
  std::uint64_t statusRequest_{0};
  /** 最近一次调整声音的编号：迟到的旧结果不覆盖新结果。 */
  std::uint64_t soundRequest_{0};
  std::function<void()> screenFocus_;
  std::map<int, std::function<void()>> listeners_;
  int nextListener_{1};
};

}  // namespace ember::win32::logic
