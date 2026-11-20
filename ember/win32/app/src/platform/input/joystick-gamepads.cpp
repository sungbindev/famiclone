/*
 * ------------------------------------------------------------------------
 * 名称：joystick-gamepads.cpp
 * 说明：通用游戏控制器读取的实现。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-20
 * 备注：没有任何手柄时每个画面帧不去问系统，隔 60 次才重新找一遍；有手柄时每帧只读已找到的，读不到就当拔掉，下一次重新找时再恢复。
 * ------------------------------------------------------------------------
 */

#include "platform/input/joystick-gamepads.hpp"

#include <windows.h>

#include <mmsystem.h>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cwctype>
#include <set>

#include "platform/text/wide-text.hpp"

namespace ember::win32::platform {

namespace {

constexpr unsigned kMaxJoysticks = 12;
/** 手柄位置从 4 开始，0 到 3 留给 XInput。 */
constexpr int kIndexBase = 4;
constexpr unsigned kRescanInterval = 60;
constexpr double kDeadzone = 0.2;
constexpr int kAxisCount = 6;
constexpr int kHatAxisX = 6;
constexpr int kHatAxisY = 7;
/** 方向帽的角度（百分之一度）：0 为上，顺时针。 */
constexpr DWORD kHatRightFrom = 4500;
constexpr DWORD kHatRightTo = 13500;
constexpr DWORD kHatLeftFrom = 22500;
constexpr DWORD kHatLeftTo = 31500;

/** 取出 HID 设备路径里 VID_xxxx 与 PID_xxxx 的值，格式不对时返回 0。 */
std::uint32_t hardwareKey(const std::wstring& path) {
  const auto field = [&path](const wchar_t* tag) -> long {
    const auto at = path.find(tag);
    if (at == std::wstring::npos || at + 8 > path.size()) return -1;
    return std::wcstol(path.substr(at + 4, 4).c_str(), nullptr, 16);
  };
  const long vendor = field(L"VID_");
  const long product = field(L"PID_");
  return vendor < 0 || product < 0 ? 0 : (static_cast<std::uint32_t>(vendor) << 16) | static_cast<std::uint32_t>(product);
}

/** 现在连着的 XInput 手柄的硬件标识（HID 路径里带 IG_ 的设备）。 */
std::set<std::uint32_t> xinputHardware() {
  std::set<std::uint32_t> keys;
  UINT count = 0;
  if (GetRawInputDeviceList(nullptr, &count, sizeof(RAWINPUTDEVICELIST)) != 0 || count == 0) return keys;
  std::vector<RAWINPUTDEVICELIST> devices(count);
  if (GetRawInputDeviceList(devices.data(), &count, sizeof(RAWINPUTDEVICELIST)) == static_cast<UINT>(-1)) return keys;
  for (UINT index = 0; index < count; ++index) {
    if (devices[index].dwType != RIM_TYPEHID) continue;
    UINT length = 0;
    if (GetRawInputDeviceInfoW(devices[index].hDevice, RIDI_DEVICENAME, nullptr, &length) != 0 || length == 0) continue;
    std::wstring path(length, L'\0');
    if (GetRawInputDeviceInfoW(devices[index].hDevice, RIDI_DEVICENAME, path.data(), &length) == static_cast<UINT>(-1)) continue;
    std::wstring upper = path;
    std::transform(upper.begin(), upper.end(), upper.begin(), [](wchar_t ch) { return static_cast<wchar_t>(std::towupper(ch)); });
    if (upper.find(L"IG_") == std::wstring::npos) continue;
    const std::uint32_t key = hardwareKey(upper);
    if (key != 0) keys.insert(key);
  }
  return keys;
}

/** 轴的值转成 -1 到 1，死区内为 0。 */
double axisValue(DWORD value, unsigned low, unsigned high) {
  if (high <= low) return 0.0;
  const double scaled = std::clamp((static_cast<double>(value) - low) * 2.0 / (static_cast<double>(high) - low) - 1.0, -1.0, 1.0);
  return std::abs(scaled) <= kDeadzone ? 0.0 : scaled;
}

}  // namespace

void JoystickGamepads::rescan() {
  devices_.clear();
  std::set<std::uint32_t> skipped;
  bool skippedLoaded = false;
  for (UINT joyId = 0; joyId < std::min(kMaxJoysticks, joyGetNumDevs()); ++joyId) {
    JOYINFOEX info{};
    info.dwSize = sizeof(info);
    info.dwFlags = JOY_RETURNALL;
    if (joyGetPosEx(joyId, &info) != JOYERR_NOERROR) continue;
    JOYCAPSW caps{};
    if (joyGetDevCapsW(joyId, &caps, sizeof(caps)) != JOYERR_NOERROR) continue;
    if (!skippedLoaded) {
      skipped = xinputHardware();
      skippedLoaded = true;
    }
    if (skipped.count((static_cast<std::uint32_t>(caps.wMid) << 16) | caps.wPid) != 0) continue;
    Device device;
    device.joyId = joyId;
    char tail[48];
    std::snprintf(tail, sizeof(tail), " (Vendor: %04x Product: %04x)", static_cast<unsigned>(caps.wMid), static_cast<unsigned>(caps.wPid));
    device.id = narrow(caps.szPname) + tail;
    device.buttonCount = static_cast<int>(std::min<UINT>(caps.wNumButtons, 32));
    const UINT lows[kAxisCount] = {caps.wXmin, caps.wYmin, caps.wZmin, caps.wRmin, caps.wUmin, caps.wVmin};
    const UINT highs[kAxisCount] = {caps.wXmax, caps.wYmax, caps.wZmax, caps.wRmax, caps.wUmax, caps.wVmax};
    const UINT flags[kAxisCount] = {0, 0, JOYCAPS_HASZ, JOYCAPS_HASR, JOYCAPS_HASU, JOYCAPS_HASV};
    for (int axis = 0; axis < kAxisCount; ++axis) {
      device.hasAxis[axis] = flags[axis] == 0 || (caps.wCaps & flags[axis]) != 0;
      device.range[axis][0] = lows[axis];
      device.range[axis][1] = highs[axis];
    }
    device.hasHat = (caps.wCaps & JOYCAPS_HASPOV) != 0;
    devices_.push_back(std::move(device));
  }
}

bool JoystickGamepads::poll(const Device& device, GamepadReading& reading) {
  JOYINFOEX info{};
  info.dwSize = sizeof(info);
  info.dwFlags = JOY_RETURNALL;
  if (joyGetPosEx(device.joyId, &info) != JOYERR_NOERROR) return false;
  reading.index = kIndexBase + static_cast<int>(device.joyId);
  reading.id = device.id;
  reading.buttons.assign(static_cast<std::size_t>(device.buttonCount), false);
  for (int button = 0; button < device.buttonCount; ++button) reading.buttons[static_cast<std::size_t>(button)] = (info.dwButtons & (1UL << button)) != 0;
  const DWORD values[kAxisCount] = {info.dwXpos, info.dwYpos, info.dwZpos, info.dwRpos, info.dwUpos, info.dwVpos};
  reading.axes.assign(kHatAxisY + 1, 0.0);
  for (int axis = 0; axis < kAxisCount; ++axis) {
    if (device.hasAxis[axis]) reading.axes[static_cast<std::size_t>(axis)] = axisValue(values[axis], device.range[axis][0], device.range[axis][1]);
  }
  // 方向帽居中时系统给 0xFFFF（有的驱动给 0xFFFFFFFF）
  const DWORD hat = info.dwPOV;
  if (device.hasHat && LOWORD(hat) != 0xFFFF) {
    if (hat >= kHatRightFrom && hat <= kHatRightTo) reading.axes[kHatAxisX] = 1.0;
    if (hat >= kHatLeftFrom && hat <= kHatLeftTo) reading.axes[kHatAxisX] = -1.0;
    if (hat <= kHatRightFrom || hat >= kHatLeftTo) reading.axes[kHatAxisY] = -1.0;
    if (hat >= kHatRightTo && hat <= kHatLeftFrom) reading.axes[kHatAxisY] = 1.0;
  }
  return true;
}

std::vector<GamepadReading> JoystickGamepads::read() {
  if (polls_++ % kRescanInterval == 0) rescan();
  std::vector<GamepadReading> pads;
  std::vector<Device> alive;
  for (const Device& device : devices_) {
    GamepadReading reading;
    if (!poll(device, reading)) continue;
    pads.push_back(std::move(reading));
    alive.push_back(device);
  }
  devices_ = std::move(alive);
  return pads;
}

}  // namespace ember::win32::platform
