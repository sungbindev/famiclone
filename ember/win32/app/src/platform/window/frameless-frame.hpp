/*
 * ------------------------------------------------------------------------
 * 名称：frameless-frame.hpp
 * 说明：自绘标题栏窗口的非客户区行为：用 WM_NCCALCSIZE 把标题栏并入客户区，用 WM_NCHITTEST 把缩放边框、标题栏空白与三个系统按钮的点击交还给系统。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-19
 * 备注：接管窗口时去掉标题栏样式（WS_CAPTION），保留缩放边框、系统菜单、最小化与最大化按钮样式，所以缩放、最大化、停靠、贴边、系统菜单与 Windows 11 的贴靠布局仍由系统处理；
 *       不去掉标题栏样式的话，系统会把缩放边框的经典样式（浅蓝底、白线）画在窗口内容之上，运行画面子窗口盖住的地方就会露出这一圈；
 *       命中测试对系统按钮返回 HTMINBUTTON、HTMAXBUTTON、HTCLOSE，对标题栏空白返回 HTCAPTION，控件上返回 HTCLIENT；
 *       阴影有两种做法：Windows 11 用系统的（DWM 阴影与圆角，边框色经 setBorderColor 设成界面的边框色）；其余系统（Windows 7、8、10）用自绘模式 customFrame：
 *       关掉系统的非客户区绘制（系统不再画标题栏、按钮与玻璃边框，窗口按下、移动、切换激活时也不会冒出系统的框架），阴影由 WindowShadow 画在主窗口后面的分层窗口里，1 像素边框由调用方画；
 *       三个系统按钮由本类自己跟踪鼠标（去掉标题栏样式后系统不再跟踪它们，Windows 7 上系统的跟踪还会画出系统样式的按钮），松开时在按钮上才发出最小化、最大化（还原）与关闭的系统命令；
 *       最大化时客户区取显示器的工作区；
 *       只用 Windows 7 自带的接口，比它新的（按显示器 DPI 取系统尺寸）运行时检测，没有就退回系统 DPI；必须在窗口所在的线程使用。
 * ------------------------------------------------------------------------
 */

#pragma once

#include <cstdint>
#include <functional>
#include <memory>

namespace ember::win32::platform {

class WindowShadow;

/** 窗口客户区里一个位置的命中结果。 */
enum class FrameHit {
  /** 控件上，由控件自己处理。 */
  Client,
  /** 标题栏空白，用来拖动窗口。 */
  Caption,
  Minimize,
  Maximize,
  Close
};

/** 非客户区事件的回调，都在窗口所在的线程调用，可以为空。 */
struct FramelessCallbacks {
  /** 客户区里一个位置（逻辑像素）的命中结果。 */
  std::function<FrameHit(double x, double y)> hitTest;
  /** 系统按钮的悬停与按下状态变化（不是系统按钮时为 Client）。 */
  std::function<void(FrameHit hover, FrameHit pressed)> systemButtonState;
  /** 窗口最大化或还原。 */
  std::function<void(bool maximized)> maximizedChanged;
  /** 窗口激活或失去激活。 */
  std::function<void(bool active)> activeChanged;
  /** 窗口位置变化（设备像素，窗口左上角）；最大化与最小化时不报告。 */
  std::function<void(int x, int y)> moved;
  /** 系统的明暗主题设置变化。 */
  std::function<void()> systemThemeChanged;
  /** 系统的窗口合成（DWM）开关变化。 */
  std::function<void(bool enabled)> compositionChanged;
  /** 在非客户区（标题栏、系统按钮、缩放边框）按下了鼠标键：界面据此关掉打开着的菜单等弹层，因为这些按下不会作为客户区的鼠标消息到达控件。 */
  std::function<void()> nonClientPressed;
};

/** 自绘标题栏窗口的非客户区行为的选项。 */
struct FramelessOptions {
  /** 窗口的最小尺寸（逻辑像素）。 */
  double minimumWidth{0};
  double minimumHeight{0};
  /** 取窗口当前的缩放比例（设备像素 / 逻辑像素）。 */
  std::function<double()> scale;
  /** 自绘模式：不用系统的框架与阴影，阴影与按钮跟踪都由本类负责，边框由调用方画。 */
  bool customFrame{false};
};

/** 自绘标题栏窗口的非客户区行为。 */
class FramelessFrame {
 public:
  /**
   * @param window 窗口句柄（HWND），窗口要有 WS_OVERLAPPEDWINDOW 样式（接管时会去掉其中的标题栏样式）。
   */
  FramelessFrame(void* window, FramelessOptions options, FramelessCallbacks callbacks);
  ~FramelessFrame();
  FramelessFrame(const FramelessFrame&) = delete;
  FramelessFrame& operator=(const FramelessFrame&) = delete;

  /**
   * 接管窗口：自绘模式关掉系统的非客户区绘制并创建阴影，否则开启 DWM 阴影；再让系统按 handle 的处理重算非客户区。
   * 必须在 handle 已经接到窗口的消息钩子之后调用，重算时系统立即发 WM_NCCALCSIZE。
   */
  void attach();

  /**
   * 处理窗口消息，作为窗口宿主的消息钩子使用。
   * @returns 已处理时为 true，result 是窗口过程的返回值。
   */
  bool handle(unsigned message, std::uintptr_t wParam, std::intptr_t lParam, std::intptr_t& result);

  /** 系统的窗口合成现在是否开启（Windows 7 的基本主题没有）。 */
  [[nodiscard]] static bool compositionEnabled();

  /** 当前系统应该用自绘模式吗：Windows 11 之前的系统（7、8、10）没有圆角与边框色，用自绘的阴影与边框。 */
  [[nodiscard]] static bool prefersCustomFrame();

  /** 全屏时没有缩放边框，整个窗口都是客户区，阴影也隐藏。 */
  void setFullScreen(bool fullScreen);

  /** 自绘模式吗。 */
  [[nodiscard]] bool customFrame() const { return options_.customFrame; }

  /** 系统画的窗口边框的颜色（Windows 11 用，其他系统没有这个设置，什么都不做）；颜色是 0x00BBGGRR。 */
  void setBorderColor(unsigned colorRef);

 private:
  void setSystemButtonState(FrameHit hover, FrameHit pressed);
  [[nodiscard]] unsigned long hitTest(int screenX, int screenY) const;
  void beginButtonTracking(FrameHit button);
  void updateButtonTracking(int clientX, int clientY);
  void endButtonTracking(int clientX, int clientY, bool commit);

  void* window_;
  FramelessOptions options_;
  FramelessCallbacks callbacks_;
  std::unique_ptr<WindowShadow> shadow_;
  FrameHit hover_{FrameHit::Client};
  FrameHit pressed_{FrameHit::Client};
  /** 自绘模式下正在跟踪的系统按钮；没有时为 Client。 */
  FrameHit tracked_{FrameHit::Client};
  bool trackingLeave_{false};
  bool lastMaximized_{false};
  bool fullScreen_{false};
};

}  // namespace ember::win32::platform
