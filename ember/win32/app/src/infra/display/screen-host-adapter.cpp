/*
 * ------------------------------------------------------------------------
 * 名称：screen-host-adapter.cpp
 * 说明：运行画面宿主的实现。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-19
 * 备注：无。
 * ------------------------------------------------------------------------
 */

#include "infra/display/screen-host-adapter.hpp"

#include <algorithm>
#include <numeric>

#include "platform/display/screen-window.hpp"
#include "platform/input/raw-input.hpp"
#include "platform/input/gamepads.hpp"
#include "platform/threading/ui-thread.hpp"
#include "shared/input/scan-code-table.hpp"

namespace ember::win32::infra {

namespace {

/** 捕获鼠标期间，两次右键按下的间隔不超过这个毫秒数就算“双击右键”，释放捕获。 */
constexpr std::chrono::milliseconds kDoubleRightClick{400};
/** 读手柄的间隔：约一个画面帧。 */
constexpr unsigned kGamepadPollMilliseconds = 16;

}  // namespace

ScreenHostAdapter::ScreenHostAdapter(void* parentWindow, contracts::FrameSource& frames, contracts::ScreenInput& input, platform::UiThread& ui, std::function<void(const std::string&)> onError)
    : frames_(frames), input_(input), ui_(ui), onError_(std::move(onError)) {
  window_ = std::make_unique<platform::ScreenWindow>(parentWindow);
  rawInput_ = std::make_unique<platform::RawInputCapture>(
      parentWindow, [this](const platform::RawKeyEvent& event) { handleRawKey(event.make, event.extended, event.prefixE1, event.up); },
      [this](const platform::RawMouseEvent& event) { handleRawMouse(event.dx, event.dy, event.buttons, event.pressed, event.released, event.wheel); });
  gamepads_ = std::make_unique<platform::Gamepads>();
  frames_.setOnFrame([this] { showLatestFrame(); });
  schedulePoll();
}

ScreenHostAdapter::~ScreenHostAdapter() {
  frames_.setOnFrame({});
  if (pollTimer_ != 0) ui_.cancelDelayed(pollTimer_);
  input_.releaseAllKeys();
  input_.disconnectGamepads();
}

void ScreenHostAdapter::setBounds(const shared::ScreenRect& rect) { window_->setBounds(rect); }

void ScreenHostAdapter::setView(shared::ScreenAspect aspect, shared::ScreenScaling scaling, shared::ScreenFilter filter, bool showAllLines) {
  aspect_ = aspect;
  scaling_ = scaling;
  cropLines_ = shared::cropLinesFor(showAllLines);
  window_->setView(aspect, scaling, filter, cropLines_);
}

double ScreenHostAdapter::nominalRatio() const { return shared::nominalScreenRatio(aspect_, cropLines_); }

void ScreenHostAdapter::setFrameVisible(bool visible) { window_->setFrameVisible(visible); }

void ScreenHostAdapter::setOverlay(const std::uint8_t* pixels, int width, int height, const std::vector<shared::ScreenRect>& glass, int glassRadius) { window_->setOverlay(pixels, width, height, glass, glassRadius); }

void ScreenHostAdapter::setHoles(const std::vector<shared::ScreenRect>& holes) { window_->setHoles(holes); }

bool ScreenHostAdapter::handleMessage(unsigned message, std::uintptr_t wParam, std::intptr_t lParam, std::intptr_t& result) { return rawInput_->handle(message, wParam, lParam, result); }

std::optional<shared::FramePoint> ScreenHostAdapter::toFramePoint(double x, double y) const {
  const shared::ScreenRect bounds = window_->bounds();
  shared::PointerView view;
  view.canvasWidth = bounds.width;
  view.canvasHeight = bounds.height;
  view.aspect = aspect_;
  view.scaling = scaling_;
  view.cropLines = cropLines_;
  return shared::toFramePoint(x, y, view);
}

void ScreenHostAdapter::setKeyboardMode(contracts::KeyboardMode mode) {
  const bool captured = mode != contracts::KeyboardMode::Off;
  const bool exclusive = mode == contracts::KeyboardMode::Exclusive;
  if (captured == rawInput_->keyboardCaptured() && exclusive == rawInput_->keyboardExclusive()) return;
  if (!captured) input_.releaseAllKeys();
  // 重新开始采集时不知道此刻哪些修饰键按着，从“都没按”算起
  else if (!rawInput_->keyboardCaptured()) commandModifiers_ = 0;
  shellKeysDown_ = 0;
  rawInput_->setKeyboardCapture(captured, exclusive);
  if (rawInput_->keyboardCaptured() != captured || rawInput_->keyboardExclusive() != exclusive) onError_("设置键盘采集方式失败");
}

bool ScreenHostAdapter::setMouseCaptured(bool captured) {
  if (captured == rawInput_->mouseCaptured()) return rawInput_->mouseCaptured();
  if (captured) {
    const shared::ScreenRect area = window_->screenBounds();
    if (area.width <= 0) return false;
    haveRightDown_ = false;
    if (!rawInput_->setMouseCapture(true, area.x + area.width / 2, area.y + area.height / 2)) onError_("开启鼠标捕获失败");
  } else {
    rawInput_->setMouseCapture(false, 0, 0);
    // 释放捕获时松开所有鼠标按键，之后没有再次捕获前鼠标的事件都不送给机器
    input_.mouseLeave(0);
  }
  return rawInput_->mouseCaptured();
}

void ScreenHostAdapter::showLatestFrame() {
  if (!frames_.takeLatest(frame_)) return;
  window_->setFrame(frame_.pixels.data(), frame_.width, frame_.height);
  window_->render();
  if (recordLatency_) latencies_.push_back(std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - frame_.publishedAt).count());
}

DisplayLatency ScreenHostAdapter::latency() const {
  DisplayLatency summary;
  if (latencies_.empty()) return summary;
  std::vector<double> sorted = latencies_;
  std::sort(sorted.begin(), sorted.end());
  summary.count = sorted.size();
  summary.average = std::accumulate(sorted.begin(), sorted.end(), 0.0) / static_cast<double>(sorted.size());
  summary.p95 = sorted[std::min(sorted.size() - 1, static_cast<std::size_t>(static_cast<double>(sorted.size()) * 0.95))];
  summary.max = sorted.back();
  return summary;
}

void ScreenHostAdapter::handleRawKey(std::uint16_t make, bool extended, bool prefixE1, bool up) {
  const auto code = shared::scanCodeToKeyCode(make, extended, prefixE1);
  if (!code) return;
  const unsigned modifier = shared::commandModifierBit(*code);
  if (modifier != 0) commandModifiers_ = up ? (commandModifiers_ & ~modifier) : (commandModifiers_ | modifier);
  // 独占时 F9 到 F12 不交给机器，直接交给外壳（独占期间窗口收不到键盘消息）
  if (rawInput_->keyboardExclusive() && shared::isAlwaysShellKey(*code)) {
    // 键位名 F9 到 F12：F 后面的数字就是位号
    unsigned number = static_cast<unsigned>((*code)[1] - '0');
    if (code->size() == 3) number = number * 10 + static_cast<unsigned>((*code)[2] - '0');
    const unsigned bit = 1u << number;
    if (up) shellKeysDown_ &= ~bit;
    else if ((shellKeysDown_ & bit) == 0) {
      shellKeysDown_ |= bit;
      if (onShellKey_) onShellKey_(*code);
    }
    return;
  }
  // 不独占时留给外壳的键不上报；松开照常上报（没按下过的键输入端口会忽略），避免按下后再按修饰键导致卡键
  if (!up && !rawInput_->keyboardExclusive() && shared::isShellKey(*code, commandModifiers_)) return;
  input_.nativeKey(std::string(*code), !up);
}

void ScreenHostAdapter::handleRawMouse(int dx, int dy, int buttons, int pressed, int released, double wheel) {
  shared::PointerView view;
  const shared::ScreenRect bounds = window_->bounds();
  view.canvasWidth = bounds.width;
  view.canvasHeight = bounds.height;
  view.aspect = aspect_;
  view.scaling = scaling_;
  view.cropLines = cropLines_;
  const auto units = shared::frameUnitsPerPixel(view);
  if ((dx != 0 || dy != 0) && units) input_.mouseMoveBy(contracts::MouseKind::Move, dx * units->x, dy * units->y, buttons, 0);
  if (pressed != 0) {
    if ((pressed & platform::kRawMouseRight) != 0) {
      const auto now = std::chrono::steady_clock::now();
      const bool doubled = haveRightDown_ && now - lastRightDown_ <= kDoubleRightClick;
      haveRightDown_ = !doubled;
      lastRightDown_ = now;
      if (doubled) {
        setMouseCaptured(false);
        if (onMouseReleased_) onMouseReleased_();
        return;
      }
    }
    input_.mouseMoveBy(contracts::MouseKind::Down, 0, 0, buttons, 0);
  }
  if (released != 0) input_.mouseMoveBy(contracts::MouseKind::Up, 0, 0, buttons, 0);
  // 原始输入里向上滚是正，输入端口里向下滚是正
  if (wheel != 0) input_.mouseMoveBy(contracts::MouseKind::Wheel, 0, 0, buttons, -wheel);
}

void ScreenHostAdapter::pollGamepads() {
  std::vector<contracts::GamepadSample> samples;
  for (auto& reading : gamepads_->read()) {
    contracts::GamepadSample sample;
    sample.index = reading.index;
    sample.id = std::move(reading.id);
    sample.connected = true;
    sample.buttons = std::move(reading.buttons);
    sample.axes = std::move(reading.axes);
    samples.push_back(std::move(sample));
  }
  input_.pollGamepads(samples);
}

void ScreenHostAdapter::schedulePoll() {
  pollTimer_ = ui_.postDelayed(kGamepadPollMilliseconds, [this] {
    pollTimer_ = 0;
    pollGamepads();
    schedulePoll();
  });
}

}  // namespace ember::win32::infra
