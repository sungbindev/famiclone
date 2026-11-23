/*
 * ------------------------------------------------------------------------
 * 名称：keybindings-model.hpp
 * 说明：快捷键设置的模型：首选项“快捷键”页要显示的每条命令的名称与实际快捷键，以及用户的操作（设置、清除、恢复默认）。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-24
 * 备注：命令与默认快捷键来自外壳的登记表，用户的修改保存在界面状态里；文字与可用状态由逻辑层算好，界面只显示；
 *       设置快捷键时先 startCapture，之后对话框窗口把按下的键交给 shortcutKeyPressed；不合格或与别的命令冲突时说明写在这一行的 error 里，不用通知；规则见 docs/05-界面.md 第 4.5、5 节。
 * ------------------------------------------------------------------------
 */

#pragma once

#include <functional>
#include <string>
#include <vector>

namespace ember::win32::contracts {

/** 一条命令的快捷键。 */
struct KeybindingRowView {
  /** 命令标识，如 runtime.powerToggle。 */
  std::string id;
  /** 命令的名称。 */
  std::string label;
  /** 实际使用的快捷键；空表示没有。 */
  std::string shortcut;
  /** 实际的快捷键与默认的不同：不同时显示“恢复默认”。 */
  bool changed{false};
  /** 正在等用户按下新的快捷键。 */
  bool capturing{false};
  /** 这一行上一次操作不成功的说明；空表示没有。 */
  std::string error;
  bool operator==(const KeybindingRowView&) const = default;
};

/** 一组命令：同一个菜单里的命令放一组。 */
struct KeybindingGroupView {
  /** 菜单标识（文件、机器、设置、帮助），不在任何菜单里的命令放“other”。 */
  std::string id;
  std::string label;
  std::vector<KeybindingRowView> rows;
  bool operator==(const KeybindingGroupView&) const = default;
};

/** 快捷键页要显示的内容。 */
struct KeybindingsView {
  std::vector<KeybindingGroupView> groups;
  /** 有命令的快捷键与默认的不同：为真时“全部恢复默认”可用。 */
  bool anyChanged{false};
  bool operator==(const KeybindingsView&) const = default;
};

/** 快捷键设置的模型。 */
class KeybindingsModel {
 public:
  virtual ~KeybindingsModel() = default;

  [[nodiscard]] virtual KeybindingsView view() const = 0;

  /** 开始为一条命令设置快捷键：等用户按下新的组合；同一时间只有一条命令在等，之前在等的取消。 */
  virtual void startCapture(const std::string& commandId) = 0;
  /** 取消正在等的设置；没有在等时什么都不做。 */
  virtual void cancelCapture() = 0;
  /**
   * 对话框窗口里按下了键。
   * 没有在等时返回 false；在等时返回 true（键被吞掉）：Esc 取消；只按了修饰键等不能构成快捷键的键继续等；其余按键写成快捷键并设给正在等的命令，成功时保存，不合格或冲突时把说明放在这一行的 error 里。
   * @param virtualKey Windows 虚拟键码。
   * @param modifiers 修饰键（Shift=1、Ctrl=2、Alt=4 的位和）。
   */
  [[nodiscard]] virtual bool shortcutKeyPressed(int virtualKey, unsigned modifiers) = 0;
  /** 清除一条命令的快捷键。 */
  virtual void clear(const std::string& commandId) = 0;
  /** 把一条命令恢复成默认快捷键；默认的快捷键已被别的命令使用时在这一行的 error 里说明，不恢复。 */
  virtual void reset(const std::string& commandId) = 0;
  /** 全部恢复成默认快捷键。 */
  virtual void resetAll() = 0;

  /** 订阅内容变化；返回取消订阅的函数。 */
  [[nodiscard]] virtual std::function<void()> subscribe(std::function<void()> handler) = 0;
};

}  // namespace ember::win32::contracts
