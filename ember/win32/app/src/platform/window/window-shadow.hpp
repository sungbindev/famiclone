/*
 * ------------------------------------------------------------------------
 * 名称：window-shadow.hpp
 * 说明：窗口的自绘外阴影：一个不能点、不能激活、不出现在任务栏的分层窗口，大小是主窗口四周各多出一圈，叠在主窗口的正后面，随主窗口移动、改变大小、置顶与显示隐藏。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-19
 * 备注：Windows 7 与 8 的无边框窗口没有可靠的系统阴影（关掉系统非客户区绘制后更没有），Windows 10 的系统阴影又很淡，所以这些系统由本类自绘阴影，图像见 shared/window/window-shadow-image.hpp；
 *       主窗口最大化、最小化、全屏或不可见时阴影隐藏；必须在主窗口所在的线程使用。
 * ------------------------------------------------------------------------
 */

#pragma once

namespace ember::win32::platform {

/** 窗口的自绘外阴影。 */
class WindowShadow {
 public:
  /** @param window 主窗口句柄（HWND）。 */
  explicit WindowShadow(void* window);
  ~WindowShadow();
  WindowShadow(const WindowShadow&) = delete;
  WindowShadow& operator=(const WindowShadow&) = delete;

  /** 让阴影与主窗口一致：位置、大小、前后顺序与显示状态；图像只在大小、缩放或激活状态变了时重画。 */
  void sync();
  /** 主窗口是否处于激活状态：激活时阴影深一些。 */
  void setActive(bool active);
  /** 阴影是否应该隐藏（全屏时）；其余的隐藏条件（最大化、最小化、不可见）由 sync 自己判断。 */
  void setSuppressed(bool suppressed);
  /** 缩放比例（设备像素 / 逻辑像素）有变化时更新。 */
  void setScale(double scale);

 private:
  struct Impl;
  Impl* impl_;
};

}  // namespace ember::win32::platform
