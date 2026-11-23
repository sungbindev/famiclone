/*
 * ------------------------------------------------------------------------
 * 名称：keybindings-controller.hpp
 * 说明：快捷键的控制器：用登记表里的默认快捷键与界面状态里用户的修改算出每条命令实际使用的快捷键，供菜单显示与按键触发；同时是首选项“快捷键”页的模型，执行设置、清除与恢复默认。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-24
 * 备注：用户的修改只存与默认不同的命令（空串表示清除），改动立即保存到界面状态；
 *       设置的快捷键必须通过 shared::checkShortcut（写法合格、不占保留组合与菜单的 Alt 加助记字母、含 Ctrl 或 Alt，F1 至 F24 除外），且不能与别的命令实际使用的快捷键相同，否则把说明放在这一行的 error 里、不保存；
 *       读到的修改里命令已不存在、写法不合格或与别的修改重复的不采用；规则见 docs/05-界面.md 第 4.5、5 节。
 * ------------------------------------------------------------------------
 */

#pragma once

#include <functional>
#include <map>
#include <optional>
#include <string>
#include <vector>

#include "contracts/shell/keybindings-model.hpp"
#include "contracts/shell/shell-registry.hpp"
#include "contracts/shell/shell-state-service.hpp"
#include "shared/shortcut/shortcut.hpp"

namespace ember::win32::logic {

/** 快捷键的控制器。 */
class KeybindingsController final : public contracts::KeybindingsModel {
 public:
  /**
   * @param commands 登记表里的全部命令，引用要在本对象存在期间一直有效。
   * @param menuItems 登记表里的菜单项，决定命令在设置页里的分组与顺序，引用同样要一直有效。
   * @param state 界面状态：读出用户的修改，改动后写回。
   * @param onChanged 实际使用的快捷键变了（菜单要重新取）。
   */
  KeybindingsController(const std::map<std::string, contracts::ShellCommandDefinition>& commands, const std::vector<contracts::ShellMenuItemDefinition>& menuItems, contracts::ShellStateService& state,
                        std::function<void()> onChanged);

  /** 命令实际使用的快捷键；没有或命令不存在时为空串。 */
  [[nodiscard]] std::string shortcutOf(const std::string& commandId) const;
  /** 实际使用这个快捷键的命令；没有时没有值。 */
  [[nodiscard]] std::optional<std::string> commandFor(const std::string& shortcut) const;

  // ---- KeybindingsModel ----
  [[nodiscard]] contracts::KeybindingsView view() const override;
  void startCapture(const std::string& commandId) override;
  void cancelCapture() override;
  [[nodiscard]] bool shortcutKeyPressed(int virtualKey, unsigned modifiers) override;
  void clear(const std::string& commandId) override;
  void reset(const std::string& commandId) override;
  void resetAll() override;
  [[nodiscard]] std::function<void()> subscribe(std::function<void()> handler) override;

 private:
  /** 设置页里的一组：同一个菜单里的命令。 */
  struct Group {
    std::string id;
    std::string label;
    std::vector<std::string> commands;
  };

  /** 把快捷键设给命令（空串表示清除）：不合格或冲突时只记下说明。 */
  void assign(const std::string& commandId, const std::string& shortcut);
  void fail(const std::string& commandId, const std::string& text);
  /** 按当前的修改重新算实际的快捷键，丢弃没有被采用的修改。 */
  void recompute();
  /** 修改有变化后：重算、保存、通知菜单与订阅者。 */
  void apply();
  void notify();
  [[nodiscard]] std::string labelOf(const std::string& commandId) const;
  [[nodiscard]] std::string defaultOf(const std::string& commandId) const;

  const std::map<std::string, contracts::ShellCommandDefinition>& commands_;
  contracts::ShellStateService& state_;
  std::function<void()> onChanged_;
  std::vector<char> mnemonics_;
  std::vector<Group> groups_;
  /** 全部命令按设置页里的顺序，附默认快捷键。 */
  std::vector<shared::CommandShortcut> order_;
  /** 用户的修改。 */
  std::map<std::string, std::string> overrides_;
  /** 每条命令实际使用的快捷键。 */
  std::map<std::string, std::string> effective_;
  /** 正在等用户按键的命令，没有时为空。 */
  std::string capturing_;
  std::string errorCommand_;
  std::string errorText_;
  std::map<int, std::function<void()>> listeners_;
  int nextListener_{1};
};

}  // namespace ember::win32::logic
