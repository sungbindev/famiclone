/*
 * ------------------------------------------------------------------------
 * 名称：frameless-frame.hpp
 * 说明：自绘标题栏窗口的非客户区行为：用 WM_NCCALCSIZE 把标题栏并入客户区，用 WM_NCHITTEST 把缩放边框、标题栏空白与三个系统按钮的点击交还给系统。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-19
 * 备注：窗口保留系统的边框样式（WS_OVERLAPPEDWINDOW），所以缩放、最大化、停靠、贴边、系统菜单与 Windows 11 的贴靠布局都由系统处理；
 *       命中测试对系统按钮返回 HTMINBUTTON、HTMAXBUTTON、HTCLOSE，对标题栏空白返回 HTCAPTION，控件上返回 HTCLIENT；
 *       阴影用 DWM（动态加载 dwmapi，系统关闭窗口合成时没有阴影，由调用方自绘 1 像素边框）；
 *       只用 Windows 7 自带的接口，比它新的（按显示器 DPI 取系统尺寸）运行时检测，没有就退回系统 DPI；必须在窗口所在的线程使用。
 * ------------------------------------------------------------------------
 */

#pragma once

#include <cstdint>
#include <functional>

namespace ember::win32::platform {

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
};

/** 自绘标题栏窗口的非客户区行为的选项。 */
struct FramelessOptions {
  /** 窗口的最小尺寸（逻辑像素）。 */
  double minimumWidth{0};
  double minimumHeight{0};
  /** 取窗口当前的缩放比例（设备像素 / 逻辑像素）。 */
  std::function<double()> scale;
};

/** 自绘标题栏窗口的非客户区行为。 */
class FramelessFrame {
 public:
  /**
   * @param window 窗口句柄（HWND），窗口要有 WS_OVERLAPPEDWINDOW 样式。
   */
  FramelessFrame(void* window, FramelessOptions options, FramelessCallbacks callbacks);

  /**
   * 接管窗口：开启 DWM 阴影，并让系统按 handle 的处理重算非客户区。
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

 private:
  void setSystemButtonState(FrameHit hover, FrameHit pressed);
  [[nodiscard]] unsigned long hitTest(int screenX, int screenY) const;

  void* window_;
  FramelessOptions options_;
  FramelessCallbacks callbacks_;
  FrameHit hover_{FrameHit::Client};
  FrameHit pressed_{FrameHit::Client};
  bool trackingLeave_{false};
  bool lastMaximized_{false};
};

}  // namespace ember::win32::platform
