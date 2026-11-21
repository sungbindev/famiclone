/*
 * ------------------------------------------------------------------------
 * 名称：rom-controller.hpp
 * 说明：“机型与 ROM”对话框的控制器：核心就绪后读取机型清单，订阅 machines.changed 保持最新，核心重启后重新读取；执行 ROM 的添加或替换（弹出系统文件选择窗口）、从目录添加（弹出系统选择文件夹窗口）、移除，以及用文件管理器打开 ROM 目录。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-21
 * 备注：添加、替换、移除的结果与错误都用外壳通知给出，对话框里不显示结果；规则见 docs/05-界面.md 第 4.7 节。
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
#include "contracts/machines/machines-model.hpp"
#include "contracts/shell/request-broker.hpp"
#include "contracts/shell/shell-registry.hpp"
#include "logic/machines/machines-rules.hpp"

namespace ember::win32::logic {

/** “机型与 ROM”对话框的控制器。 */
class RomController final : public contracts::RomModel {
 public:
  /**
   * @param broker 请求中转：核心的请求与外壳自己处理的请求（选择 ROM 文件、打开 ROM 目录）都经它。
   * @param core 核心网关：订阅核心事件与核心运行状态。
   */
  RomController(contracts::RequestBroker& broker, contracts::CoreGateway& core);
  ~RomController() override;
  RomController(const RomController&) = delete;
  RomController& operator=(const RomController&) = delete;

  /**
   * 接上外壳：订阅核心事件与核心运行状态，并读取机型清单。
   * @returns 断开的函数。
   */
  std::function<void()> attach(contracts::ShellApi& shell);

  // ---- RomModel ----
  [[nodiscard]] contracts::RomView view() const override;
  void choose(const std::string& machine) override;
  void chooseDirectory() override;
  void remove(const std::string& machine) override;
  void openDirectory() override;
  [[nodiscard]] std::function<void()> subscribe(std::function<void()> handler) override;

 private:
  void load();
  void handleEvent(const std::string& name);
  void handleCoreStatus(const contracts::CoreStatus& status);
  void report(const std::string& text);
  void notifySuccess(const std::string& text);
  void changed();
  /** 请求：回复只在仍接着外壳时处理。 */
  void ask(const std::string& name, nlohmann::json params, std::function<void(const contracts::CoreReply&)> done);
  [[nodiscard]] const Machine* find(const std::string& id) const;

  contracts::RequestBroker& broker_;
  contracts::CoreGateway& core_;
  contracts::ShellApi* shell_{nullptr};
  std::vector<std::function<void()>> stops_;
  std::shared_ptr<bool> alive_{std::make_shared<bool>(true)};
  std::uint64_t generation_{0};
  std::uint64_t loadRequest_{0};

  contracts::MachinesLoad load_{contracts::MachinesLoad::Loading};
  std::string message_;
  std::vector<Machine> machines_;
  std::map<int, std::function<void()>> listeners_;
  int nextListener_{1};
};

}  // namespace ember::win32::logic
