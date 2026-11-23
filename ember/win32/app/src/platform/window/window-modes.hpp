/*
 * ------------------------------------------------------------------------
 * 名称：window-modes.hpp
 * 说明：主窗口的几种模式：全屏（只显示画面，盖住整个显示器）、窗口始终置顶、接收拖入的文件。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-19
 * 备注：全屏做法：记下窗口的位置与样式，去掉标题与缩放边框的样式，让窗口铺满它所在的显示器（系统据此隐藏任务栏）；退出全屏恢复样式与位置，原来是最大化的仍回到最大化；
 *       置顶用 SetWindowPos 的 HWND_TOPMOST，勾选状态取窗口的真实置顶状态；拖放用 WM_DROPFILES，路径只交给回调，界面拿到的是登记后的编号；必须在窗口所在的线程使用。
 * ------------------------------------------------------------------------
 */

#pragma once

#include <cstdint>
#include <functional>
#include <string>
#include <vector>

namespace ember::win32::platform {

class FramelessFrame;

/** 主窗口的模式。 */
class WindowModes {
 public:
  /**
   * @param window 主窗口句柄（HWND）。
   * @param frame 窗口的非客户区处理，全屏时要告诉它去掉缩放边框。
   * @param onChanged 全屏或置顶状态变化时调用。
   * @param onDropped 拖入文件时调用，参数是文件的路径（UTF-8）。
   */
  WindowModes(void* window, FramelessFrame& frame, std::function<void()> onChanged, std::function<void(std::vector<std::string>)> onDropped);
  ~WindowModes();
  WindowModes(const WindowModes&) = delete;
  WindowModes& operator=(const WindowModes&) = delete;

  void toggleFullScreen();
  void toggleAlwaysOnTop();
  /**
   * 把窗口调成指定大小（设备像素）：全屏时不处理；最大化时先还原；大小不超过所在显示器的工作区，调整后窗口不会超出工作区，左上角尽量保持不动。
   */
  void resizeTo(int width, int height);
  [[nodiscard]] bool fullScreen() const { return fullScreen_; }
  /** 窗口现在是否置顶（取系统里的真实状态）。 */
  [[nodiscard]] bool alwaysOnTop() const;

  /** 处理窗口消息，作为窗口宿主的消息钩子之一使用；只处理 WM_DROPFILES。 */
  bool handle(unsigned message, std::uintptr_t wParam, std::intptr_t lParam, std::intptr_t& result);

 private:
  void* window_;
  FramelessFrame& frame_;
  std::function<void()> onChanged_;
  std::function<void(std::vector<std::string>)> onDropped_;
  bool fullScreen_{false};
  /** 进入全屏前的样式与位置；用不透明的字节保存，免得头文件里出现系统类型。 */
  std::uintptr_t savedStyle_{0};
  alignas(8) unsigned char savedPlacement_[48]{};
};

}  // namespace ember::win32::platform
