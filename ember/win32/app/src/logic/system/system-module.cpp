/*
 * ------------------------------------------------------------------------
 * 名称：system-module.cpp
 * 说明：系统模块登记项的实现。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-21
 * 备注：无。
 * ------------------------------------------------------------------------
 */

#include "logic/system/system-module.hpp"

namespace ember::win32::logic {

namespace {

/** 对话框内容区的初始高度与最小高度（逻辑像素）；宽度按内容算，打开时就用最小宽度。 */
constexpr int kDialogHeight = 560;
constexpr int kDialogMinHeight = 320;

/** 用户可见的文字。 */
constexpr const char* kAbout = "关于 Ember";
constexpr const char* kOpenHelp = "帮助文档";
constexpr const char* kOpenChangelog = "更新记录";
constexpr const char* kHelpFailed = "无法打开帮助文档";
constexpr const char* kOpenLogDirectory = "打开日志目录";
constexpr const char* kLogDirectoryFailed = "无法打开日志目录";
constexpr const char* kUpdate = "更新";

/** 发一个外壳请求，失败时用通知给出核心给的说明（没有说明用 fallback）。 */
void requestOrNotify(contracts::RequestBroker& broker, contracts::ShellApi& shell, const std::string& name, nlohmann::json params, const char* fallback) {
  contracts::ShellApi* target = &shell;
  broker.request(name, std::move(params), [target, fallback](contracts::CoreReply reply) {
    if (!reply.ok) target->notify(reply.error.message.empty() ? std::string(fallback) : reply.error.message, contracts::NotificationLevel::Error);
  });
}

contracts::ShellCommandDefinition command(const char* id, std::function<std::string()> title, std::function<void(contracts::ShellApi&)> run) {
  contracts::ShellCommandDefinition result;
  result.id = id;
  result.title = std::move(title);
  result.run = std::move(run);
  return result;
}

contracts::ShellMenuItemDefinition menuItem(const char* commandId, bool separatorBefore) {
  contracts::ShellMenuItemDefinition item;
  item.menu = "help";
  item.command = commandId;
  item.separatorBefore = separatorBefore;
  return item;
}

}  // namespace

contracts::ShellModule createSystemModule(contracts::RequestBroker& broker, UpdateController& update, AboutController& about) {
  contracts::ShellModule module;
  module.name = "system";

  contracts::ShellDialogDefinition dialog;
  dialog.id = kAboutDialogId;
  dialog.title = kAbout;
  dialog.height = kDialogHeight;
  dialog.fitMinWidth = true;
  dialog.fitMinHeight = true;
  dialog.minHeight = kDialogMinHeight;
  module.dialogs.push_back(std::move(dialog));

  contracts::ShellTitleBarItemDefinition updateItem;
  updateItem.id = kSystemUpdateTitleBarItem;
  updateItem.view = [&update]() -> std::optional<contracts::ShellTitleBarItemView> {
    if (!update.isReady()) return std::nullopt;
    return contracts::ShellTitleBarItemView{kUpdate, update.title()};
  };
  updateItem.click = [&update](contracts::ShellApi& shell) { update.run(shell); };
  module.titleBarItems.push_back(std::move(updateItem));

  module.commands.push_back(command(kSystemOpenHelpCommand, [] { return std::string(kOpenHelp); },
                                    [&broker](contracts::ShellApi& shell) { requestOrNotify(broker, shell, "system.openHelp", nlohmann::json::object(), kHelpFailed); }));
  module.commands.push_back(command(kSystemOpenChangelogCommand, [] { return std::string(kOpenChangelog); },
                                    [&broker](contracts::ShellApi& shell) { requestOrNotify(broker, shell, "system.openHelp", {{"page", "changelog"}}, kHelpFailed); }));
  module.commands.push_back(command(kSystemOpenLogDirectoryCommand, [] { return std::string(kOpenLogDirectory); },
                                    [&broker](contracts::ShellApi& shell) { requestOrNotify(broker, shell, "system.openLogDirectory", nlohmann::json::object(), kLogDirectoryFailed); }));
  auto check = command(kSystemCheckUpdateCommand, [&update] { return update.title(); }, [&update](contracts::ShellApi& shell) { update.run(shell); });
  check.enabled = [&update](const contracts::ShellViewState&) { return update.canRun(); };
  module.commands.push_back(std::move(check));
  module.commands.push_back(command(kSystemOpenAboutCommand, [] { return std::string(kAbout); }, [](contracts::ShellApi& shell) { shell.openDialog(kAboutDialogId); }));

  module.menuItems.push_back(menuItem(kSystemOpenHelpCommand, false));
  module.menuItems.push_back(menuItem(kSystemOpenChangelogCommand, false));
  module.menuItems.push_back(menuItem(kSystemOpenLogDirectoryCommand, false));
  module.menuItems.push_back(menuItem(kSystemCheckUpdateCommand, true));
  module.menuItems.push_back(menuItem(kSystemOpenAboutCommand, true));

  module.attach = [&update, &about](contracts::ShellApi& shell) -> std::function<void()> {
    auto stopUpdate = update.attach(shell);
    auto stopAbout = about.attach(shell);
    return [stopUpdate = std::move(stopUpdate), stopAbout = std::move(stopAbout)] {
      stopUpdate();
      stopAbout();
    };
  };
  return module;
}

}  // namespace ember::win32::logic
