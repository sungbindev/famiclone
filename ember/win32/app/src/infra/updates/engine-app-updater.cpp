/*
 * ------------------------------------------------------------------------
 * 名称：engine-app-updater.cpp
 * 说明：应用升级服务的实现（基于升级引擎）。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-21
 * 备注：无。
 * ------------------------------------------------------------------------
 */

#include "infra/updates/engine-app-updater.hpp"

namespace ember::win32::infra {

namespace {

constexpr const char* kCheckTimeout = "检查更新超时，请稍后再试";

}  // namespace

EngineAppUpdater::EngineAppUpdater(contracts::UpdateEngine& engine, platform::UiThread& ui, unsigned checkTimeoutMilliseconds)
    : engine_(engine), ui_(ui), checkTimeout_(checkTimeoutMilliseconds), current_{engine.supported() ? "idle" : "unsupported", std::nullopt, std::nullopt} {}

EngineAppUpdater::~EngineAppUpdater() {
  *alive_ = false;
  if (timer_ != 0) ui_.cancelDelayed(timer_);
}

contracts::UpdateStatus EngineAppUpdater::check() {
  if (current_.state != "idle" && current_.state != "upToDate" && current_.state != "error") return current_;
  update({"checking", std::nullopt, std::nullopt});
  const std::weak_ptr<bool> weak = alive_;
  timer_ = ui_.postDelayed(checkTimeout_, [this, weak] {
    const auto alive = weak.lock();
    if (!alive || !*alive) return;
    timer_ = 0;
    if (current_.state == "checking") update({"error", std::nullopt, std::string(kCheckTimeout)});
  });

  contracts::UpdateEngineHandlers handlers;
  // 引擎的回调在界面线程，但引擎可能比本对象活得久，所以用存活标记保护
  handlers.available = [this, weak] {
    const auto alive = weak.lock();
    if (alive && *alive && current_.state == "checking") update({"downloading", std::nullopt, std::nullopt});
  };
  handlers.notAvailable = [this, weak] {
    const auto alive = weak.lock();
    if (alive && *alive && current_.state == "checking") update({"upToDate", std::nullopt, std::nullopt});
  };
  handlers.downloaded = [this, weak](const std::string& version) {
    const auto alive = weak.lock();
    if (alive && *alive && (current_.state == "checking" || current_.state == "downloading")) update({"ready", version, std::nullopt});
  };
  handlers.failed = [this, weak](const std::string& message) {
    const auto alive = weak.lock();
    if (alive && *alive && (current_.state == "checking" || current_.state == "downloading")) update({"error", std::nullopt, message});
  };
  engine_.check(std::move(handlers));
  return current_;
}

bool EngineAppUpdater::install() {
  if (current_.state != "ready") return false;
  return engine_.restart();
}

std::function<void()> EngineAppUpdater::subscribe(std::function<void(const contracts::UpdateStatus&)> handler) {
  const int id = nextListener_++;
  listeners_[id] = std::move(handler);
  const std::weak_ptr<bool> weak = alive_;
  return [this, weak, id] {
    const auto alive = weak.lock();
    if (alive && *alive) listeners_.erase(id);
  };
}

void EngineAppUpdater::update(contracts::UpdateStatus next) {
  if (next.state != "checking" && timer_ != 0) {
    ui_.cancelDelayed(timer_);
    timer_ = 0;
  }
  current_ = std::move(next);
  const auto copy = listeners_;
  for (const auto& [id, listener] : copy) {
    if (listeners_.find(id) != listeners_.end()) listener(current_);
  }
}

}  // namespace ember::win32::infra
