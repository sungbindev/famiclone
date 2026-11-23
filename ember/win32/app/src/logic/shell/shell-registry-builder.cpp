/*
 * ------------------------------------------------------------------------
 * 名称：shell-registry-builder.cpp
 * 说明：登记表汇总与检查的实现。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-19
 * 备注：无。
 * ------------------------------------------------------------------------
 */

#include "logic/shell/shell-registry-builder.hpp"

#include <algorithm>
#include <cctype>
#include <iterator>
#include <set>
#include <stdexcept>

#include "shared/shortcut/shortcut.hpp"

namespace ember::win32::logic {

namespace {

/** 模块名格式：小写字母开头，字母与数字，段之间用连字符。 */
bool isValidModuleName(const std::string& name) {
  if (name.empty() || !(name[0] >= 'a' && name[0] <= 'z')) return false;
  bool previousHyphen = false;
  for (std::size_t i = 1; i < name.size(); ++i) {
    const char c = name[i];
    if (c == '-') {
      if (previousHyphen) return false;
      previousHyphen = true;
      continue;
    }
    if (!((c >= 'a' && c <= 'z') || (c >= '0' && c <= '9'))) return false;
    previousHyphen = false;
  }
  return !previousHyphen;
}

/** 扩展名的写法：小写字母与数字，不带点。 */
bool isValidExtension(const std::string& extension) {
  return !extension.empty() && std::all_of(extension.begin(), extension.end(), [](char c) { return (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9'); });
}

/** 登记标识的检查器：必须以“模块名.”开头，且同一种登记项里不重复。 */
class Claims {
 public:
  explicit Claims(std::vector<std::string>& errors) : errors_(errors) {}

  bool claim(const std::string& kind, const std::string& id, const std::string& owner) {
    if (id.rfind(owner + ".", 0) != 0 || id.size() <= owner.size() + 1) {
      errors_.push_back(kind + "“" + id + "”的标识必须以“" + owner + ".”开头");
      return false;
    }
    if (!claimed_[kind].insert(id).second) {
      errors_.push_back(kind + "“" + id + "”重复登记");
      return false;
    }
    return true;
  }

 private:
  std::vector<std::string>& errors_;
  std::map<std::string, std::set<std::string>> claimed_;
};

/** 菜单标识是否是 kShellMenus 里的。 */
bool isKnownMenu(const std::string& menu) {
  return std::any_of(std::begin(contracts::kShellMenus), std::end(contracts::kShellMenus), [&menu](const contracts::ShellMenuInfo& info) { return menu == info.id; });
}

}  // namespace

ShellRegistry buildShellRegistry(std::vector<contracts::ShellModule> modules) {
  std::vector<std::string> errors;
  Claims claims(errors);
  ShellRegistry registry;
  std::map<std::string, std::string> claimedExtensions;
  std::vector<contracts::ShellMenuItemDefinition> shellMenuItems;
  std::set<std::string> names;

  for (auto& module : modules) {
    const std::string owner = module.name;
    if (!isValidModuleName(owner) || !names.insert(owner).second) {
      errors.push_back("模块名“" + owner + "”不合格或重复");
      continue;
    }
    if (module.hasScreen) {
      if (!registry.screenOwner.empty()) errors.push_back("显示区只能有一个，“" + registry.screenOwner + "”与“" + owner + "”都登记了");
      else registry.screenOwner = owner;
    }
    for (auto& dialog : module.dialogs) {
      if (claims.claim("对话框", dialog.id, owner)) registry.dialogs.emplace(dialog.id, std::move(dialog));
    }
    for (auto& command : module.commands) {
      if (claims.claim("命令", command.id, owner)) registry.commands.emplace(command.id, std::move(command));
    }
    for (auto& item : module.bottomPanels) {
      if (claims.claim("底部面板页签", item.id, owner)) registry.bottomPanels.push_back(std::move(item));
    }
    for (auto& item : module.statusItems) {
      if (claims.claim("状态项", item.id, owner)) registry.statusItems.push_back(std::move(item));
    }
    for (auto& item : module.titleBarItems) {
      if (claims.claim("标题栏项", item.id, owner)) registry.titleBarItems.push_back(std::move(item));
    }
    for (auto& item : module.fileHandlers) {
      if (!claims.claim("文件处理项", item.id, owner)) continue;
      if (item.extensions.empty() || !std::all_of(item.extensions.begin(), item.extensions.end(), isValidExtension)) {
        errors.push_back("文件处理项“" + item.id + "”的扩展名必须是小写字母与数字、不带点，且至少有一个");
        continue;
      }
      for (const auto& extension : item.extensions) {
        const auto [position, inserted] = claimedExtensions.emplace(extension, item.id);
        if (!inserted) errors.push_back("文件处理项“" + item.id + "”与“" + position->second + "”都认领了扩展名 " + extension);
      }
      registry.fileHandlers.push_back(std::move(item));
    }
    auto& menuTarget = owner == kShellNamespace ? shellMenuItems : registry.menuItems;
    for (auto& item : module.menuItems) {
      if (!isKnownMenu(item.menu)) {
        errors.push_back("模块“" + owner + "”的菜单项放在不存在的菜单里：" + item.menu);
      } else if (!item.command.empty() && item.submenu.has_value()) {
        errors.push_back("模块“" + owner + "”的菜单项不能同时是命令与子菜单：" + item.command);
      } else if (item.submenu.has_value()) {
        if (claims.claim("子菜单", item.submenu->id, owner)) menuTarget.push_back(std::move(item));
      } else if (item.command.empty()) {
        errors.push_back("模块“" + owner + "”有菜单项既没有命令也没有子菜单");
      } else if (item.command.rfind(owner + ".", 0) == 0) {
        menuTarget.push_back(std::move(item));
      } else {
        errors.push_back("模块“" + owner + "”的菜单项只能使用本模块的命令：" + item.command);
      }
    }
    if (module.attach) registry.attachments.emplace_back(owner, std::move(module.attach));
  }

  registry.menuItems.insert(registry.menuItems.end(), std::make_move_iterator(shellMenuItems.begin()), std::make_move_iterator(shellMenuItems.end()));
  for (const auto& item : registry.menuItems) {
    if (!item.command.empty() && registry.commands.find(item.command) == registry.commands.end()) errors.push_back("菜单项引用的命令不存在：" + item.command);
  }

  std::vector<char> mnemonics;
  for (const auto& menu : contracts::kShellMenus) mnemonics.push_back(menu.mnemonic);
  for (const auto& [id, command] : registry.commands) {
    if (command.defaultShortcut.empty()) continue;
    if (const auto reason = shared::checkShortcut(command.defaultShortcut, mnemonics)) errors.push_back("命令“" + id + "”的默认快捷键 " + command.defaultShortcut + " 不可用：" + *reason);
    const auto [position, inserted] = registry.shortcuts.emplace(command.defaultShortcut, id);
    if (!inserted) errors.push_back("命令“" + id + "”与“" + position->second + "”的默认快捷键 " + command.defaultShortcut + " 重复");
  }

  if (!errors.empty()) {
    std::string message = "外壳登记有误：";
    for (const auto& error : errors) message += "\n- " + error;
    throw std::runtime_error(message);
  }
  return registry;
}

}  // namespace ember::win32::logic
