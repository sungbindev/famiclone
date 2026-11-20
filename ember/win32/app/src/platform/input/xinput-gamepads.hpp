/*
 * ------------------------------------------------------------------------
 * 名称：xinput-gamepads.hpp
 * 说明：游戏手柄：每个画面帧读一次 XInput 手柄（Xbox 类手柄，以及多数带 XInput 模式的第三方手柄），按浏览器的“标准映射”给出按钮与轴。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-19
 * 备注：动态加载 Windows 7 自带的 xinput9_1_0.dll（不依赖需要另装的 xinput1_3.dll），所以没有这个库时手柄不可用但程序照常运行；
 *       设备标识的格式与浏览器里的 Gamepad.id 一致（“Xbox 360 Controller (XInput STANDARD GAMEPAD)”），两个外壳共用同一份手柄绑定；
 *       按钮顺序：0 A、1 B、2 X、3 Y、4 左肩键、5 右肩键、6 左扳机、7 右扳机、8 Back、9 Start、10 左摇杆按下、11 右摇杆按下、12 到 15 方向键上下左右、16 Guide（XInput 9.1.0 读不到，恒为未按）；
 *       轴顺序：左摇杆 X、Y，右摇杆 X、Y；向上与向左为负。非 XInput 的 USB 手柄由 JoystickGamepads 读。
 * ------------------------------------------------------------------------
 */

#pragma once

#include <string>
#include <vector>

namespace ember::win32::platform {

/** 一个手柄此刻的状态。 */
struct GamepadReading {
  /** 手柄位置（0 到 3）。 */
  int index{0};
  std::string id;
  std::vector<bool> buttons;
  std::vector<double> axes;
};

/** XInput 手柄读取。 */
class XInputGamepads {
 public:
  XInputGamepads();
  ~XInputGamepads();
  XInputGamepads(const XInputGamepads&) = delete;
  XInputGamepads& operator=(const XInputGamepads&) = delete;

  /** XInput 库是否可用。 */
  [[nodiscard]] bool available() const { return getState_ != nullptr; }

  /** 读一轮：返回现在连着的手柄，没有库或没有手柄时为空。 */
  [[nodiscard]] std::vector<GamepadReading> read();

 private:
  void* module_{nullptr};
  void* getState_{nullptr};
  /** 没有手柄时，隔几次才再查一次已经断开的位置，免得每帧都去问系统。 */
  unsigned idlePolls_{0};
};

}  // namespace ember::win32::platform
