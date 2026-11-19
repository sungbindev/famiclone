/*
 * ------------------------------------------------------------------------
 * 名称：fakes.hpp
 * 说明：测试用的假实现：核心网关、文件选择窗口、可打开的目录、最近打开的卡带服务、错误日志、升级与帮助。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-19
 * 备注：假网关记录收到的请求，按请求名给出事先设好的回复（没设时回复空对象），回复在 request 里同步给出。
 * ------------------------------------------------------------------------
 */

#pragma once

#include <deque>
#include <functional>
#include <map>
#include <string>
#include <utility>
#include <vector>

#include "contracts/cartridges/recent-cartridges-service.hpp"
#include "contracts/core-gateway/core-gateway.hpp"
#include "contracts/logging/error-logger.hpp"
#include "contracts/shell/file-dialog.hpp"
#include "contracts/shell/openable-directory.hpp"
#include "contracts/system/app-updater.hpp"
#include "contracts/system/help-viewer.hpp"

namespace ember::win32::test {

/** 假核心网关。 */
class FakeCoreGateway : public contracts::CoreGateway {
 public:
  /** 收到的一次请求。 */
  struct Call {
    std::string name;
    nlohmann::json params;
  };

  /** 请求名对应的回复；没设时回复空对象。 */
  std::map<std::string, std::function<contracts::CoreReply(const nlohmann::json& params)>> replies;
  std::vector<Call> calls;
  std::vector<nlohmann::json> inputs;
  /** 当前状态，setState 会通知订阅者。 */
  contracts::CoreStatus current;

  void start() override {}
  void stop() override {}
  [[nodiscard]] contracts::CoreStatus status() const override { return current; }
  void request(const std::string& name, nlohmann::json params, contracts::CoreReplyHandler done) override {
    calls.push_back({name, params});
    const auto found = replies.find(name);
    done(found == replies.end() ? contracts::CoreReply::success(nlohmann::json::object()) : found->second(params));
  }
  void sendInput(const nlohmann::json& message) override { inputs.push_back(message); }
  [[nodiscard]] std::function<void()> onEvent(contracts::CoreEventHandler handler) override {
    const int handle = nextHandle_++;
    eventHandlers_[handle] = std::move(handler);
    return [this, handle] { eventHandlers_.erase(handle); };
  }
  [[nodiscard]] std::function<void()> onStatusChange(contracts::CoreStatusHandler handler) override {
    const int handle = nextHandle_++;
    statusHandlers_[handle] = std::move(handler);
    return [this, handle] { statusHandlers_.erase(handle); };
  }

  /** 改变状态并通知订阅者。 */
  void setState(contracts::CoreState state) {
    current.state = state;
    const auto handlers = statusHandlers_;
    for (const auto& [handle, handler] : handlers) handler(current);
  }
  /** 发一个核心事件给订阅者。 */
  void emitEvent(const std::string& name, const nlohmann::json& data) {
    const auto handlers = eventHandlers_;
    for (const auto& [handle, handler] : handlers) handler(name, data);
  }
  [[nodiscard]] std::size_t subscriberCount() const { return eventHandlers_.size() + statusHandlers_.size(); }

 private:
  int nextHandle_{1};
  std::map<int, contracts::CoreEventHandler> eventHandlers_;
  std::map<int, contracts::CoreStatusHandler> statusHandlers_;
};

/** 假文件选择窗口：按事先设好的队列依次给出结果，并记下每次的参数。 */
class FakeFileDialog : public contracts::FileDialog {
 public:
  std::deque<std::optional<std::string>> results;
  std::vector<contracts::FileDialogOptions> fileCalls;
  std::vector<contracts::FileDialogOptions> saveCalls;
  std::vector<contracts::DirectoryDialogOptions> directoryCalls;

  [[nodiscard]] std::optional<std::string> pickFile(const contracts::FileDialogOptions& options) override {
    fileCalls.push_back(options);
    return next();
  }
  [[nodiscard]] std::optional<std::string> pickDirectory(const contracts::DirectoryDialogOptions& options) override {
    directoryCalls.push_back(options);
    return next();
  }
  [[nodiscard]] std::optional<std::string> pickSaveFile(const contracts::FileDialogOptions& options) override {
    saveCalls.push_back(options);
    return next();
  }

 private:
  std::optional<std::string> next() {
    if (results.empty()) return std::nullopt;
    auto value = results.front();
    results.pop_front();
    return value;
  }
};

/** 假的可打开目录。 */
class FakeDirectory : public contracts::OpenableDirectory {
 public:
  std::string error;
  int opened{0};

  [[nodiscard]] std::string open() override {
    ++opened;
    return error;
  }
};

/** 内存里的最近打开的卡带服务。 */
class FakeRecentCartridges : public contracts::RecentCartridgesService {
 public:
  std::vector<std::string> paths;

  [[nodiscard]] std::vector<contracts::RecentCartridge> list() const override {
    std::vector<contracts::RecentCartridge> result;
    for (std::size_t i = 0; i < paths.size(); ++i) result.push_back({static_cast<std::int64_t>(i + 1), paths[i].substr(paths[i].find_last_of("\\/") + 1)});
    return result;
  }
  void add(const std::string& path) override {
    std::erase(paths, path);
    paths.insert(paths.begin(), path);
  }
  void remove(const std::string& path) override { std::erase(paths, path); }
  [[nodiscard]] std::optional<std::string> resolve(std::int64_t id) const override { return id >= 1 && static_cast<std::size_t>(id) <= paths.size() ? std::optional<std::string>(paths[static_cast<std::size_t>(id - 1)]) : std::nullopt; }
  void flush() override {}
};

/** 记下写入内容的错误日志。 */
class FakeErrorLogger : public contracts::ErrorLogger {
 public:
  std::vector<contracts::ErrorLogEntry> entries;

  void write(const contracts::ErrorLogEntry& entry) noexcept override { entries.push_back(entry); }
};

/** 假的升级：状态可设，install 的结果可设。 */
class FakeUpdater : public contracts::AppUpdater {
 public:
  contracts::UpdateStatus current{"idle", std::nullopt, std::nullopt};
  bool installResult{false};
  int checks{0};

  [[nodiscard]] contracts::UpdateStatus status() const override { return current; }
  contracts::UpdateStatus check() override {
    ++checks;
    current.state = "checking";
    return current;
  }
  bool install() override { return installResult; }
};

/** 假的帮助文档。 */
class FakeHelp : public contracts::HelpViewer {
 public:
  std::string error;
  std::vector<std::optional<std::string>> pages;

  [[nodiscard]] std::string open(const std::optional<std::string>& page) override {
    pages.push_back(page);
    return error;
  }
};

}  // namespace ember::win32::test
