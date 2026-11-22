/*
 * ------------------------------------------------------------------------
 * 名称：runtime-controller.cpp
 * 说明：运行模块控制器的实现。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-19
 * 备注：无。
 * ------------------------------------------------------------------------
 */

#include "logic/runtime/runtime-controller.hpp"

#include <algorithm>
#include <cmath>

namespace ember::win32::logic {

namespace {

/** 画面效果的首选项标识。 */
constexpr const char* kScreenFilterSetting = "display.filter";

std::string failedText(const std::string& action, const std::string& reason) { return action + "失败：" + reason; }

}  // namespace

RuntimeController::RuntimeController(contracts::RequestBroker& broker, contracts::CoreGateway& core, contracts::ShellStateService& state) : broker_(broker), core_(core), state_(state) {}

RuntimeController::~RuntimeController() {
  *alive_ = false;
  for (const auto& stop : stops_) stop();
}

std::function<void()> RuntimeController::attach(contracts::ShellApi& shell) {
  const auto stopAll = [this] {
    for (const auto& stop : stops_) stop();
    stops_.clear();
    shell_ = nullptr;
    ++generation_;
    ++statusRequest_;
  };
  stopAll();
  shell_ = &shell;
  stops_.push_back(core_.onEvent([this](const std::string& name, const nlohmann::json& data) { handleEvent(name, data); }));
  stops_.push_back(core_.onStatusChange([this](const contracts::CoreStatus& status) { handleCoreStatus(status); }));
  if (const auto& saved = state_.get().layout.selectedMachine) selectedMachine_ = *saved;
  refreshAll();
  return stopAll;
}

contracts::RuntimeControls RuntimeController::availability() const { return controlAvailability(status_, selectedMachine_, machines_); }

contracts::RuntimeView RuntimeController::view() const {
  contracts::RuntimeView result;
  result.state = machineState(status_);
  result.powerToggleText = powerToggleText(status_);
  result.pauseToggleText = pauseToggleText(status_);
  result.controls = availability();
  result.volume = volume_;
  result.muted = muted_;
  result.capturesMouse = capturesMouse(screen_.captureMouse, status_);
  result.exclusiveKeyboard = exclusiveKeyboard(familyKeyboard_, status_);
  result.resetCount = resetCount_;
  result.screen = screen_;
  return result;
}

void RuntimeController::changed() {
  // 回调里可能取消订阅，先复制一份再逐个调用
  const auto listeners = listeners_;
  for (const auto& [id, listener] : listeners) {
    if (listeners_.find(id) != listeners_.end()) listener();
  }
  if (shell_ != nullptr) shell_->refreshCommands();
}

std::function<void()> RuntimeController::subscribe(std::function<void()> handler) {
  const int id = nextListener_++;
  listeners_[id] = std::move(handler);
  const auto alive = alive_;
  return [this, alive, id] {
    if (*alive) listeners_.erase(id);
  };
}

void RuntimeController::ask(const std::string& name, nlohmann::json params, std::function<void(const contracts::CoreReply&)> done) {
  const auto alive = alive_;
  const auto generation = generation_;
  broker_.request(name, std::move(params), [this, alive, generation, done = std::move(done)](contracts::CoreReply reply) {
    // 控制器已经销毁，或这之间断开过、重新接上过：旧的回复不处理
    if (!*alive || generation != generation_ || shell_ == nullptr) return;
    done(reply);
  });
}

void RuntimeController::report(const std::string& action, const contracts::CoreReply& reply) {
  if (shell_ != nullptr) shell_->notify(failedText(action, reply.error.message), contracts::NotificationLevel::Error);
}

void RuntimeController::perform(const std::string& action, const std::string& request, nlohmann::json params, bool refreshRecentAfter) {
  if (shell_ == nullptr) return;
  ask(request, std::move(params), [this, action, refreshRecentAfter](const contracts::CoreReply& reply) {
    if (!reply.ok) report(action, reply);
    refreshStatus();
    if (refreshRecentAfter) refreshRecent();
    // 操作结束后焦点回到画面，键盘才能继续控制游戏
    if (screenFocus_) screenFocus_();
  });
}

void RuntimeController::togglePower() {
  switch (powerToggleAction(status_, selectedMachine_, machines_)) {
    case PowerToggleAction::PowerOff: perform("断电", "runtime.powerOff", nlohmann::json::object()); break;
    case PowerToggleAction::PowerOn: perform("开机", "runtime.powerOn", {{"resume", true}}); break;
    case PowerToggleAction::Boot: bootMachine(selectedMachine_); break;
    case PowerToggleAction::None: break;
  }
}

void RuntimeController::togglePause() {
  if (!status_ || !status_->machine || !status_->powerOn) return;
  if (status_->running) perform("暂停", "runtime.pause", nlohmann::json::object());
  else perform("继续", "runtime.resume", nlohmann::json::object());
}

void RuntimeController::softReset() {
  if (!availability().softReset) return;
  perform("软复位", "runtime.reset", {{"kind", "soft"}});
}

void RuntimeController::hardReset() {
  if (!availability().hardReset) return;
  perform("硬复位", "runtime.reset", {{"kind", "hard"}});
}

void RuntimeController::ejectCartridge() {
  if (!status_ || !status_->cartridge) return;
  perform("关闭卡带", "cartridges.eject", nlohmann::json::object());
}

void RuntimeController::openCartridge() { perform("打开卡带", "cartridges.open", nlohmann::json::object(), true); }

void RuntimeController::openDroppedCartridge(std::int64_t droppedId) { perform("打开卡带", "cartridges.open", {{"droppedId", droppedId}}, true); }

void RuntimeController::openRecent(std::int64_t id) { perform("打开卡带", "cartridges.openRecent", {{"id", id}}, true); }

void RuntimeController::bootMachine(const std::string& id) {
  if (std::none_of(machines_.begin(), machines_.end(), [&id](const MachineEntry& entry) { return entry.id == id; })) return;
  perform("开机", "machines.boot", {{"machine", id}});
}

void RuntimeController::selectMachine(const std::string& id) {
  if (selectedMachine_ == id) return;
  selectedMachine_ = id;
  shared::ShellLayout layout = state_.get().layout;
  layout.selectedMachine = id;
  state_.updateLayout(layout);
  changed();
}

void RuntimeController::selectAndBootMachine(const std::string& id) {
  // 已选的机型正在运行（通电，运行或暂停）时再点它什么都不做
  if (id == selectedMachine_ && status_ && status_->powerOn) return;
  selectMachine(id);
  bootMachine(id);
}

void RuntimeController::setVolume(int value) {
  volume_ = std::min(100, std::max(0, value));
  changed();
  adjustSound({{"volume", volume_}});
}

void RuntimeController::toggleMute() { adjustSound({{"muted", !muted_}}); }

void RuntimeController::adjustSound(nlohmann::json params) {
  if (shell_ == nullptr) return;
  const auto current = ++soundRequest_;
  ask("output.setVolume", std::move(params), [this, current](const contracts::CoreReply& reply) {
    if (!reply.ok) {
      report("调整音量", reply);
      return;
    }
    if (soundRequest_ != current) return;
    volume_ = reply.result.value("volume", volume_);
    muted_ = reply.result.value("muted", muted_);
    changed();
  });
}

void RuntimeController::toggleFamilyKeyboard() {
  if (shell_ == nullptr) return;
  ask("input.setFamilyKeyboard", {{"connected", !familyKeyboard_}}, [this](const contracts::CoreReply& reply) {
    if (!reply.ok) {
      report("设置 Family BASIC 键盘", reply);
      return;
    }
    familyKeyboard_ = reply.result.value("connected", false);
    changed();
  });
}

void RuntimeController::handleEvent(const std::string& name, const nlohmann::json& data) {
  if (name == "preferences.changed") {
    if (data.is_object() && data.contains("id") && data["id"].is_string() && data.contains("value")) applySetting(data["id"].get<std::string>(), data["value"]);
  } else if (name == "input.familyKeyboardChanged") {
    if (data.is_object() && data.contains("connected") && data["connected"].is_boolean() && data["connected"].get<bool>() != familyKeyboard_) {
      familyKeyboard_ = data["connected"].get<bool>();
      changed();
    }
  } else if (name == "runtime.stateChanged") {
    if (data.is_object() && data.contains("reason") && data["reason"] == "reset") ++resetCount_;
    refreshStatus();
  } else if (name == "cartridges.loaded") {
    refreshStatus();
    refreshRecent();
    // 打开卡带成功后已选机型自动变成经典 FC
    selectMachine(kClassicMachine);
  } else if (name == "machines.changed") {
    refreshMachines();
  }
}

void RuntimeController::handleCoreStatus(const contracts::CoreStatus& status) {
  if (status.state == contracts::CoreState::Running) {
    refreshAll();
    return;
  }
  // 重启过程中先清掉旧状态，就绪后重新读取
  ++statusRequest_;
  status_.reset();
  changed();
}

void RuntimeController::refreshAll() {
  refreshStatus();
  refreshRecent();
  refreshMachines();
  refreshVolume();
  refreshSettings();
  refreshFamilyKeyboard();
}

void RuntimeController::refreshStatus() {
  if (shell_ == nullptr) return;
  const auto current = ++statusRequest_;
  ask("runtime.status", nlohmann::json::object(), [this, current](const contracts::CoreReply& reply) {
    // 核心没有就绪时读不到，等核心状态推送后再读，不报错
    if (!reply.ok || statusRequest_ != current) return;
    status_ = parseRuntimeStatus(reply.result);
    changed();
  });
}

void RuntimeController::refreshRecent() {
  if (shell_ == nullptr) return;
  ask("cartridges.recent", nlohmann::json::object(), [this](const contracts::CoreReply& reply) {
    if (!reply.ok) return;
    recent_ = parseRecent(reply.result);
    changed();
  });
}

void RuntimeController::refreshMachines() {
  if (shell_ == nullptr) return;
  ask("machines.list", nlohmann::json::object(), [this](const contracts::CoreReply& reply) {
    if (!reply.ok) return;
    machines_ = parseMachines(reply.result);
    changed();
  });
}

void RuntimeController::refreshVolume() {
  if (shell_ == nullptr) return;
  const auto current = ++soundRequest_;
  ask("output.setVolume", nlohmann::json::object(), [this, current](const contracts::CoreReply& reply) {
    if (!reply.ok || soundRequest_ != current) return;
    volume_ = reply.result.value("volume", volume_);
    muted_ = reply.result.value("muted", muted_);
    changed();
  });
}

void RuntimeController::refreshFamilyKeyboard() {
  if (shell_ == nullptr) return;
  ask("input.getFamilyKeyboard", nlohmann::json::object(), [this](const contracts::CoreReply& reply) {
    if (!reply.ok) return;
    const bool connected = reply.result.value("connected", false);
    if (connected == familyKeyboard_) return;
    familyKeyboard_ = connected;
    changed();
  });
}

void RuntimeController::refreshSettings() {
  if (shell_ == nullptr) return;
  ask("preferences.list", nlohmann::json::object(), [this](const contracts::CoreReply& reply) {
    if (!reply.ok || !reply.result.is_object() || !reply.result.contains("settings") || !reply.result["settings"].is_array()) return;
    // 读不到的项继续用默认值，所以先回到默认再逐项套用
    screen_ = contracts::ScreenSettings{};
    for (const auto& item : reply.result["settings"]) {
      if (item.is_object() && item.contains("id") && item["id"].is_string() && item.contains("value")) applySetting(item["id"].get<std::string>(), item["value"]);
    }
    changed();
  });
}

void RuntimeController::applySetting(const std::string& id, const nlohmann::json& value) {
  const contracts::ScreenSettings before = screen_;
  if (id == "display.aspect" && value.is_string()) {
    if (value == "4:3") screen_.aspect = shared::ScreenAspect::Ratio4x3;
    else if (value == "8:7") screen_.aspect = shared::ScreenAspect::Ratio8x7;
  } else if (id == "display.scaling" && value.is_string()) {
    if (value == "fit") screen_.scaling = shared::ScreenScaling::Fit;
    else if (value == "integer") screen_.scaling = shared::ScreenScaling::Integer;
  } else if (id == kScreenFilterSetting && value.is_string()) {
    if (value == "smooth") screen_.filter = shared::ScreenFilter::Smooth;
    else if (value == "pixel") screen_.filter = shared::ScreenFilter::Pixel;
  } else if (id == "display.showAllLines" && value.is_boolean()) {
    screen_.showAllLines = value.get<bool>();
  } else if (id == "display.fullScreen" && value.is_string()) {
    if (value == "keep") screen_.fullScreenStretch = false;
    else if (value == "stretch") screen_.fullScreenStretch = true;
  } else if (id == "input.captureMouse" && value.is_boolean()) {
    screen_.captureMouse = value.get<bool>();
  }
  const bool same = before.aspect == screen_.aspect && before.scaling == screen_.scaling && before.filter == screen_.filter && before.showAllLines == screen_.showAllLines && before.fullScreenStretch == screen_.fullScreenStretch &&
                    before.captureMouse == screen_.captureMouse;
  if (!same) changed();
}

}  // namespace ember::win32::logic
