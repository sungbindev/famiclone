/*
 * ------------------------------------------------------------------------
 * 名称：screen-input.hpp
 * 说明：画面区的输入端口：键盘、鼠标与游戏手柄的原始事件交给它，它转成核心的输入消息；界面层的画面控件与平台层的输入来源都经它。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-19
 * 备注：界面不做任何键位映射，原始事件交给核心：键盘与手柄由核心按手柄绑定变成手柄口的按键，其余键盘与鼠标事件交给机器（卡带的 mapper 脚本）处理；
 *       键位名用 KeyboardEvent.code 的写法（物理键位）；鼠标坐标是标准帧（256x240）上的坐标；手柄的按键与轴按浏览器的标准映射排列。
 * ------------------------------------------------------------------------
 */

#pragma once

#include <string>
#include <vector>

#include "shared/screen/pointer-mapping.hpp"

namespace ember::win32::contracts {

/** 鼠标事件的种类。 */
enum class MouseKind { Move, Down, Up, Wheel, Leave };

/** 鼠标按键位：1 左键、2 右键、4 中键。 */
inline constexpr int kMouseButtonLeft = 1;
inline constexpr int kMouseButtonRight = 2;
inline constexpr int kMouseButtonMiddle = 4;

/** 一个手柄此刻的状态。 */
struct GamepadSample {
  /** 手柄位置（0 起）。 */
  int index{0};
  /** 设备标识，格式同浏览器的 Gamepad.id。 */
  std::string id;
  bool connected{true};
  std::vector<bool> buttons;
  std::vector<double> axes;
};

/** 画面区的输入端口。 */
class ScreenInput {
 public:
  virtual ~ScreenInput() = default;

  /** 键盘独占交来的一次物理按键：按住不放的重复按下忽略，其余全部上报，没有保留键。 */
  virtual void nativeKey(const std::string& code, bool down) = 0;
  /** 松开全部按着的键：窗口失焦、画面区失去焦点、关闭时调用。 */
  virtual void releaseAllKeys() = 0;

  /**
   * 上报一次鼠标事件（指针在标准帧上的位置）。
   * @param kind 事件种类；Leave 不看位置，按最后的位置上报。
   * @param point 指针在标准帧上的位置。
   * @param buttons 事件之后按住的按键位。
   * @param wheel 滚轮的滚动量，向下为正，只有 Wheel 事件不为 0。
   */
  virtual void mouseAt(MouseKind kind, const shared::FramePoint& point, int buttons, double wheel) = 0;
  /**
   * 捕获鼠标时上报一次事件：只有相对位移，位置是累加出来的虚拟位置（从画面中心开始，限制在整帧范围内）。
   * @param dx 横向位移，单位是标准帧的像素。
   * @param dy 纵向位移。
   */
  virtual void mouseMoveBy(MouseKind kind, double dx, double dy, int buttons, double wheel) = 0;
  /** 指针离开画面区：按最后的位置上报一次 leave，之后的第一次移动不带位移。 */
  virtual void mouseLeave(int buttons) = 0;

  /** 处理一轮读到的手柄：状态变了的上报，原来有现在没有的上报断开。 */
  virtual void pollGamepads(const std::vector<GamepadSample>& pads) = 0;
  /** 全部手柄断开：画面区关闭时调用，核心里的脚本据此松开手柄上的按键。 */
  virtual void disconnectGamepads() = 0;
};

}  // namespace ember::win32::contracts
