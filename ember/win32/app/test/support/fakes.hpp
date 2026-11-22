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
#include "contracts/shell/confirmation.hpp"
#include "contracts/shell/directory-opener.hpp"
#include "contracts/shell/openable-directory.hpp"
#include "contracts/shell/request-broker.hpp"
#include "contracts/shell/shell-model.hpp"
#include "contracts/shell/shell-state-service.hpp"
#include "contracts/system/app-updater.hpp"
#include "contracts/system/external-links.hpp"
#include "contracts/system/help-viewer.hpp"
#include "contracts/system/user-data-location.hpp"

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

/** 假的目录打开者：记下打开的路径。 */
class FakeDirectoryOpener : public contracts::DirectoryOpener {
 public:
  std::string error;
  std::vector<std::string> opened;

  [[nodiscard]] std::string open(const std::string& path) override {
    opened.push_back(path);
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

/** 假的外部链接：记下打开过的网址，结果可设。 */
class FakeExternalLinks : public contracts::ExternalLinks {
 public:
  std::string error;
  std::vector<std::string> opened;

  [[nodiscard]] std::string open(const std::string& url) override {
    opened.push_back(url);
    return error;
  }
};

/** 假的用户数据位置：状态与各项结果可设，记下启动移动的目标。 */
class FakeUserDataLocation : public contracts::UserDataLocation {
 public:
  contracts::UserDataState current{"C:\\Users\\me\\AppData\\Roaming\\Ember", shared::UserDataPlace::UserProfile, false};
  std::string executableUnavailable;
  std::string profileUnavailable;
  std::string moveError;
  std::vector<shared::UserDataPlace> moves;

  [[nodiscard]] contracts::UserDataState state() const override { return current; }
  [[nodiscard]] std::string directoryOf(shared::UserDataPlace place) const override { return place == shared::UserDataPlace::ExecutableDirectory ? "D:\\Ember\\ember-data" : "C:\\Users\\me\\AppData\\Roaming\\Ember"; }
  [[nodiscard]] std::string unavailableReason(shared::UserDataPlace target) const override { return target == shared::UserDataPlace::ExecutableDirectory ? executableUnavailable : profileUnavailable; }
  [[nodiscard]] std::string startMove(shared::UserDataPlace target) override {
    moves.push_back(target);
    return moveError;
  }
};

/** 假的确认询问：回答可设，记下问过的内容。 */
class FakeConfirmation : public contracts::Confirmation {
 public:
  bool answer{true};
  std::vector<std::string> asked;

  [[nodiscard]] bool ask(const std::string&, const std::string& text) override {
    asked.push_back(text);
    return answer;
  }
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
  std::map<int, std::function<void(const contracts::UpdateStatus&)>> handlers;

  [[nodiscard]] contracts::UpdateStatus status() const override { return current; }
  contracts::UpdateStatus check() override {
    ++checks;
    current.state = "checking";
    return current;
  }
  bool install() override { ++installs; return installResult; }
  [[nodiscard]] std::function<void()> subscribe(std::function<void(const contracts::UpdateStatus&)> handler) override {
    const int id = nextHandler_++;
    handlers[id] = std::move(handler);
    return [this, id] { handlers.erase(id); };
  }
  /** 改变状态并通知订阅者。 */
  void push(const contracts::UpdateStatus& status) {
    current = status;
    const auto copy = handlers;
    for (const auto& [id, handler] : copy) handler(current);
  }
  int installs{0};

 private:
  int nextHandler_{1};
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

/** 假的界面状态服务：只在内存里，记下保存次数。 */
class FakeShellState : public contracts::ShellStateService {
 public:
  shared::ShellState state;
  int layoutUpdates{0};

  [[nodiscard]] const shared::ShellState& get() const override { return state; }
  void updateLayout(const shared::ShellLayout& layout) override {
    state.layout = layout;
    ++layoutUpdates;
  }
  void updateWindow(const shared::WindowPlacement& placement) override { state.window = placement; }
  void updateReleaseNotesVersion(const std::string& version) override { state.releaseNotesVersion = version; }
  void flush() override {}
};

/** 假的拖入文件服务：按路径的文件名登记，编号从一开始；路径含 unreadable 的不登记。 */
class FakeDroppedFiles : public contracts::DroppedFilesService {
 public:
  std::map<std::int64_t, std::string> paths;

  [[nodiscard]] std::vector<contracts::DroppedFile> registerFiles(const std::vector<std::string>& files) override {
    std::vector<contracts::DroppedFile> result;
    for (const auto& path : files) {
      if (path.find("unreadable") != std::string::npos) continue;
      const std::int64_t id = static_cast<std::int64_t>(paths.size()) + 1;
      paths[id] = path;
      const std::size_t slash = path.find_last_of("\\/");
      result.push_back({id, slash == std::string::npos ? path : path.substr(slash + 1)});
    }
    return result;
  }
  [[nodiscard]] std::optional<std::string> resolve(std::int64_t id) const override {
    const auto found = paths.find(id);
    return found == paths.end() ? std::nullopt : std::optional<std::string>(found->second);
  }
};

/** 假的窗口控制：记下调用。 */
class FakeShellWindow : public contracts::ShellWindowControl {
 public:
  int fullScreenToggles{0};
  int topmostToggles{0};
  int quits{0};

  void toggleFullScreen() override { ++fullScreenToggles; }
  void toggleAlwaysOnTop() override { ++topmostToggles; }
  void quit() override { ++quits; }
};

/** 假的对话框打开者与通知发出者：记下调用。 */
class FakeShellDialogs : public contracts::ShellDialogOpener {
 public:
  std::vector<std::string> opened;

  void openDialog(const std::string& dialogId) override { opened.push_back(dialogId); }
};

class FakeNotifier : public contracts::ShellNotifier {
 public:
  std::vector<std::pair<std::string, contracts::NotificationLevel>> notices;
  /** 与 notices 一一对应：每条通知带的链接（没有链接时 label 为空）。 */
  std::vector<contracts::NotificationAction> actions;

  void notify(const std::string& text, contracts::NotificationLevel level, contracts::NotificationAction action) override {
    notices.emplace_back(text, level);
    actions.push_back(std::move(action));
  }
};

/** 假的请求中转：请求都交给假核心网关（主进程处理的请求也在它的 replies 里设回复）。 */
class FakeBroker : public contracts::RequestBroker {
 public:
  explicit FakeBroker(FakeCoreGateway& gateway) : gateway_(gateway) {}

  void request(const std::string& name, nlohmann::json params, contracts::CoreReplyHandler done) override { gateway_.request(name, std::move(params), std::move(done)); }

 private:
  FakeCoreGateway& gateway_;
};

/** 假的外壳接口：记下通知与刷新次数，其余调用只计数。 */
class FakeShellApi : public contracts::ShellApi {
 public:
  std::vector<std::pair<std::string, contracts::NotificationLevel>> notices;
  std::vector<contracts::NotificationAction> actions;
  int refreshes{0};
  std::vector<std::string> commands;
  std::vector<std::string> dialogs;
  int quits{0};

  void openDialog(const std::string& dialogId) override { dialogs.push_back(dialogId); }
  void togglePanel(const std::string&) override {}
  void toggleFullScreen() override {}
  void toggleAlwaysOnTop() override {}
  void quit() override { ++quits; }
  void runCommand(const std::string& commandId) override { commands.push_back(commandId); }
  void refreshCommands() override { ++refreshes; }
  void notify(const std::string& text, contracts::NotificationLevel level, contracts::NotificationAction action) override {
    notices.emplace_back(text, level);
    actions.push_back(std::move(action));
  }
};

}  // namespace ember::win32::test
