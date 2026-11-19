/*
 * ------------------------------------------------------------------------
 * 名称：gamepad-settings-controller.cpp
 * 说明：手柄设置模块控制器的实现。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-20
 * 备注：无。
 * ------------------------------------------------------------------------
 */

#include "logic/input/gamepad-settings-controller.hpp"

#include <algorithm>

namespace ember::win32::logic {

namespace {

/** 用户可见的文字。 */
constexpr const char* kFailed = "操作失败。";
constexpr const char* kListening = "请按键";
constexpr const char* kWaitKey = "请按下键盘上的键（Esc 取消）";
constexpr const char* kWaitPad = "请按下手柄上的按钮或推动摇杆（再点一次取消）";
constexpr const char* kIdleKey = "点手柄上的键，再按下键盘上的键来设置。";
constexpr const char* kIdlePad = "点手柄上的键，再按下手柄上的按钮或推动摇杆来设置。";
constexpr const char* kNoDevice = "先在上面选择要使用的手柄。";
constexpr const char* kOffline = "这个手柄现在没有连接，接上后要按一下它的任意按钮。";
constexpr const char* kClearNone = "清除绑定";
constexpr const char* kCancelCode = "Escape";

/** 玩家的个数：核心固定两个手柄口。 */
constexpr int kPlayerCount = 2;
/** 学习实体手柄时读手柄的间隔：每个画面帧一次；平时只要刷新手柄列表。 */
constexpr unsigned kLearningPollMilliseconds = 16;
constexpr unsigned kIdlePollMilliseconds = 500;

std::string errorText(const contracts::CoreError& error) { return error.message.empty() ? std::string(kFailed) : error.message; }

}  // namespace

GamepadSettingsController::GamepadSettingsController(contracts::RequestBroker& broker, contracts::CoreGateway& core, contracts::GamepadReader& reader) : broker_(broker), core_(core), reader_(reader) {}

GamepadSettingsController::~GamepadSettingsController() {
  *alive_ = false;
  for (const auto& stop : stops_) stop();
}

std::function<void()> GamepadSettingsController::attach(contracts::ShellApi& shell) {
  const auto stopAll = [this] {
    for (const auto& stop : stops_) stop();
    stops_.clear();
    shell_ = nullptr;
    ++generation_;
    ++loadRequest_;
  };
  stopAll();
  shell_ = &shell;
  stops_.push_back(core_.onEvent([this](const std::string& name, const nlohmann::json& data) { handleEvent(name, data); }));
  stops_.push_back(core_.onStatusChange([this](const contracts::CoreStatus& status) { handleCoreStatus(status); }));
  load();
  return stopAll;
}

void GamepadSettingsController::changed() {
  const auto listeners = listeners_;
  for (const auto& [id, listener] : listeners) {
    if (listeners_.find(id) != listeners_.end()) listener();
  }
}

std::function<void()> GamepadSettingsController::subscribe(std::function<void()> handler) {
  const int id = nextListener_++;
  listeners_[id] = std::move(handler);
  const auto alive = alive_;
  return [this, alive, id] {
    if (*alive) listeners_.erase(id);
  };
}

void GamepadSettingsController::ask(const std::string& name, nlohmann::json params, std::function<void(const contracts::CoreReply&)> done) {
  const auto alive = alive_;
  const auto generation = generation_;
  broker_.request(name, std::move(params), [this, alive, generation, done = std::move(done)](contracts::CoreReply reply) {
    if (!*alive || generation != generation_ || shell_ == nullptr) return;
    done(reply);
  });
}

void GamepadSettingsController::report(const std::string& text) {
  if (shell_ != nullptr) shell_->notify(text, contracts::NotificationLevel::Error);
}

void GamepadSettingsController::load() {
  if (shell_ == nullptr) return;
  const auto current = ++loadRequest_;
  ask("input.getBindings", nlohmann::json::object(), [this, current](const contracts::CoreReply& reply) {
    if (loadRequest_ != current) return;
    auto slots = reply.ok && reply.result.is_object() && reply.result.contains("slots") ? parseSlots(reply.result["slots"]) : std::nullopt;
    if (!slots || static_cast<int>(slots->size()) != kPlayerCount) {
      load_ = contracts::PadSettingsLoad::Failed;
      message_ = errorText(reply.ok ? contracts::CoreError{} : reply.error);
      slots_.clear();
      players_.clear();
    } else {
      load_ = contracts::PadSettingsLoad::Ready;
      message_.clear();
      slots_ = std::move(*slots);
      if (players_.size() != slots_.size()) players_.assign(slots_.size(), PlayerState{});
    }
    changed();
  });
}

void GamepadSettingsController::handleEvent(const std::string& name, const nlohmann::json& data) {
  if (name != "input.bindingsChanged" || !data.is_object() || !data.contains("slots")) return;
  auto slots = parseSlots(data["slots"]);
  if (!slots || static_cast<int>(slots->size()) != kPlayerCount) return;
  load_ = contracts::PadSettingsLoad::Ready;
  message_.clear();
  slots_ = std::move(*slots);
  if (players_.size() != slots_.size()) players_.assign(slots_.size(), PlayerState{});
  changed();
}

void GamepadSettingsController::handleCoreStatus(const contracts::CoreStatus& status) {
  if (status.state == contracts::CoreState::Running) {
    load();
    return;
  }
  ++loadRequest_;
  load_ = contracts::PadSettingsLoad::Loading;
  message_.clear();
  slots_.clear();
  waiting_.reset();
  baseline_.reset();
  changed();
}

void GamepadSettingsController::apply(const std::vector<PadSlot>& slots) {
  if (shell_ == nullptr) return;
  ask("input.setBindings", {{"slots", slotsToJson(slots)}}, [this](const contracts::CoreReply& reply) {
    if (!reply.ok) {
      report(errorText(reply.error));
    } else if (reply.result.is_object() && reply.result.contains("slots")) {
      if (auto next = parseSlots(reply.result["slots"])) slots_ = std::move(*next);
    }
    changed();
  });
}

bool GamepadSettingsController::isConnected(const PadDevice& device) const { return std::find(devices_.begin(), devices_.end(), device) != devices_.end(); }

std::optional<std::string> GamepadSettingsController::boundText(int player, const std::string& button) const {
  if (!validPlayer(player)) return std::nullopt;
  const PadSlot& slot = slots_[static_cast<std::size_t>(player)];
  if (players_[static_cast<std::size_t>(player)].source == contracts::PadInputSource::Keyboard) {
    const auto code = keyOf(slot, button);
    return code ? std::optional<std::string>(keyLabel(*code)) : std::nullopt;
  }
  const auto binding = bindingOf(slot.gamepad, button);
  return binding ? std::optional<std::string>(sourceLabel(PadSource{binding->kind, binding->index, binding->direction})) : std::nullopt;
}

contracts::PadPlayerView GamepadSettingsController::buildPlayer(int player) const {
  const PadSlot& slot = slots_[static_cast<std::size_t>(player)];
  const PlayerState& state = players_[static_cast<std::size_t>(player)];
  const bool keyboard = state.source == contracts::PadInputSource::Keyboard;
  std::optional<PadDevice> assigned;
  if (slot.gamepad) assigned = PadDevice{slot.gamepad->id, slot.gamepad->occurrence};
  const bool connected = assigned && isConnected(*assigned);

  contracts::PadPlayerView view;
  view.source = state.source;
  view.keysEnabled = keyboard || connected;
  view.showDevice = !keyboard;
  std::vector<PadDevice> choices = devices_;
  if (assigned && !connected) choices.push_back(*assigned);
  for (const PadDevice& device : choices) view.devices.push_back({deviceKey(device), deviceLabel(device, isConnected(device))});
  view.assignedDevice = assigned ? deviceKey(*assigned) : std::string();

  std::string learning;
  if (waiting_ && waiting_->player == player && waiting_->pad == !keyboard) learning = waiting_->button;
  const std::string waitText = keyboard ? kWaitKey : kWaitPad;
  for (const PadButtonInfo& info : kPadButtons) {
    const std::string id = info.id;
    const auto bound = boundText(player, id);
    const std::string current = bound ? *bound : unboundText();
    const bool isLearning = learning == id;
    contracts::PadKeyView key;
    key.id = id;
    key.role = info.label;
    key.current = isLearning ? kListening : current;
    key.unbound = !bound && !isLearning;
    if (isLearning) key.tip = waitText;
    else key.tip = std::string(keyboard ? "绑定键盘按键：" : "绑定手柄按钮：") + info.label + "（当前：" + current + "）";
    key.selected = state.selected == id;
    key.learning = isLearning;
    view.keys.push_back(std::move(key));
  }

  const std::string selectedName = state.selected.empty() ? std::string() : buttonLabel(state.selected);
  view.message = keyboard ? kIdleKey : kIdlePad;
  if (!keyboard && !assigned) view.message = kNoDevice;
  else if (!keyboard && !connected) view.message = kOffline;
  else if (!learning.empty()) view.message = "设置“" + buttonLabel(learning) + "”：" + waitText;
  else if (!state.selected.empty()) view.message = selectedName + "：" + boundText(player, state.selected).value_or(unboundText());

  view.canClear = !state.selected.empty() && boundText(player, state.selected).has_value();
  view.clearLabel = state.selected.empty() ? std::string(kClearNone) : (keyboard ? "清除键盘按键：" : "清除手柄按钮：") + selectedName;
  return view;
}

contracts::PadSettingsView GamepadSettingsController::view() const {
  contracts::PadSettingsView result;
  result.load = load_;
  result.message = message_;
  result.activePlayer = activePlayer_;
  if (load_ == contracts::PadSettingsLoad::Ready) {
    for (int player = 0; player < static_cast<int>(slots_.size()); ++player) result.players.push_back(buildPlayer(player));
  }
  return result;
}

void GamepadSettingsController::cancelLearning() {
  if (!waiting_) return;
  waiting_.reset();
  baseline_.reset();
  changed();
}

void GamepadSettingsController::selectPlayer(int player) {
  if (!validPlayer(player)) return;
  waiting_.reset();
  baseline_.reset();
  activePlayer_ = player;
  changed();
}

void GamepadSettingsController::setSource(int player, contracts::PadInputSource source) {
  if (!validPlayer(player)) return;
  waiting_.reset();
  baseline_.reset();
  PlayerState& state = players_[static_cast<std::size_t>(player)];
  state.selected.clear();
  state.source = source;
  changed();
}

void GamepadSettingsController::pickKey(int player, const std::string& key) {
  if (!validPlayer(player)) return;
  PlayerState& state = players_[static_cast<std::size_t>(player)];
  const bool keyboard = state.source == contracts::PadInputSource::Keyboard;
  // 正在学习的同一个键再点一次：取消
  if (waiting_ && waiting_->player == player && waiting_->pad == !keyboard && waiting_->button == key) {
    cancelLearning();
    return;
  }
  const PadSlot& slot = slots_[static_cast<std::size_t>(player)];
  state.selected = key;
  baseline_.reset();
  if (keyboard) {
    waiting_ = Waiting{false, player, key, {}};
  } else if (slot.gamepad && isConnected(PadDevice{slot.gamepad->id, slot.gamepad->occurrence})) {
    waiting_ = Waiting{true, player, key, PadDevice{slot.gamepad->id, slot.gamepad->occurrence}};
  }
  changed();
}

void GamepadSettingsController::clearSelected(int player) {
  if (!validPlayer(player)) return;
  PlayerState& state = players_[static_cast<std::size_t>(player)];
  if (state.selected.empty()) return;
  waiting_.reset();
  baseline_.reset();
  const std::size_t index = static_cast<std::size_t>(player);
  apply(state.source == contracts::PadInputSource::Keyboard ? clearKey(slots_, index, state.selected) : clearGamepad(slots_, index, state.selected));
  changed();
}

void GamepadSettingsController::assignDevice(int player, const std::string& deviceValue) {
  if (!validPlayer(player)) return;
  waiting_.reset();
  baseline_.reset();
  apply(logic::assignDevice(slots_, static_cast<std::size_t>(player), deviceValue.empty() ? std::nullopt : parseDeviceKey(deviceValue)));
  changed();
}

void GamepadSettingsController::reset() {
  waiting_.reset();
  baseline_.reset();
  if (shell_ == nullptr) return;
  ask("input.resetBindings", nlohmann::json::object(), [this](const contracts::CoreReply& reply) {
    if (!reply.ok) {
      report(errorText(reply.error));
    } else if (reply.result.is_object() && reply.result.contains("slots")) {
      if (auto next = parseSlots(reply.result["slots"])) slots_ = std::move(*next);
    }
    changed();
  });
  changed();
}

bool GamepadSettingsController::keyPressed(const std::string& code) {
  if (!waiting_ || waiting_->pad) return false;
  // 学习期间的按键不再触发界面里的其他操作
  if (code == kCancelCode) {
    cancelLearning();
    return true;
  }
  if (!isCapturableCode(code)) return true;
  const Waiting learned = *waiting_;
  waiting_.reset();
  baseline_.reset();
  apply(bindKey(slots_, static_cast<std::size_t>(learned.player), learned.button, code));
  changed();
  return true;
}

void GamepadSettingsController::poll() {
  samples_ = reader_.read();
  auto next = listDevices(samples_);
  const bool devicesChanged = next != devices_;
  devices_ = std::move(next);
  if (waiting_ && waiting_->pad) {
    if (const contracts::GamepadSample* pad = findPad(samples_, waiting_->device)) {
      if (!baseline_) {
        baseline_ = baselineOf(*pad);
      } else if (const auto source = detectSource(*baseline_, *pad)) {
        const Waiting learned = *waiting_;
        waiting_.reset();
        baseline_.reset();
        apply(bindGamepad(slots_, static_cast<std::size_t>(learned.player), learned.button, *source));
        changed();
        return;
      }
    }
  }
  if (devicesChanged) changed();
}

unsigned GamepadSettingsController::pollIntervalMs() const { return waiting_ && waiting_->pad ? kLearningPollMilliseconds : kIdlePollMilliseconds; }

}  // namespace ember::win32::logic
