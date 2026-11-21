/*
 * ------------------------------------------------------------------------
 * 名称：pad-rules.cpp
 * 说明：手柄设置纯规则的实现。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-20
 * 备注：无。
 * ------------------------------------------------------------------------
 */

#include "logic/input/pad-rules.hpp"

#include <algorithm>
#include <cmath>
#include <map>

namespace ember::win32::logic {

namespace {

/** 轴被认为按下的阈值，与核心一致。 */
constexpr double kAxisThreshold = 0.5;
/** 核心接受的按钮与轴的编号上限。 */
constexpr int kMaxButtonIndex = 31;
constexpr int kMaxAxisIndex = 15;

/** 方向键与常见符号键的显示名。 */
const std::map<std::string, std::string>& keyNames() {
  static const std::map<std::string, std::string> names{
      {"ArrowUp", "↑"},     {"ArrowDown", "↓"},     {"ArrowLeft", "←"},     {"ArrowRight", "→"},   {"Enter", "回车"},        {"Space", "空格"},      {"Backspace", "退格"},  {"Tab", "Tab"},
      {"Escape", "Esc"},    {"Delete", "Delete"},   {"Insert", "Insert"},   {"Home", "Home"},      {"End", "End"},           {"PageUp", "PageUp"},   {"PageDown", "PageDown"}, {"CapsLock", "CapsLock"},
      {"Backquote", "`"},   {"Minus", "-"},         {"Equal", "="},         {"BracketLeft", "["},  {"BracketRight", "]"},    {"Backslash", "\\"},    {"Semicolon", ";"},     {"Quote", "'"},
      {"Comma", ","},       {"Period", "."},        {"Slash", "/"}};
  return names;
}

bool startsWith(const std::string& text, const std::string& prefix) { return text.rfind(prefix, 0) == 0; }

bool isButtonName(const std::string& button) {
  return std::any_of(kPadButtons.begin(), kPadButtons.end(), [&button](const PadButtonInfo& info) { return button == info.id; });
}

std::string stringField(const nlohmann::json& object, const char* name) {
  const auto found = object.find(name);
  return found != object.end() && found->is_string() ? found->get<std::string>() : std::string();
}

std::optional<int> integerField(const nlohmann::json& object, const char* name) {
  const auto found = object.find(name);
  if (found == object.end() || !found->is_number_integer()) return std::nullopt;
  return found->get<int>();
}

/** 替换第 slotIndex 个槽位。 */
template <typename Change>
std::vector<PadSlot> withSlot(const std::vector<PadSlot>& slots, std::size_t slotIndex, Change change) {
  std::vector<PadSlot> result = slots;
  if (slotIndex < result.size()) result[slotIndex] = change(result[slotIndex]);
  return result;
}

bool sameDevice(const PadDevice& left, const PadGamepadAssignment& right) { return left.id == right.id && left.occurrence == right.occurrence; }

}  // namespace

std::string buttonLabel(const std::string& button) {
  for (const PadButtonInfo& info : kPadButtons) {
    if (button == info.id) return info.label;
  }
  return button;
}

std::optional<std::vector<PadSlot>> parseSlots(const nlohmann::json& slots) {
  if (!slots.is_array()) return std::nullopt;
  std::vector<PadSlot> result;
  for (const auto& item : slots) {
    if (!item.is_object()) return std::nullopt;
    PadSlot slot;
    const auto keys = item.find("keys");
    if (keys == item.end() || !keys->is_array()) return std::nullopt;
    for (const auto& key : *keys) {
      if (!key.is_object() || !isButtonName(stringField(key, "button")) || stringField(key, "code").empty()) return std::nullopt;
      slot.keys.push_back({stringField(key, "button"), stringField(key, "code")});
    }
    const auto gamepad = item.find("gamepad");
    if (gamepad != item.end() && !gamepad->is_null()) {
      if (!gamepad->is_object() || stringField(*gamepad, "id").empty() || !integerField(*gamepad, "occurrence")) return std::nullopt;
      PadGamepadAssignment assignment;
      assignment.id = stringField(*gamepad, "id");
      assignment.occurrence = *integerField(*gamepad, "occurrence");
      const auto bindings = gamepad->find("bindings");
      if (bindings == gamepad->end() || !bindings->is_array()) return std::nullopt;
      for (const auto& binding : *bindings) {
        const std::string kind = binding.is_object() ? stringField(binding, "kind") : std::string();
        if (!binding.is_object() || !isButtonName(stringField(binding, "button")) || (kind != "button" && kind != "axis") || !integerField(binding, "index")) return std::nullopt;
        assignment.bindings.push_back({stringField(binding, "button"), kind, *integerField(binding, "index"), stringField(binding, "direction")});
      }
      slot.gamepad = std::move(assignment);
    }
    result.push_back(std::move(slot));
  }
  return result;
}

nlohmann::json slotsToJson(const std::vector<PadSlot>& slots) {
  nlohmann::json result = nlohmann::json::array();
  for (const PadSlot& slot : slots) {
    nlohmann::json item = {{"keys", nlohmann::json::array()}};
    for (const PadKeyBinding& key : slot.keys) item["keys"].push_back({{"button", key.button}, {"code", key.code}});
    if (slot.gamepad) {
      nlohmann::json bindings = nlohmann::json::array();
      for (const PadGamepadBinding& binding : slot.gamepad->bindings) {
        nlohmann::json entry = {{"button", binding.button}, {"kind", binding.kind}, {"index", binding.index}};
        if (binding.kind == "axis" && !binding.direction.empty()) entry["direction"] = binding.direction;
        bindings.push_back(std::move(entry));
      }
      item["gamepad"] = {{"id", slot.gamepad->id}, {"occurrence", slot.gamepad->occurrence}, {"bindings", std::move(bindings)}};
    }
    result.push_back(std::move(item));
  }
  return result;
}

std::string keyLabel(const std::string& code) {
  const auto known = keyNames().find(code);
  if (known != keyNames().end()) return known->second;
  if (code.size() == 4 && startsWith(code, "Key") && code[3] >= 'A' && code[3] <= 'Z') return code.substr(3);
  if (code.size() == 6 && startsWith(code, "Digit") && code[5] >= '0' && code[5] <= '9') return code.substr(5);
  if (code.size() > 6 && startsWith(code, "Numpad")) return "小键盘 " + code.substr(6);
  for (const char* modifier : {"Shift", "Control", "Alt", "Meta"}) {
    const std::string name = modifier;
    if (code == name + "Left") return "左 " + name;
    if (code == name + "Right") return "右 " + name;
  }
  return code;
}

bool isCapturableCode(const std::string& code) {
  return !code.empty() && code.size() <= 32 && std::all_of(code.begin(), code.end(), [](char c) { return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9'); });
}

std::optional<std::string> keyOf(const PadSlot& slot, const std::string& button) {
  for (const PadKeyBinding& key : slot.keys) {
    if (key.button == button) return key.code;
  }
  return std::nullopt;
}

std::vector<PadSlot> bindKey(const std::vector<PadSlot>& slots, std::size_t slotIndex, const std::string& button, const std::string& code) {
  std::vector<PadSlot> freed = slots;
  for (PadSlot& slot : freed) std::erase_if(slot.keys, [&code](const PadKeyBinding& key) { return key.code == code; });
  return withSlot(freed, slotIndex, [&button, &code](PadSlot slot) {
    std::erase_if(slot.keys, [&button](const PadKeyBinding& key) { return key.button == button; });
    slot.keys.push_back({button, code});
    return slot;
  });
}

std::vector<PadSlot> clearKey(const std::vector<PadSlot>& slots, std::size_t slotIndex, const std::string& button) {
  return withSlot(slots, slotIndex, [&button](PadSlot slot) {
    std::erase_if(slot.keys, [&button](const PadKeyBinding& key) { return key.button == button; });
    return slot;
  });
}

std::string deviceKey(const PadDevice& device) { return std::to_string(device.occurrence) + ":" + device.id; }

std::optional<PadDevice> parseDeviceKey(const std::string& value) {
  const auto colon = value.find(':');
  if (colon == std::string::npos || colon == 0 || colon + 1 >= value.size()) return std::nullopt;
  const std::string digits = value.substr(0, colon);
  if (!std::all_of(digits.begin(), digits.end(), [](char c) { return c >= '0' && c <= '9'; }) || digits.size() > 9) return std::nullopt;
  return PadDevice{value.substr(colon + 1), std::stoi(digits)};
}

std::string deviceLabel(const PadDevice& device, bool connected) {
  const std::string name = device.occurrence == 0 ? device.id : device.id + "（第 " + std::to_string(device.occurrence + 1) + " 个）";
  return connected ? name : name + "（未连接）";
}

std::vector<PadDevice> listDevices(const std::vector<contracts::GamepadSample>& pads) {
  std::vector<const contracts::GamepadSample*> connected;
  for (const auto& pad : pads) {
    if (pad.connected) connected.push_back(&pad);
  }
  std::stable_sort(connected.begin(), connected.end(), [](const auto* left, const auto* right) { return left->index < right->index; });
  std::map<std::string, int> counts;
  std::vector<PadDevice> devices;
  for (const auto* pad : connected) {
    const int occurrence = counts[pad->id]++;
    devices.push_back({pad->id, occurrence});
  }
  return devices;
}

const contracts::GamepadSample* findPad(const std::vector<contracts::GamepadSample>& pads, const PadDevice& device) {
  std::vector<const contracts::GamepadSample*> matches;
  for (const auto& pad : pads) {
    if (pad.connected && pad.id == device.id) matches.push_back(&pad);
  }
  std::stable_sort(matches.begin(), matches.end(), [](const auto* left, const auto* right) { return left->index < right->index; });
  return device.occurrence >= 0 && static_cast<std::size_t>(device.occurrence) < matches.size() ? matches[static_cast<std::size_t>(device.occurrence)] : nullptr;
}

std::vector<PadSlot> assignDevice(const std::vector<PadSlot>& slots, std::size_t slotIndex, const std::optional<PadDevice>& device) {
  std::vector<PadSlot> released = slots;
  for (std::size_t index = 0; index < released.size(); ++index) {
    if (index == slotIndex || !device || !released[index].gamepad || !sameDevice(*device, *released[index].gamepad)) continue;
    released[index].gamepad.reset();
  }
  return withSlot(released, slotIndex, [&device](PadSlot slot) {
    if (!device) {
      slot.gamepad.reset();
      return slot;
    }
    std::vector<PadGamepadBinding> keep;
    if (slot.gamepad && sameDevice(*device, *slot.gamepad)) keep = slot.gamepad->bindings;
    slot.gamepad = PadGamepadAssignment{device->id, device->occurrence, std::move(keep)};
    return slot;
  });
}

std::optional<PadGamepadBinding> bindingOf(const std::optional<PadGamepadAssignment>& assignment, const std::string& button) {
  if (!assignment) return std::nullopt;
  for (const PadGamepadBinding& binding : assignment->bindings) {
    if (binding.button == button) return binding;
  }
  return std::nullopt;
}

std::string sourceLabel(const PadSource& source) {
  if (source.kind == "button") return "按钮 " + std::to_string(source.index);
  return "轴 " + std::to_string(source.index) + " " + (source.direction == "negative" ? "负向" : "正向");
}

std::string unboundText() { return "未绑定"; }

std::vector<PadSlot> bindGamepad(const std::vector<PadSlot>& slots, std::size_t slotIndex, const std::string& button, const PadSource& source) {
  return withSlot(slots, slotIndex, [&button, &source](PadSlot slot) {
    if (!slot.gamepad) return slot;
    std::erase_if(slot.gamepad->bindings, [&button](const PadGamepadBinding& binding) { return binding.button == button; });
    slot.gamepad->bindings.push_back({button, source.kind, source.index, source.kind == "axis" ? source.direction : std::string()});
    return slot;
  });
}

std::vector<PadSlot> clearGamepad(const std::vector<PadSlot>& slots, std::size_t slotIndex, const std::string& button) {
  return withSlot(slots, slotIndex, [&button](PadSlot slot) {
    if (slot.gamepad) std::erase_if(slot.gamepad->bindings, [&button](const PadGamepadBinding& binding) { return binding.button == button; });
    return slot;
  });
}

PadBaseline baselineOf(const contracts::GamepadSample& pad) { return PadBaseline{pad.buttons, pad.axes}; }

bool anyButtonHeld(const contracts::GamepadSample& pad) {
  const std::size_t buttons = std::min<std::size_t>(pad.buttons.size(), kMaxButtonIndex + 1);
  for (std::size_t index = 0; index < buttons; ++index) {
    if (pad.buttons[index]) return true;
  }
  return false;
}

PadBaseline relaxBaseline(const PadBaseline& baseline, const contracts::GamepadSample& pad) {
  PadBaseline result = baseline;
  for (std::size_t index = 0; index < result.buttons.size(); ++index) {
    if (index >= pad.buttons.size() || !pad.buttons[index]) result.buttons[index] = false;
  }
  for (std::size_t index = 0; index < result.axes.size(); ++index) {
    const double value = index < pad.axes.size() ? pad.axes[index] : 0.0;
    if (std::abs(value) <= kAxisThreshold) result.axes[index] = value;
  }
  return result;
}

std::optional<PadSource> detectSource(const PadBaseline& baseline, const contracts::GamepadSample& pad) {
  const std::size_t buttons = std::min<std::size_t>(pad.buttons.size(), kMaxButtonIndex + 1);
  for (std::size_t index = 0; index < buttons; ++index) {
    const bool wasPressed = index < baseline.buttons.size() && baseline.buttons[index];
    if (pad.buttons[index] && !wasPressed) return PadSource{"button", static_cast<int>(index), {}};
  }
  const std::size_t axes = std::min<std::size_t>(pad.axes.size(), kMaxAxisIndex + 1);
  for (std::size_t index = 0; index < axes; ++index) {
    const double value = pad.axes[index];
    const double origin = index < baseline.axes.size() ? baseline.axes[index] : 0.0;
    if (std::abs(value) > kAxisThreshold && std::abs(value - origin) > kAxisThreshold) return PadSource{"axis", static_cast<int>(index), value < 0 ? "negative" : "positive"};
  }
  return std::nullopt;
}

}  // namespace ember::win32::logic
