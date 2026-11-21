/*
 * ------------------------------------------------------------------------
 * 名称：gamepad-settings-controller.hpp
 * 说明：手柄设置模块的控制器：核心就绪后读取两个玩家的绑定，订阅 input.bindingsChanged 保持最新，核心重启后重新读取；记住对话框里的操作状态（当前玩家页签、每个玩家的输入来源与选中的键、正在学习的键）；执行学习、清除、指定手柄与恢复默认。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-20
 * 备注：保存与恢复默认不在本地先改：请求成功后以核心给出的结果为准，失败时保持原样并用通知给出核心的说明；
 *       学习键盘：对话框窗口把下一次物理按键交给 keyPressed；学习实体手柄：开始学习时立即读一次手柄取基准，界面按 pollIntervalMs 定时调用 poll；开始时还按着按钮就先等全部松开，基准在学习期间跟着手柄回到静止；规则见 docs/05-界面.md 第 4.6 节。
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
#include "contracts/input/gamepad-reader.hpp"
#include "contracts/input/gamepad-settings-model.hpp"
#include "contracts/shell/request-broker.hpp"
#include "contracts/shell/shell-registry.hpp"
#include "logic/input/pad-rules.hpp"

namespace ember::win32::logic {

/** 手柄设置模块的控制器。 */
class GamepadSettingsController final : public contracts::GamepadSettingsModel {
 public:
  /**
   * @param broker 请求中转：绑定的读取、保存与恢复默认都经它。
   * @param core 核心网关：订阅核心事件与核心运行状态。
   * @param reader 手柄读取端口：发现手柄与学习手柄上的按钮或轴。
   */
  GamepadSettingsController(contracts::RequestBroker& broker, contracts::CoreGateway& core, contracts::GamepadReader& reader);
  ~GamepadSettingsController() override;
  GamepadSettingsController(const GamepadSettingsController&) = delete;
  GamepadSettingsController& operator=(const GamepadSettingsController&) = delete;

  /**
   * 接上外壳：订阅核心事件与核心运行状态，并读取绑定。
   * @returns 断开的函数。
   */
  std::function<void()> attach(contracts::ShellApi& shell);

  // ---- GamepadSettingsModel ----
  [[nodiscard]] contracts::PadSettingsView view() const override;
  void selectPlayer(int player) override;
  void setSource(int player, contracts::PadInputSource source) override;
  void pickKey(int player, const std::string& key) override;
  void clearSelected(int player) override;
  void assignDevice(int player, const std::string& deviceValue) override;
  void reset() override;
  void cancelLearning() override;
  [[nodiscard]] bool keyPressed(const std::string& code) override;
  [[nodiscard]] bool learningKey() const override { return waiting_ && !waiting_->pad; }
  void poll() override;
  [[nodiscard]] unsigned pollIntervalMs() const override;
  [[nodiscard]] std::function<void()> subscribe(std::function<void()> handler) override;

 private:
  /** 正在学习的键：键盘的，或指定设备上的手柄的。 */
  struct Waiting {
    bool pad{false};
    int player{0};
    std::string button;
    PadDevice device;
    /** 手柄学习：开始时手柄上还有按钮按着，等全部松开才开始认。 */
    bool holding{false};
  };
  /** 一个玩家页签里的操作状态。 */
  struct PlayerState {
    contracts::PadInputSource source{contracts::PadInputSource::Keyboard};
    std::string selected;
  };

  void load();
  void handleEvent(const std::string& name, const nlohmann::json& data);
  void handleCoreStatus(const contracts::CoreStatus& status);
  /** 保存修改后的槽位：成功后以核心给出的结果为准，失败时用通知提示。 */
  void apply(const std::vector<PadSlot>& slots);
  void report(const std::string& text);
  /** 以这一刻手柄的状态为学习基准；读不到这个手柄时返回 false。 */
  bool captureBaseline();
  void changed();
  void ask(const std::string& name, nlohmann::json params, std::function<void(const contracts::CoreReply&)> done);
  [[nodiscard]] bool validPlayer(int player) const { return player >= 0 && static_cast<std::size_t>(player) < slots_.size(); }
  [[nodiscard]] bool isConnected(const PadDevice& device) const;
  /** 玩家当前来源下一个键的绑定文字；没有绑定时没有值。 */
  [[nodiscard]] std::optional<std::string> boundText(int player, const std::string& button) const;
  [[nodiscard]] contracts::PadPlayerView buildPlayer(int player) const;

  contracts::RequestBroker& broker_;
  contracts::CoreGateway& core_;
  contracts::GamepadReader& reader_;
  contracts::ShellApi* shell_{nullptr};
  std::vector<std::function<void()>> stops_;
  std::shared_ptr<bool> alive_{std::make_shared<bool>(true)};
  std::uint64_t generation_{0};
  std::uint64_t loadRequest_{0};

  contracts::PadSettingsLoad load_{contracts::PadSettingsLoad::Loading};
  std::string message_;
  std::vector<PadSlot> slots_;
  std::vector<PlayerState> players_;
  int activePlayer_{0};
  std::optional<Waiting> waiting_;
  std::optional<PadBaseline> baseline_;
  std::vector<contracts::GamepadSample> samples_;
  std::vector<PadDevice> devices_;
  std::map<int, std::function<void()>> listeners_;
  int nextListener_{1};
};

}  // namespace ember::win32::logic
