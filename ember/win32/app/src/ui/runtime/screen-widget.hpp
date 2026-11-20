/*
 * ------------------------------------------------------------------------
 * 名称：screen-widget.hpp
 * 说明：显示区里的运行画面：在显示区里居中放一个按显示比例缩放的画面框（不变形，剩余的地方是页面底色），画面框里是 Direct3D 画面子窗口；可聚焦，持有焦点（且窗口在前台）时采集键盘（学习机与接入 Family BASIC 键盘时独占，经典 FC 不独占）；采集鼠标并交给输入端口；按状态叠上提示位图。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-19
 * 备注：规则见 docs/05-界面.md 第 2、7 节：
 *       画面框的显示宽高比固定为标准帧（256x240）在当前画面比例下的显示宽高比（默认 128:105；关闭“显示全部 240 行”时按 256x224 计算），尽量放大；
 *       点击画面捕获鼠标（首选项“点击画面捕获鼠标”，且机器带鼠标）：第一次点击只用来捕获，不把这次按下送给机器；没有捕获时鼠标不进画面；双击右键释放；
 *       开启且机器在运行或暂停时，画面右下角的提示框里显示“点击画面捕获鼠标，双击右键释放”；键盘独占时同一个提示框里显示“键盘已被机器独占，快捷键无效，请用鼠标操作菜单”；
 *       两个提示 4 秒后一起消失，机器断电或重新开机后再次显示；
 *       不画键盘焦点框（画面区持有焦点才能把按键转给机器）。
 * ------------------------------------------------------------------------
 */

#pragma once

#include <cstdint>
#include <functional>

#include "contracts/input/screen-input.hpp"
#include "contracts/runtime/runtime-model.hpp"
#include "contracts/runtime/screen-host.hpp"
#include "rujian/ui/widget.hpp"
#include "ui/runtime/screen-overlay-view.hpp"

namespace ember::win32::ui {

/** 运行画面。 */
class ScreenWidget final : public rujian::ui::Widget {
 public:
  /** 三者都必须比本控件活得久。 */
  ScreenWidget(contracts::RuntimeModel& model, contracts::ScreenHost& host, contracts::ScreenInput& input);
  ~ScreenWidget() override;

  /** 窗口是否在前台：不在前台时关闭键盘采集与鼠标捕获。 */
  void setWindowActive(bool active);
  /** 把键盘焦点交给画面区（操作结束后键盘才能继续控制游戏）。 */
  void focusScreen();
  /** 画面子窗口此刻的矩形（设备像素，测试用）。 */
  [[nodiscard]] const shared::ScreenRect& deviceRect() const { return deviceRect_; }
  /** 鼠标捕获与键盘独占的提示此刻是否显示（测试用）。 */
  [[nodiscard]] bool captureHintShown() const { return mouseHintShown_; }
  [[nodiscard]] bool keyboardHintShown() const { return keyboardHintShown_; }
  [[nodiscard]] bool mouseCaptured() const { return mouseCaptured_; }

  [[nodiscard]] rujian::ui::Size measure(rujian::ui::Size available) override { return available; }
  void arrange(const rujian::ui::Rect& bounds) override;
  void paint(rujian::ui::Canvas& canvas) override;
  [[nodiscard]] bool focusable() const override { return true; }
  bool onMouseDown(const rujian::ui::MouseEvent& event) override;
  bool onMouseUp(const rujian::ui::MouseEvent& event) override;
  bool onMouseMove(const rujian::ui::MouseEvent& event) override;
  bool onMouseWheel(const rujian::ui::MouseEvent& event) override;
  void onStateChanged() override;
  void onTimer(unsigned id) override;

 private:
  void refresh();
  void updateBox();
  void updateKeyboard();
  void updateOverlay();
  void releaseMouse();
  void updateHints();
  /** 窗口坐标（逻辑像素）换算成画面子窗口里的设备像素。 */
  [[nodiscard]] rujian::ui::Point toDevice(rujian::ui::Point position) const;
  [[nodiscard]] static int buttonBit(rujian::ui::MouseButton button);

  contracts::RuntimeModel& model_;
  contracts::ScreenHost& host_;
  contracts::ScreenInput& input_;
  std::function<void()> unsubscribe_;
  contracts::RuntimeView view_;
  /** 画面框（逻辑像素，窗口坐标）与对应的设备像素矩形。 */
  rujian::ui::Rect box_;
  shared::ScreenRect deviceRect_;
  bool windowActive_{true};
  contracts::KeyboardMode keyboardMode_{contracts::KeyboardMode::Off};
  bool mouseCaptured_{false};
  /** 鼠标捕获与键盘独占的提示：是否显示、上一次是否满足显示条件，以及共用的计时器（两个提示一起消失）。 */
  bool mouseHintShown_{false};
  bool mouseHintEligible_{false};
  bool keyboardHintShown_{false};
  bool keyboardHintEligible_{false};
  unsigned hintTimer_{0};
  /** 鼠标现在按住的按键位，以及指针是否在画面里。 */
  int buttons_{0};
  bool inside_{false};
  /** 上一次画出的提示位图的内容，没变就不重画。 */
  struct OverlayKey {
    OverlayState state{OverlayState::Hidden};
    bool mouseHint{false};
    bool keyboardHint{false};
    int width{0};
    int height{0};
    double scale{0};
    [[nodiscard]] bool operator==(const OverlayKey&) const = default;
  };
  OverlayKey overlayKey_;
  bool overlayDrawn_{false};
};

}  // namespace ember::win32::ui
