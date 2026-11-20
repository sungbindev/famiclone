/*
 * ------------------------------------------------------------------------
 * 名称：runtime-model.hpp
 * 说明：运行模块给界面层的模型：机器的状态、各控制是否可用、音量、画面设置与鼠标捕获，以及用户的操作（开机或断电、暂停或继续、复位、调音量、静音）。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-19
 * 备注：模型不含控件与系统类型，状态变化时调用 setOnChanged 设置的回调，界面据此重新读取并重画。
 *       可用性规则见 docs/05-界面.md 第 4.1 节：没有开机时暂停、复位置灰；断电时暂停与软复位置灰；硬复位只在通电时可用；
 *       状态还没读到（核心启动中、重启中或不可用）时为 Unknown，全部置灰。
 * ------------------------------------------------------------------------
 */

#pragma once

#include <functional>
#include <string>

#include "shared/screen/screen-layout.hpp"

namespace ember::win32::contracts {

/** 机器的状态：Unknown 为还没读到，None 为没有开机，其余是开机后的电源与运行模式。 */
enum class MachineState { Unknown, None, Off, Paused, Running };

/** 各控制是否可用。 */
struct RuntimeControls {
  bool powerToggle{false};
  bool pauseToggle{false};
  bool softReset{false};
  bool hardReset{false};
  /** 关闭（取出）卡带。 */
  bool eject{false};
};

/** 画面区用的设置：显示（比例、缩放、行数）与点击画面是否捕获鼠标。 */
struct ScreenSettings {
  shared::ScreenAspect aspect{shared::kDefaultScreenAspect};
  shared::ScreenScaling scaling{shared::kDefaultScreenScaling};
  bool showAllLines{true};
  bool captureMouse{true};
};

/** 运行模块此刻要显示的内容。 */
struct RuntimeView {
  MachineState state{MachineState::Unknown};
  /** 开机或断电按钮与菜单项的名称。 */
  std::string powerToggleText;
  /** 暂停或继续按钮与菜单项的名称。 */
  std::string pauseToggleText;
  RuntimeControls controls;
  /** 音量，0 到 100；静音时保持原值。 */
  int volume{100};
  bool muted{false};
  /** 是否捕获鼠标：设置打开并且机器现在带鼠标。 */
  bool capturesMouse{false};
  /** 键盘是否整个交给机器（独占）：学习机整机机型，或接入了 Family BASIC 键盘；经典 FC 的普通游戏不独占，快捷键照常生效。 */
  bool exclusiveKeyboard{false};
  ScreenSettings screen;
};

/** 运行模块的模型。 */
class RuntimeModel {
 public:
  virtual ~RuntimeModel() = default;

  [[nodiscard]] virtual RuntimeView view() const = 0;

  /** 开机或断电：通电时断电；未通电时按已选机型开机（规则见 runtime-controller.hpp）。 */
  virtual void togglePower() = 0;
  /** 暂停或继续：连续运行时暂停，否则继续；断电时不可用。 */
  virtual void togglePause() = 0;
  virtual void softReset() = 0;
  virtual void hardReset() = 0;
  /** 调整音量（0 到 100）。 */
  virtual void setVolume(int value) = 0;
  /** 切换静音：音量保持原值。 */
  virtual void toggleMute() = 0;

  /** 登记画面区的聚焦函数：操作结束后调用，键盘才能继续控制游戏；空函数表示去掉。 */
  virtual void setScreenFocus(std::function<void()> focus) = 0;
  /** 订阅内容变化（面板、画面控件各订阅一次）；返回取消订阅的函数。 */
  [[nodiscard]] virtual std::function<void()> subscribe(std::function<void()> handler) = 0;
};

}  // namespace ember::win32::contracts
