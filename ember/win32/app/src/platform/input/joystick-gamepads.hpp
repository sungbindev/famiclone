/*
 * ------------------------------------------------------------------------
 * 名称：joystick-gamepads.hpp
 * 说明：游戏手柄：每个画面帧读一次系统的通用游戏控制器（DirectInput 一类的 USB 手柄，如北通的 DInput 模式），给出按钮与轴的原始编号。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-20
 * 备注：用 Windows 7 自带的 winmm 手柄接口（joyGetPosEx），最多 12 个；系统会把 XInput 手柄也列成通用控制器，这里按硬件标识（含 IG_ 的 HID 设备）跳过它们，由 XInputGamepads 读；
 *       设备标识的格式与浏览器里没有标准映射的 Gamepad.id 一致（“名称 (Vendor: 0079 Product: 0011)”）；
 *       按钮：系统报告的按钮数（最多 32），顺序即按钮编号；
 *       轴：0 到 5 依次是 X、Y、Z、R、U、V（没有的轴恒为 0），6、7 是方向帽的左右与上下（右、下为正）；
 *       手柄的位置（GamepadReading::index）从 4 开始，不与 XInput 的 0 到 3 重叠。
 * ------------------------------------------------------------------------
 */

#pragma once

#include <string>
#include <vector>

#include "platform/input/xinput-gamepads.hpp"

namespace ember::win32::platform {

/** 通用游戏控制器读取。 */
class JoystickGamepads {
 public:
  JoystickGamepads() = default;

  /** 读一轮：返回现在连着的通用手柄，没有时为空。 */
  [[nodiscard]] std::vector<GamepadReading> read();

 private:
  struct Device {
    unsigned joyId{0};
    std::string id;
    int buttonCount{0};
    /** 轴是否存在（X、Y、Z、R、U、V）。 */
    bool hasAxis[6]{};
    /** 每个轴的最小值与最大值。 */
    unsigned range[6][2]{};
    bool hasHat{false};
  };

  void rescan();
  /** 读一个设备；读不到（已拔掉）时返回 false。 */
  static bool poll(const Device& device, GamepadReading& reading);

  std::vector<Device> devices_;
  unsigned polls_{0};
};

}  // namespace ember::win32::platform
