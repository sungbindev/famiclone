/*
 * ------------------------------------------------------------------------
 * 名称：gamepad-settings-model.hpp
 * 说明：手柄设置模块给界面层的模型：对话框里两个玩家各一张手柄图要显示的内容（每个键的名称与当前绑定、选中与学习状态）、输入来源与使用的手柄、说明行与“清除”的可用状态，以及用户的操作（点键学习、清除、换来源、指定手柄、恢复默认、切换玩家）。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-20
 * 备注：对应 Electron 外壳 src/ui/input 的 GamepadSettings.tsx、PadSlotEditor.tsx 与 use-binding-capture.ts 面向界面的部分；文字与可用状态由逻辑层算好，界面只显示；
 *       学习绑定：键盘来源时由对话框窗口把下一次物理按键交给 keyPressed（Esc 取消），实体手柄来源时界面定时调用 poll 读取手柄，以点击那一刻的状态为基准认出被按下的按钮或轴；规则见 docs/05-界面.md 第 4.6 节。
 * ------------------------------------------------------------------------
 */

#pragma once

#include <functional>
#include <string>
#include <vector>

namespace ember::win32::contracts {

/** 数据的读取状态。 */
enum class PadSettingsLoad {
  /** 正在读取（核心还没有就绪或核心正在重启）。 */
  Loading,
  Ready,
  /** 读取失败，说明在 message 里。 */
  Failed
};

/** 输入来源：键盘，或指定的实体手柄。 */
enum class PadInputSource { Keyboard, Gamepad };

/** 手柄图上一个键要显示的内容。 */
struct PadKeyView {
  /** 键的标识：up、down、left、right、a、b、turboA、turboB、select、start。 */
  std::string id;
  /** 键名，如“上”“A”“连发 A”。 */
  std::string role;
  /** 当前的绑定文字：没有绑定时是“未绑定”，正在学习时是“请按键”。 */
  std::string current;
  bool unbound{false};
  /** 悬停提示与可访问名称。 */
  std::string tip;
  bool selected{false};
  bool learning{false};
  bool operator==(const PadKeyView&) const = default;
};

/** “使用的手柄”下拉框的一个选项。 */
struct PadDeviceOption {
  /** 选项的值：占几号与设备标识连在一起。 */
  std::string value;
  /** 显示的文字：同一个设备标识连着多个时标出第几个，没有连接的注明“未连接”。 */
  std::string label;
  bool operator==(const PadDeviceOption&) const = default;
};

/** 一个玩家的设置。 */
struct PadPlayerView {
  PadInputSource source{PadInputSource::Keyboard};
  /** 手柄的 10 个键，按显示顺序排列。 */
  std::vector<PadKeyView> keys;
  /** 图上的键是否可点：键盘来源总是可点，实体手柄来源要先指定手柄且手柄已连接。 */
  bool keysEnabled{true};
  /** 是否显示“使用的手柄”下拉框（实体手柄来源时显示）。 */
  bool showDevice{false};
  /** 下拉框的选项：系统发现的手柄，加上已指定但现在没接上的那个。 */
  std::vector<PadDeviceOption> devices;
  /** 指定的手柄的选项值；没有指定时为空（“不使用”）。 */
  std::string assignedDevice;
  /** 图下方的说明行。 */
  std::string message;
  /** “清除”按钮的可访问名称。 */
  std::string clearLabel;
  /** “清除”是否可点：选中了键并且它在当前来源下有绑定。 */
  bool canClear{false};
  bool operator==(const PadPlayerView&) const = default;
};

/** 手柄设置对话框此刻要显示的内容。 */
struct PadSettingsView {
  PadSettingsLoad load{PadSettingsLoad::Loading};
  /** 读取失败的说明。 */
  std::string message;
  /** 两个玩家（手柄口 1、2）；读取完成前为空。 */
  std::vector<PadPlayerView> players;
  /** 当前显示的玩家页签，从 0 开始。 */
  int activePlayer{0};
  bool operator==(const PadSettingsView&) const = default;
};

/** 手柄设置模块的模型。 */
class GamepadSettingsModel {
 public:
  virtual ~GamepadSettingsModel() = default;

  [[nodiscard]] virtual PadSettingsView view() const = 0;

  /** 切换玩家页签；取消正在进行的学习。 */
  virtual void selectPlayer(int player) = 0;
  /** 切换输入来源：取消学习、取消选中的键，不改数据。 */
  virtual void setSource(int player, PadInputSource source) = 0;
  /** 点图上的键：正在学习的同一个键再点一次取消，否则选中它并开始学习（实体手柄来源要先指定手柄且已连接）。 */
  virtual void pickKey(int player, const std::string& key) = 0;
  /** 清除选中的键在当前来源下的绑定。 */
  virtual void clearSelected(int player) = 0;
  /** 指定使用的手柄（选项值）；空串是不使用；这个手柄原来在另一个玩家名下时从那里取消。 */
  virtual void assignDevice(int player, const std::string& deviceValue) = 0;
  /** 恢复默认绑定；取消学习。 */
  virtual void reset() = 0;
  /** 取消正在进行的学习。 */
  virtual void cancelLearning() = 0;

  /**
   * 对话框窗口收到一次物理按键（不重复的按下）：正在学习键盘时 Esc 取消，其余键绑给正在学习的键。
   * @param code 键位名（KeyboardEvent.code 的写法）。
   * @returns 是否被学习吞掉；被吞掉的按键不再交给界面。
   */
  [[nodiscard]] virtual bool keyPressed(const std::string& code) = 0;
  /** 是否正在学习键盘：对话框窗口据此决定要不要拦截按键。 */
  [[nodiscard]] virtual bool learningKey() const = 0;

  /** 读一次手柄：刷新系统发现的手柄，学习实体手柄时认出被按下的按钮或轴；界面按 pollIntervalMs 定时调用。 */
  virtual void poll() = 0;
  /** 调用 poll 的间隔（毫秒）：学习实体手柄时要每个画面帧读一次，平时只要刷新手柄列表。 */
  [[nodiscard]] virtual unsigned pollIntervalMs() const = 0;

  /** 订阅内容变化；返回取消订阅的函数。 */
  [[nodiscard]] virtual std::function<void()> subscribe(std::function<void()> handler) = 0;
};

}  // namespace ember::win32::contracts
