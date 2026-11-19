/*
 * ------------------------------------------------------------------------
 * 名称：preferences-controller.cpp
 * 说明：首选项模块控制器的实现。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-20
 * 备注：无。
 * ------------------------------------------------------------------------
 */

#include "logic/preferences/preferences-controller.hpp"

namespace ember::win32::logic {

PreferencesController::PreferencesController(contracts::RequestBroker& broker, contracts::CoreGateway& core) : broker_(broker), core_(core) {}

PreferencesController::~PreferencesController() {
  *alive_ = false;
  for (const auto& stop : stops_) stop();
}

std::function<void()> PreferencesController::attach(contracts::ShellApi& shell) {
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

contracts::PreferencesView PreferencesController::view() const {
  contracts::PreferencesView result;
  result.load = load_;
  result.message = message_;
  if (listing_) result.groups = buildGroupViews(*listing_);
  return result;
}

void PreferencesController::changed() {
  // 回调里可能取消订阅，先复制一份再逐个调用
  const auto listeners = listeners_;
  for (const auto& [id, listener] : listeners) {
    if (listeners_.find(id) != listeners_.end()) listener();
  }
}

std::function<void()> PreferencesController::subscribe(std::function<void()> handler) {
  const int id = nextListener_++;
  listeners_[id] = std::move(handler);
  const auto alive = alive_;
  return [this, alive, id] {
    if (*alive) listeners_.erase(id);
  };
}

void PreferencesController::ask(const std::string& name, nlohmann::json params, std::function<void(const contracts::CoreReply&)> done) {
  const auto alive = alive_;
  const auto generation = generation_;
  broker_.request(name, std::move(params), [this, alive, generation, done = std::move(done)](contracts::CoreReply reply) {
    // 控制器已经销毁，或这之间断开过、重新接上过：旧的回复不处理
    if (!*alive || generation != generation_ || shell_ == nullptr) return;
    done(reply);
  });
}

void PreferencesController::report(const std::string& text) {
  if (shell_ != nullptr) shell_->notify(text, contracts::NotificationLevel::Error);
}

const Setting* PreferencesController::find(const std::string& id) const {
  if (!listing_) return nullptr;
  for (const Setting& setting : listing_->settings) {
    if (setting.id == id) return &setting;
  }
  return nullptr;
}

void PreferencesController::load() {
  if (shell_ == nullptr) return;
  const auto current = ++loadRequest_;
  ask("preferences.list", nlohmann::json::object(), [this, current](const contracts::CoreReply& reply) {
    if (loadRequest_ != current) return;
    if (!reply.ok) {
      load_ = contracts::PreferencesLoad::Failed;
      message_ = errorText(reply.error);
      listing_.reset();
      changed();
      return;
    }
    auto listing = parseSettingListing(reply.result);
    if (!listing) {
      load_ = contracts::PreferencesLoad::Failed;
      message_ = errorText({});
      listing_.reset();
      changed();
      return;
    }
    load_ = contracts::PreferencesLoad::Ready;
    message_.clear();
    listing_ = std::move(listing);
    changed();
  });
}

void PreferencesController::handleEvent(const std::string& name, const nlohmann::json& data) {
  if (name != "preferences.changed" || !listing_ || !data.is_object()) return;
  const auto id = data.find("id");
  const auto value = data.find("value");
  if (id == data.end() || !id->is_string() || value == data.end()) return;
  applyValue(listing_->settings, id->get<std::string>(), *value);
  changed();
}

void PreferencesController::handleCoreStatus(const contracts::CoreStatus& status) {
  if (status.state == contracts::CoreState::Running) {
    load();
    return;
  }
  ++loadRequest_;
  load_ = contracts::PreferencesLoad::Loading;
  message_.clear();
  listing_.reset();
  changed();
}

void PreferencesController::save(const std::string& id, nlohmann::json value) {
  if (shell_ == nullptr) return;
  ask("preferences.set", {{"id", id}, {"value", std::move(value)}}, [this](const contracts::CoreReply& reply) {
    if (!reply.ok) {
      report(errorText(reply.error));
    } else if (listing_ && reply.result.is_object() && reply.result.contains("id") && reply.result["id"].is_string() && reply.result.contains("value")) {
      applyValue(listing_->settings, reply.result["id"].get<std::string>(), reply.result["value"]);
    }
    changed();
  });
}

void PreferencesController::setChoice(const std::string& id, const std::string& value) { save(id, value); }

void PreferencesController::setSwitch(const std::string& id, bool value) { save(id, value); }

std::string PreferencesController::submitText(const std::string& id, const std::string& text) {
  const Setting* setting = find(id);
  if (setting == nullptr) return {};
  nlohmann::json next = text;
  if (setting->kind == SettingKind::Integer) {
    const IntegerParse parsed = parseInteger(text, setting->minimum, setting->maximum);
    if (!parsed.ok) return parsed.message;
    next = parsed.value;
  }
  if (next == setting->value) {
    // 没有要保存的，输入框里的文字（如多余的空格）回到当前取值
    changed();
    return {};
  }
  save(id, std::move(next));
  return {};
}

void PreferencesController::reset(const std::string& id) {
  if (shell_ == nullptr) return;
  ask("preferences.reset", {{"id", id}}, [this](const contracts::CoreReply& reply) {
    if (!reply.ok) {
      report(errorText(reply.error));
    } else if (listing_ && reply.result.is_object() && reply.result.contains("id") && reply.result["id"].is_string() && reply.result.contains("value")) {
      applyValue(listing_->settings, reply.result["id"].get<std::string>(), reply.result["value"]);
    }
    changed();
  });
}

void PreferencesController::pickDirectory(const std::string& id) {
  if (shell_ == nullptr) return;
  ask("preferences.pickDirectory", nlohmann::json::object(), [this, id](const contracts::CoreReply& reply) {
    if (!reply.ok) {
      report(errorText(reply.error));
      return;
    }
    const bool canceled = reply.result.is_object() && reply.result.value("canceled", false);
    const auto path = reply.result.is_object() ? reply.result.find("path") : reply.result.end();
    if (canceled || path == reply.result.end() || !path->is_string()) return;
    save(id, path->get<std::string>());
  });
}

}  // namespace ember::win32::logic
