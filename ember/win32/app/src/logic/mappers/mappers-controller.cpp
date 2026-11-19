/*
 * ------------------------------------------------------------------------
 * 名称：mappers-controller.cpp
 * 说明：mapper 插件模块控制器的实现。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-21
 * 备注：无。
 * ------------------------------------------------------------------------
 */

#include "logic/mappers/mappers-controller.hpp"

#include <algorithm>

namespace ember::win32::logic {

namespace {

/** 用户可见的文字。 */
constexpr const char* kListFailed = "读取插件列表失败";
constexpr const char* kDetailFailed = "读取插件详情失败";
constexpr const char* kInstallFailed = "安装插件失败";
constexpr const char* kUninstallFailed = "卸载插件失败";
constexpr const char* kToggleFailed = "修改插件的启用状态失败";
constexpr const char* kUninstallTitle = "卸载插件";

}  // namespace

MappersController::MappersController(contracts::RequestBroker& broker, contracts::CoreGateway& core, contracts::Confirmation& confirmation) : broker_(broker), core_(core), confirmation_(confirmation) {}

MappersController::~MappersController() {
  *alive_ = false;
  for (const auto& stop : stops_) stop();
}

std::function<void()> MappersController::attach(contracts::ShellApi& shell) {
  const auto stopAll = [this] {
    for (const auto& stop : stops_) stop();
    stops_.clear();
    shell_ = nullptr;
    ++generation_;
    ++loadRequest_;
    ++detailRequest_;
  };
  stopAll();
  shell_ = &shell;
  stops_.push_back(core_.onEvent([this](const std::string& name, const nlohmann::json&) { handleEvent(name); }));
  stops_.push_back(core_.onStatusChange([this](const contracts::CoreStatus& status) { handleCoreStatus(status); }));
  load();
  return stopAll;
}

const MapperSummary* MappersController::find(const std::string& id) const {
  const auto found = std::find_if(plugins_.begin(), plugins_.end(), [&id](const MapperSummary& plugin) { return plugin.id == id; });
  return found == plugins_.end() ? nullptr : &*found;
}

contracts::MappersView MappersController::view() const {
  contracts::MappersView result;
  result.load = load_;
  result.message = message_;
  result.search = search_;
  result.source = source_;
  if (load_ != contracts::MappersLoad::Ready) return result;
  result.stats = statsText(plugins_);
  for (const MapperSummary& plugin : filterMappers(plugins_, source_, search_)) result.rows.push_back(toRow(plugin));
  const MapperSummary* chosen = chosenId_.empty() ? nullptr : find(chosenId_);
  if (chosen == nullptr) return result;
  result.selectedId = chosen->id;
  result.selected = toRow(*chosen);
  if (!detail_ || detail_->id != chosen->id) {
    result.detailLoad = contracts::MapperDetailLoad::Loading;
  } else if (detail_->detail) {
    result.detailLoad = contracts::MapperDetailLoad::Ready;
    result.facts = detailFacts(*detail_->detail);
  } else {
    result.detailLoad = contracts::MapperDetailLoad::Failed;
    result.detailMessage = detail_->message;
  }
  return result;
}

void MappersController::changed() {
  const auto listeners = listeners_;
  for (const auto& [id, listener] : listeners) {
    if (listeners_.find(id) != listeners_.end()) listener();
  }
}

std::function<void()> MappersController::subscribe(std::function<void()> handler) {
  const int id = nextListener_++;
  listeners_[id] = std::move(handler);
  const auto alive = alive_;
  return [this, alive, id] {
    if (*alive) listeners_.erase(id);
  };
}

void MappersController::ask(const std::string& name, nlohmann::json params, std::function<void(const contracts::CoreReply&)> done) {
  const auto alive = alive_;
  const auto generation = generation_;
  broker_.request(name, std::move(params), [this, alive, generation, done = std::move(done)](contracts::CoreReply reply) {
    if (!*alive || generation != generation_ || shell_ == nullptr) return;
    done(reply);
  });
}

void MappersController::report(const std::string& text) {
  if (shell_ != nullptr) shell_->notify(text, contracts::NotificationLevel::Error);
}

void MappersController::notifySuccess(const std::string& text) {
  if (shell_ != nullptr) shell_->notify(text, contracts::NotificationLevel::Success);
}

void MappersController::load() {
  if (shell_ == nullptr) return;
  const auto current = ++loadRequest_;
  ask("mappers.list", nlohmann::json::object(), [this, current](const contracts::CoreReply& reply) {
    if (loadRequest_ != current) return;
    auto plugins = reply.ok ? parseMapperList(reply.result) : std::nullopt;
    if (!plugins) {
      load_ = contracts::MappersLoad::Failed;
      message_ = mapperErrorText(reply.ok ? contracts::CoreError{} : reply.error, kListFailed);
      plugins_.clear();
    } else {
      load_ = contracts::MappersLoad::Ready;
      message_.clear();
      plugins_ = std::move(*plugins);
    }
    changed();
  });
}

void MappersController::loadDetail() {
  if (shell_ == nullptr || chosenId_.empty()) return;
  const auto current = ++detailRequest_;
  const std::string id = chosenId_;
  ask("mappers.info", {{"id", id}}, [this, current, id](const contracts::CoreReply& reply) {
    if (detailRequest_ != current) return;
    auto detail = reply.ok ? parseMapperDetail(reply.result) : std::nullopt;
    detail_ = DetailResult{id, detail, detail ? std::string() : mapperErrorText(reply.ok ? contracts::CoreError{} : reply.error, kDetailFailed)};
    changed();
  });
}

void MappersController::handleEvent(const std::string& name) {
  if (name != "mappers.changed") return;
  load();
  loadDetail();
}

void MappersController::handleCoreStatus(const contracts::CoreStatus& status) {
  if (status.state == contracts::CoreState::Running) {
    load();
    loadDetail();
    return;
  }
  ++loadRequest_;
  ++detailRequest_;
  load_ = contracts::MappersLoad::Loading;
  message_.clear();
  changed();
}

void MappersController::put(const MapperSummary& plugin) {
  const auto found = std::find_if(plugins_.begin(), plugins_.end(), [&plugin](const MapperSummary& item) { return item.id == plugin.id; });
  if (found != plugins_.end()) {
    *found = plugin;
    return;
  }
  plugins_.push_back(plugin);
  std::sort(plugins_.begin(), plugins_.end(), [](const MapperSummary& a, const MapperSummary& b) { return a.id < b.id; });
}

void MappersController::setSearch(const std::string& text) {
  if (search_ == text) return;
  search_ = text;
  changed();
}

void MappersController::setSource(const std::string& source) {
  if (source_ == source) return;
  source_ = source;
  changed();
}

void MappersController::toggleSelect(const std::string& id) {
  chosenId_ = chosenId_ == id ? std::string() : id;
  if (!chosenId_.empty()) loadDetail();
  changed();
}

void MappersController::install() {
  if (shell_ == nullptr) return;
  ask("mappers.openPackage", nlohmann::json::object(), [this](const contracts::CoreReply& reply) {
    if (!reply.ok) {
      report(mapperErrorText(reply.error, kInstallFailed));
      return;
    }
    if (!reply.result.is_object() || reply.result.value("canceled", false)) return;
    const auto plugin = reply.result.contains("plugin") ? parseMapperSummary(reply.result["plugin"]) : std::nullopt;
    if (!plugin) return;
    put(*plugin);
    chosenId_ = plugin->id;
    loadDetail();
    changed();
    notifySuccess("已安装插件：" + plugin->name + " " + plugin->version);
  });
}

void MappersController::uninstall(const std::string& id) {
  if (shell_ == nullptr) return;
  const MapperSummary* plugin = find(id);
  if (plugin == nullptr) return;
  const std::string name = plugin->name;
  if (!confirmation_.ask(kUninstallTitle, "卸载“" + name + "”？卸载后使用它的卡带将不能运行，可以重新安装插件包。")) return;
  ask("mappers.uninstall", {{"id", id}}, [this, id, name](const contracts::CoreReply& reply) {
    if (!reply.ok) {
      report(mapperErrorText(reply.error, kUninstallFailed));
      return;
    }
    plugins_.erase(std::remove_if(plugins_.begin(), plugins_.end(), [&id](const MapperSummary& item) { return item.id == id; }), plugins_.end());
    if (chosenId_ == id) chosenId_.clear();
    changed();
    notifySuccess("已卸载插件：" + name);
  });
}

void MappersController::setEnabled(const std::string& id, bool enabled) {
  if (shell_ == nullptr) return;
  ask("mappers.setEnabled", {{"id", id}, {"enabled", enabled}}, [this](const contracts::CoreReply& reply) {
    if (!reply.ok) {
      report(mapperErrorText(reply.error, kToggleFailed));
      return;
    }
    const auto plugin = reply.result.is_object() && reply.result.contains("plugin") ? parseMapperSummary(reply.result["plugin"]) : std::nullopt;
    if (!plugin) return;
    put(*plugin);
    changed();
  });
}

}  // namespace ember::win32::logic
