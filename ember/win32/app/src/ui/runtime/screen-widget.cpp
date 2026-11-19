/*
 * ------------------------------------------------------------------------
 * 名称：screen-widget.cpp
 * 说明：运行画面控件的实现。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-19
 * 备注：无。
 * ------------------------------------------------------------------------
 */

#include "ui/runtime/screen-widget.hpp"

#include <algorithm>
#include <cmath>

#include "rujian/ui/canvas.hpp"
#include "rujian/ui/offscreen.hpp"
#include "rujian/ui/root.hpp"

namespace ember::win32::ui {

namespace {

/** 捕获鼠标的提示显示几秒后消失，不一直遮挡画面。 */
constexpr unsigned kCaptureHintMilliseconds = 4000;

OverlayState overlayStateOf(contracts::MachineState state) {
  switch (state) {
    case contracts::MachineState::None: return OverlayState::NotPoweredOn;
    case contracts::MachineState::Off: return OverlayState::PoweredOff;
    case contracts::MachineState::Paused: return OverlayState::Paused;
    case contracts::MachineState::Unknown:
    case contracts::MachineState::Running: break;
  }
  return OverlayState::Hidden;
}

}  // namespace

ScreenWidget::ScreenWidget(contracts::RuntimeModel& model, contracts::ScreenHost& host, contracts::ScreenInput& input) : model_(model), host_(host), input_(input), view_(model.view()) {
  unsubscribe_ = model_.subscribe([this] { refresh(); });
  model_.setScreenFocus([this] { focusScreen(); });
  host_.setOnMouseReleased([this] {
    mouseCaptured_ = false;
    buttons_ = 0;
  });
  host_.setView(view_.screen.aspect, view_.screen.scaling, view_.screen.showAllLines);
  host_.setFrameVisible(false);
}

ScreenWidget::~ScreenWidget() {
  if (unsubscribe_) unsubscribe_();
  model_.setScreenFocus({});
  host_.setOnMouseReleased({});
  if (hintTimer_ != 0 && host() != nullptr) host()->stopTimer(hintTimer_);
  host_.setKeyboardExclusive(false);
  host_.setMouseCaptured(false);
  host_.setOverlay(nullptr, 0, 0);
  host_.setBounds(shared::ScreenRect{});
}

void ScreenWidget::setWindowActive(bool active) {
  if (windowActive_ == active) return;
  windowActive_ = active;
  updateKeyboard();
}

void ScreenWidget::focusScreen() {
  if (host() != nullptr) host()->setFocus(this, false);
}

void ScreenWidget::arrange(const rujian::ui::Rect& bounds) {
  Widget::arrange(bounds);
  updateBox();
}

void ScreenWidget::paint(rujian::ui::Canvas& canvas) {
  // 画面子窗口盖在这里；它还没有显示（或被弹层挖开）的地方是黑底
  if (box_.width > 0 && box_.height > 0) canvas.fillRect(box_, rujian::ui::Color::rgba(0, 0, 0));
}

void ScreenWidget::updateBox() {
  if (host() == nullptr) return;
  const rujian::ui::Rect area = bounds();
  const double scale = host()->scale();
  const double ratio = host_.nominalRatio();
  const double width = std::min(area.width, area.height * ratio);
  const double height = ratio > 0 ? width / ratio : 0;
  box_ = rujian::ui::Rect{area.x + (area.width - width) / 2, area.y + (area.height - height) / 2, std::max(0.0, width), std::max(0.0, height)};
  // 子窗口的位置与大小取整到设备像素：左上角四舍五入，右下角也四舍五入，相邻内容之间不留缝
  const int left = static_cast<int>(std::lround(box_.x * scale));
  const int top = static_cast<int>(std::lround(box_.y * scale));
  const int right = static_cast<int>(std::lround(box_.right() * scale));
  const int bottom = static_cast<int>(std::lround(box_.bottom() * scale));
  deviceRect_ = (right > left && bottom > top) ? shared::ScreenRect{left, top, right - left, bottom - top} : shared::ScreenRect{};
  host_.setBounds(deviceRect_);
  updateOverlay();
  invalidate();
}

void ScreenWidget::refresh() {
  const contracts::RuntimeView previous = view_;
  view_ = model_.view();
  host_.setView(view_.screen.aspect, view_.screen.scaling, view_.screen.showAllLines);
  host_.setFrameVisible(view_.state == contracts::MachineState::Paused || view_.state == contracts::MachineState::Running);
  // 不再带鼠标（或关掉了“点击画面捕获鼠标”）时，已有的捕获立即释放
  if (!view_.capturesMouse && mouseCaptured_) releaseMouse();
  showCaptureHint();
  if (previous.screen.aspect != view_.screen.aspect || previous.screen.showAllLines != view_.screen.showAllLines) updateBox();
  else updateOverlay();
}

void ScreenWidget::showCaptureHint() {
  const bool eligible = view_.capturesMouse && view_.state != contracts::MachineState::None && view_.state != contracts::MachineState::Off;
  if (eligible == hintEligible_) return;
  hintEligible_ = eligible;
  if (hintTimer_ != 0 && host() != nullptr) host()->stopTimer(hintTimer_);
  hintTimer_ = 0;
  captureHintShown_ = eligible;
  if (eligible && host() != nullptr) hintTimer_ = host()->startTimer(this, kCaptureHintMilliseconds);
}

void ScreenWidget::onTimer(unsigned id) {
  if (id != hintTimer_) return;
  hintTimer_ = 0;
  captureHintShown_ = false;
  updateOverlay();
}

void ScreenWidget::updateOverlay() {
  if (host() == nullptr) return;
  OverlayKey key;
  key.state = overlayStateOf(view_.state);
  key.hint = captureHintShown_;
  key.width = deviceRect_.width;
  key.height = deviceRect_.height;
  key.scale = host()->scale();
  if (overlayDrawn_ && key == overlayKey_) return;
  overlayKey_ = key;
  overlayDrawn_ = true;
  if ((key.state == OverlayState::Hidden && !key.hint) || key.width <= 0 || key.height <= 0) {
    host_.setOverlay(nullptr, 0, 0);
    return;
  }
  const double scale = key.scale;
  const rujian::ui::Size logical{key.width / scale, key.height / scale};
  rujian::ui::Root root(rujian::ui::ThemeName::Dark, scale);
  root.setContent(std::make_unique<ScreenOverlayView>(key.state, key.hint));
  root.layout(logical);
  rujian::ui::OffscreenSurface surface(logical, scale);
  if (!surface.render(root, rujian::ui::Color::rgba(0, 0, 0, 0))) {
    host_.setOverlay(nullptr, 0, 0);
    return;
  }
  const auto pixels = surface.pixels();
  host_.setOverlay(pixels.data(), surface.pixelWidth(), surface.pixelHeight());
}

void ScreenWidget::updateKeyboard() {
  // 持有焦点且窗口在前台时才独占键盘：独占期间应用里所有窗口都收不到键盘消息
  const bool want = focused() && windowActive_;
  if (want != keyboardExclusive_) {
    keyboardExclusive_ = want;
    host_.setKeyboardExclusive(want);
  }
  if (!want && mouseCaptured_) releaseMouse();
}

void ScreenWidget::onStateChanged() {
  Widget::onStateChanged();
  updateKeyboard();
  // 指针离开画面：按最后的位置上报一次 leave（按着键拖动时等松开之后）
  if (!hovered() && inside_ && buttons_ == 0 && !mouseCaptured_) {
    inside_ = false;
    if (!view_.capturesMouse) input_.mouseLeave(0);
  }
}

void ScreenWidget::releaseMouse() {
  host_.setMouseCaptured(false);
  mouseCaptured_ = false;
  buttons_ = 0;
}

rujian::ui::Point ScreenWidget::toDevice(rujian::ui::Point position) const {
  const double scale = host() != nullptr ? host()->scale() : 1.0;
  return rujian::ui::Point{position.x * scale - deviceRect_.x, position.y * scale - deviceRect_.y};
}

int ScreenWidget::buttonBit(rujian::ui::MouseButton button) {
  switch (button) {
    case rujian::ui::MouseButton::Left: return contracts::kMouseButtonLeft;
    case rujian::ui::MouseButton::Right: return contracts::kMouseButtonRight;
    case rujian::ui::MouseButton::Middle: return contracts::kMouseButtonMiddle;
    case rujian::ui::MouseButton::None: break;
  }
  return 0;
}

bool ScreenWidget::onMouseDown(const rujian::ui::MouseEvent& event) {
  if (mouseCaptured_) return true;
  focusScreen();
  if (view_.capturesMouse) {
    // 第一次点击只用来捕获，不把这次按下送给机器
    mouseCaptured_ = host_.setMouseCaptured(true);
    return true;
  }
  const int bit = buttonBit(event.button);
  buttons_ |= bit;
  // 按住拖到画面之外再松开，松开事件也要到这里
  if (host() != nullptr) host()->captureMouse(this);
  const rujian::ui::Point device = toDevice(event.position);
  if (const auto point = host_.toFramePoint(device.x, device.y)) input_.mouseAt(contracts::MouseKind::Down, *point, buttons_, 0);
  return true;
}

bool ScreenWidget::onMouseUp(const rujian::ui::MouseEvent& event) {
  if (mouseCaptured_ || view_.capturesMouse) return true;
  buttons_ &= ~buttonBit(event.button);
  const rujian::ui::Point device = toDevice(event.position);
  if (const auto point = host_.toFramePoint(device.x, device.y)) input_.mouseAt(contracts::MouseKind::Up, *point, buttons_, 0);
  if (buttons_ == 0 && host() != nullptr) host()->releaseMouse(this);
  return true;
}

bool ScreenWidget::onMouseMove(const rujian::ui::MouseEvent& event) {
  inside_ = true;
  // 开启了鼠标捕获的机器，没捕获时鼠标不进画面，里面的鼠标不跟着动
  if (mouseCaptured_ || view_.capturesMouse) return true;
  const rujian::ui::Point device = toDevice(event.position);
  if (const auto point = host_.toFramePoint(device.x, device.y)) input_.mouseAt(contracts::MouseKind::Move, *point, buttons_, 0);
  return true;
}

bool ScreenWidget::onMouseWheel(const rujian::ui::MouseEvent& event) {
  if (mouseCaptured_ || view_.capturesMouse) return true;
  const rujian::ui::Point device = toDevice(event.position);
  // 输入端口里向下滚是正，控件库里向上滚是正
  if (const auto point = host_.toFramePoint(device.x, device.y)) input_.mouseAt(contracts::MouseKind::Wheel, *point, buttons_, -event.wheelDelta);
  return true;
}

}  // namespace ember::win32::ui
