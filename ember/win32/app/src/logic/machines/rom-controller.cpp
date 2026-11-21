/*
 * ------------------------------------------------------------------------
 * 名称：rom-controller.cpp
 * 说明：“机型与 ROM”对话框控制器的实现。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-21
 * 备注：无。
 * ------------------------------------------------------------------------
 */

#include "logic/machines/rom-controller.hpp"

namespace ember::win32::logic {

RomController::RomController(contracts::RequestBroker& broker, contracts::CoreGateway& core) : broker_(broker), core_(core) {}

RomController::~RomController() {
  *alive_ = false;
  for (const auto& stop : stops_) stop();
}

std::function<void()> RomController::attach(contracts::ShellApi& shell) {
  const auto stopAll = [this] {
    for (const auto& stop : stops_) stop();
    stops_.clear();
    shell_ = nullptr;
    ++generation_;
    ++loadRequest_;
  };
  stopAll();
  shell_ = &shell;
  stops_.push_back(core_.onEvent([this](const std::string& name, const nlohmann::json&) { handleEvent(name); }));
  stops_.push_back(core_.onStatusChange([this](const contracts::CoreStatus& status) { handleCoreStatus(status); }));
  load();
  return stopAll;
}

contracts::RomView RomController::view() const {
  contracts::RomView result;
  result.load = load_;
  result.message = message_;
  if (load_ == contracts::MachinesLoad::Ready) result.rows = buildRomRows(machines_);
  return result;
}

void RomController::changed() {
  const auto listeners = listeners_;
  for (const auto& [id, listener] : listeners) {
    if (listeners_.find(id) != listeners_.end()) listener();
  }
}

std::function<void()> RomController::subscribe(std::function<void()> handler) {
  const int id = nextListener_++;
  listeners_[id] = std::move(handler);
  const auto alive = alive_;
  return [this, alive, id] {
    if (*alive) listeners_.erase(id);
  };
}

void RomController::ask(const std::string& name, nlohmann::json params, std::function<void(const contracts::CoreReply&)> done) {
  const auto alive = alive_;
  const auto generation = generation_;
  broker_.request(name, std::move(params), [this, alive, generation, done = std::move(done)](contracts::CoreReply reply) {
    if (!*alive || generation != generation_ || shell_ == nullptr) return;
    done(reply);
  });
}

void RomController::report(const std::string& text) {
  if (shell_ != nullptr) shell_->notify(text, contracts::NotificationLevel::Error);
}

void RomController::notifySuccess(const std::string& text) {
  if (shell_ != nullptr) shell_->notify(text, contracts::NotificationLevel::Success);
}

const Machine* RomController::find(const std::string& id) const {
  for (const Machine& machine : machines_) {
    if (machine.id == id) return &machine;
  }
  return nullptr;
}

void RomController::load() {
  if (shell_ == nullptr) return;
  const auto current = ++loadRequest_;
  ask("machines.list", nlohmann::json::object(), [this, current](const contracts::CoreReply& reply) {
    if (loadRequest_ != current) return;
    auto machines = reply.ok ? parseRomMachines(reply.result) : std::nullopt;
    if (!machines) {
      load_ = contracts::MachinesLoad::Failed;
      message_ = machineErrorText(reply.ok ? contracts::CoreError{} : reply.error);
      machines_.clear();
    } else {
      load_ = contracts::MachinesLoad::Ready;
      message_.clear();
      machines_ = std::move(*machines);
    }
    changed();
  });
}

void RomController::handleEvent(const std::string& name) {
  if (name == "machines.changed") load();
}

void RomController::handleCoreStatus(const contracts::CoreStatus& status) {
  if (status.state == contracts::CoreState::Running) {
    load();
    return;
  }
  ++loadRequest_;
  load_ = contracts::MachinesLoad::Loading;
  message_.clear();
  machines_.clear();
  changed();
}

void RomController::choose(const std::string& machine) {
  if (shell_ == nullptr) return;
  const Machine* known = find(machine);
  const std::string name = known != nullptr ? known->name : machine;
  const bool hadRom = known != nullptr && known->hasRom;
  ask("machines.openRom", {{"machine", machine}}, [this, name, hadRom](const contracts::CoreReply& reply) {
    if (!reply.ok) {
      report(machineErrorText(reply.error));
      return;
    }
    if (!reply.result.is_object() || reply.result.value("canceled", false) || !reply.result.contains("machine")) return;
    const auto updated = parseRomMachine(reply.result["machine"]);
    if (!updated || !updated->hasRom) return;
    notifySuccess(std::string(hadRom ? "已替换 " : "已添加 ") + name + " 的 ROM：" + updated->romFileName);
  });
}

void RomController::remove(const std::string& machine) {
  if (shell_ == nullptr) return;
  const Machine* known = find(machine);
  const std::string name = known != nullptr ? known->name : machine;
  ask("machines.removeRom", {{"machine", machine}}, [this, name](const contracts::CoreReply& reply) {
    if (!reply.ok) {
      report(machineErrorText(reply.error));
      return;
    }
    notifySuccess("已移除 " + name + " 的 ROM");
  });
}

void RomController::openDirectory() {
  if (shell_ == nullptr) return;
  ask("machines.openRomDirectory", nlohmann::json::object(), [this](const contracts::CoreReply& reply) {
    if (!reply.ok) report(machineErrorText(reply.error));
  });
}

}  // namespace ember::win32::logic
