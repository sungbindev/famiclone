/*
 * ------------------------------------------------------------------------
 * 名称：input-controller.cpp
 * 说明：输入控制器的实现。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-19
 * 备注：无。
 * ------------------------------------------------------------------------
 */

#include "logic/input/input-controller.hpp"

#include <algorithm>
#include <cmath>

namespace ember::win32::logic {

namespace {

/** 上报的按键数与轴数的上限，与核心宿主的校验一致；超出的部分不上报。 */
constexpr std::size_t kMaxButtons = 32;
constexpr std::size_t kMaxAxes = 16;
/** 设备标识上报的长度上限，与核心宿主的校验一致；超出的部分截掉。 */
constexpr std::size_t kMaxIdLength = 256;
/** 轴的值保留的小数位：两位。 */
constexpr double kAxisScale = 100;

const char* mouseKindName(contracts::MouseKind kind) {
  switch (kind) {
    case contracts::MouseKind::Move: return "move";
    case contracts::MouseKind::Down: return "down";
    case contracts::MouseKind::Up: return "up";
    case contracts::MouseKind::Wheel: return "wheel";
    case contracts::MouseKind::Leave: return "leave";
  }
  return "move";
}

/** UTF-8 字符串截到不超过 limit 个字节且不切断字符。 */
std::string truncateUtf8(const std::string& text, std::size_t limit) {
  if (text.size() <= limit) return text;
  std::size_t end = limit;
  while (end > 0 && (static_cast<unsigned char>(text[end]) & 0xC0) == 0x80) --end;
  return text.substr(0, end);
}

}  // namespace

void InputController::nativeKey(const std::string& code, bool down) {
  if (code.empty()) return;
  if (!down) {
    if (held_.erase(code) != 0) core_.sendInput({{"type", "key"}, {"code", code}, {"down", false}});
    return;
  }
  if (!held_.insert(code).second) return;
  core_.sendInput({{"type", "key"}, {"code", code}, {"down", true}});
}

void InputController::releaseAllKeys() {
  const auto keys = held_;
  held_.clear();
  for (const auto& code : keys) core_.sendInput({{"type", "key"}, {"code", code}, {"down", false}});
}

void InputController::sendMouse(contracts::MouseKind kind, double x, double y, double dx, double dy, int buttons, double wheel) {
  core_.sendInput({{"type", "mouse"}, {"kind", mouseKindName(kind)}, {"x", x}, {"y", y}, {"dx", shared::roundCoordinate(dx)}, {"dy", shared::roundCoordinate(dy)}, {"buttons", buttons & 7}, {"wheel", wheel}});
}

void InputController::mouseAt(contracts::MouseKind kind, const shared::FramePoint& point, int buttons, double wheel) {
  const shared::FramePoint previous = last_.value_or(point);
  last_ = point;
  sendMouse(kind, point.x, point.y, point.x - previous.x, point.y - previous.y, buttons, wheel);
}

void InputController::mouseMoveBy(contracts::MouseKind kind, double dx, double dy, int buttons, double wheel) {
  const shared::FramePoint origin = last_.value_or(shared::FramePoint{shared::kNominalFrameWidth / 2.0, shared::kNominalFrameHeight / 2.0});
  const shared::FramePoint point{shared::roundCoordinate(std::min<double>(shared::kNominalFrameWidth, std::max(0.0, origin.x + dx))),
                                 shared::roundCoordinate(std::min<double>(shared::kNominalFrameHeight, std::max(0.0, origin.y + dy)))};
  last_ = point;
  sendMouse(kind, point.x, point.y, dx, dy, buttons, wheel);
}

void InputController::mouseLeave(int buttons) {
  const shared::FramePoint point = last_.value_or(shared::FramePoint{0, 0});
  sendMouse(contracts::MouseKind::Leave, point.x, point.y, 0, 0, buttons, 0);
  last_.reset();
}

void InputController::reportGamepad(const contracts::GamepadSample& pad) {
  nlohmann::json buttons = nlohmann::json::array();
  for (std::size_t i = 0; i < std::min(pad.buttons.size(), kMaxButtons); ++i) buttons.push_back(static_cast<bool>(pad.buttons[i]));
  nlohmann::json axes = nlohmann::json::array();
  for (std::size_t i = 0; i < std::min(pad.axes.size(), kMaxAxes); ++i) axes.push_back(std::floor(std::max(-1.0, std::min(1.0, pad.axes[i])) * kAxisScale + 0.5) / kAxisScale);
  const std::string id = truncateUtf8(pad.id, kMaxIdLength);
  const nlohmann::json message{{"type", "gamepad"}, {"index", pad.index}, {"id", id}, {"connected", pad.connected}, {"buttons", buttons}, {"axes", axes}};
  const std::string text = message.dump();
  const auto found = reported_.find(pad.index);
  if (found != reported_.end() && found->second == text) return;
  reported_[pad.index] = text;
  ids_[pad.index] = id;
  core_.sendInput(message);
}

void InputController::disconnectGamepad(int index) {
  const auto id = ids_.find(index);
  core_.sendInput({{"type", "gamepad"}, {"index", index}, {"id", id == ids_.end() ? std::string() : id->second}, {"connected", false}, {"buttons", nlohmann::json::array()}, {"axes", nlohmann::json::array()}});
  reported_.erase(index);
  ids_.erase(index);
}

void InputController::pollGamepads(const std::vector<contracts::GamepadSample>& pads) {
  std::set<int> present;
  for (const auto& pad : pads) {
    present.insert(pad.index);
    reportGamepad(pad);
  }
  std::vector<int> gone;
  for (const auto& [index, text] : reported_) {
    if (present.find(index) == present.end()) gone.push_back(index);
  }
  for (const int index : gone) disconnectGamepad(index);
}

void InputController::disconnectGamepads() {
  std::vector<int> indexes;
  for (const auto& [index, text] : reported_) indexes.push_back(index);
  for (const int index : indexes) disconnectGamepad(index);
}

}  // namespace ember::win32::logic
