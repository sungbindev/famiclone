/*
 * ------------------------------------------------------------------------
 * 名称：pad-rules.hpp
 * 说明：手柄设置用到的纯规则：手柄的 10 个键、物理键与手柄按钮的显示文字、绑定的修改（键盘键在两个玩家之间不重复）、系统发现的手柄与设备选项、学习绑定时从手柄状态的变化里认出被按下的按钮或轴，以及玩家槽位与核心 JSON 的互转。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-20
 * 备注：规则见 docs/05-界面.md 第 4.6 节；所有修改都返回新的槽位数组，不改传入的数据；不依赖界面与请求，便于单独测试。
 * ------------------------------------------------------------------------
 */

#pragma once

#include <array>
#include <optional>
#include <string>
#include <vector>

#include <nlohmann/json.hpp>

#include "contracts/input/screen-input.hpp"

namespace ember::win32::logic {

/** 手柄的键盘绑定：一个手柄键对应一个物理键位。 */
struct PadKeyBinding {
  std::string button;
  std::string code;
  bool operator==(const PadKeyBinding&) const = default;
};

/** 实体手柄上一个手柄键的绑定：一个按钮，或一个轴的一个方向。 */
struct PadGamepadBinding {
  std::string button;
  /** button 或 axis。 */
  std::string kind;
  int index{0};
  /** 轴的方向：negative 或 positive；按钮没有。 */
  std::string direction;
  bool operator==(const PadGamepadBinding&) const = default;
};

/** 一个玩家指定的实体手柄。 */
struct PadGamepadAssignment {
  std::string id;
  /** 同一个设备标识连着多个时，按编号从小到大排的第几个（从 0 开始）。 */
  int occurrence{0};
  std::vector<PadGamepadBinding> bindings;
  bool operator==(const PadGamepadAssignment&) const = default;
};

/** 一个玩家槽位（手柄口）。 */
struct PadSlot {
  std::vector<PadKeyBinding> keys;
  std::optional<PadGamepadAssignment> gamepad;
  bool operator==(const PadSlot&) const = default;
};

/** 系统发现的一个手柄。 */
struct PadDevice {
  std::string id;
  int occurrence{0};
  bool operator==(const PadDevice&) const = default;
};

/** 手柄上被认出的来源（不含它绑给哪个键）。 */
struct PadSource {
  /** button 或 axis。 */
  std::string kind;
  int index{0};
  /** 轴的方向：negative 或 positive；按钮没有。 */
  std::string direction;
  bool operator==(const PadSource&) const = default;
};

/** 学习绑定时用的基准：开始学习那一刻手柄上的按钮与轴。 */
struct PadBaseline {
  std::vector<bool> buttons;
  std::vector<double> axes;
};

/** 手柄的一个键：标识与键名。 */
struct PadButtonInfo {
  const char* id;
  const char* label;
};

/** 手柄的 10 个键，按显示顺序排列。 */
inline constexpr std::array<PadButtonInfo, 10> kPadButtons{{{"up", "上"}, {"down", "下"}, {"left", "左"}, {"right", "右"}, {"a", "A"}, {"b", "B"}, {"turboA", "连发 A"}, {"turboB", "连发 B"}, {"select", "选择"}, {"start", "开始"}}};

/** 手柄键的名称，如“上”“A”“连发 A”；不认识时原样返回。 */
[[nodiscard]] std::string buttonLabel(const std::string& button);

/** 从核心的玩家槽位（input.getBindings 等的 slots）读出各槽位；格式不对时没有值。 */
[[nodiscard]] std::optional<std::vector<PadSlot>> parseSlots(const nlohmann::json& slots);
/** 槽位转成核心接受的 JSON（input.setBindings 的 slots）。 */
[[nodiscard]] nlohmann::json slotsToJson(const std::vector<PadSlot>& slots);

/** 物理键位的显示文字：方向键用箭头，字母与数字直接显示，小键盘、修饰键与常见符号键有中文或符号名。 */
[[nodiscard]] std::string keyLabel(const std::string& code);
/** 核心接受的物理键位：只含字母与数字、不超过 32 个字符。 */
[[nodiscard]] bool isCapturableCode(const std::string& code);
/** 一个键在键盘上绑定的物理键位；没有绑定时没有值。 */
[[nodiscard]] std::optional<std::string> keyOf(const PadSlot& slot, const std::string& button);

/**
 * 把一个物理键绑给手柄的一个键：这个物理键原来绑着的键（两个玩家里的任何一个）先解除，这个键原来的物理键被替换。
 */
[[nodiscard]] std::vector<PadSlot> bindKey(const std::vector<PadSlot>& slots, std::size_t slotIndex, const std::string& button, const std::string& code);
/** 解除手柄的一个键在键盘上的绑定。 */
[[nodiscard]] std::vector<PadSlot> clearKey(const std::vector<PadSlot>& slots, std::size_t slotIndex, const std::string& button);

/** 设备在选项里用的值：占几号与设备标识连在一起。 */
[[nodiscard]] std::string deviceKey(const PadDevice& device);
/** 由选项值还原设备；格式不对时没有值。 */
[[nodiscard]] std::optional<PadDevice> parseDeviceKey(const std::string& value);
/** 设备的显示名：同一个设备标识连着多个时标出第几个，没有连接的注明“未连接”。 */
[[nodiscard]] std::string deviceLabel(const PadDevice& device, bool connected);
/** 从读到的手柄得到系统发现的手柄（只含已连接的），按编号从小到大排，并算出各自是同一个设备标识里的第几个。 */
[[nodiscard]] std::vector<PadDevice> listDevices(const std::vector<contracts::GamepadSample>& pads);
/** 找出设备对应的手柄：同一个设备标识里按编号排的第 occurrence 个已连接的手柄。 */
[[nodiscard]] const contracts::GamepadSample* findPad(const std::vector<contracts::GamepadSample>& pads, const PadDevice& device);

/**
 * 给一个玩家指定实体手柄，或取消指定（device 没有值）：这个手柄原来在另一个玩家名下的话先从那里取消；
 * 换成别的手柄时原来的按钮绑定清空（不同手柄的按钮排布不同），指定的还是同一个手柄时保留。
 */
[[nodiscard]] std::vector<PadSlot> assignDevice(const std::vector<PadSlot>& slots, std::size_t slotIndex, const std::optional<PadDevice>& device);
/** 一个玩家指定的实体手柄上，手柄的一个键的绑定；没有时没有值。 */
[[nodiscard]] std::optional<PadGamepadBinding> bindingOf(const std::optional<PadGamepadAssignment>& assignment, const std::string& button);
/** 实体手柄上一个来源的显示文字：按钮几，或轴几的哪个方向。 */
[[nodiscard]] std::string sourceLabel(const PadSource& source);
/** 没有绑定时显示的文字。 */
[[nodiscard]] std::string unboundText();
/** 把实体手柄上的一个来源绑给手柄的一个键；这个键原来的来源被替换，没有指定实体手柄时不改。 */
[[nodiscard]] std::vector<PadSlot> bindGamepad(const std::vector<PadSlot>& slots, std::size_t slotIndex, const std::string& button, const PadSource& source);
/** 解除手柄的一个键在实体手柄上的绑定。 */
[[nodiscard]] std::vector<PadSlot> clearGamepad(const std::vector<PadSlot>& slots, std::size_t slotIndex, const std::string& button);

/** 取手柄此刻的按钮与轴，作为学习绑定的基准。 */
[[nodiscard]] PadBaseline baselineOf(const contracts::GamepadSample& pad);
/**
 * 从手柄状态相对基准的变化里认出被按下的按钮或轴：先看按钮（从没按下变成按下），再看轴（离开基准超过 0.5 且绝对值超过 0.5，方向取符号）；编号超出核心接受范围的不认。
 */
[[nodiscard]] std::optional<PadSource> detectSource(const PadBaseline& baseline, const contracts::GamepadSample& pad);

}  // namespace ember::win32::logic
