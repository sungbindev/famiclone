/*
 * ------------------------------------------------------------------------
 * 名称：resize-border.hpp
 * 说明：窗口的缩放边框：一个看不见的分层窗口，大小是主窗口四周各多出一圈，叠在主窗口的正后面，鼠标在这一圈里按下左键就能拖动改变主窗口的大小。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-20
 * 备注：缩放区只在窗口边框线的外面，边框线以内（含贴着边的滚动条）都是客户区；
 *       窗口的命中测试返回值只在窗口矩形内有效，矩形外的缩放区只能由另一个窗口接住鼠标，按下后向主窗口发带方向的 SC_SIZE，由系统接着做缩放；
 *       窗口以 1/255 的透明度显示（几乎看不见，但能接住鼠标）；主窗口最大化、最小化、全屏、不可见或被禁用（模态对话框打开期间）时，缩放区不接鼠标；必须在主窗口所在的线程使用。
 * ------------------------------------------------------------------------
 */

#pragma once

#include <functional>

namespace ember::win32::platform {

/** 窗口外的缩放边框。 */
class ResizeBorder {
 public:
  /**
   * @param window 主窗口句柄（HWND）。
   * @param onPressed 在缩放区按下了左键（缩放开始前调用），可以为空。
   */
  ResizeBorder(void* window, std::function<void()> onPressed);
  ~ResizeBorder();
  ResizeBorder(const ResizeBorder&) = delete;
  ResizeBorder& operator=(const ResizeBorder&) = delete;

  /** 让缩放区与主窗口一致：位置、大小、前后顺序与显示状态。 @param thickness 缩放区的厚度（设备像素）。 */
  void sync(int thickness);

  /** 缩放区是否应该隐藏（全屏时）；其余的隐藏条件由 sync 自己判断。 */
  void setSuppressed(bool suppressed);

  /** 屏幕上一个位置的命中结果：缩放区里是 HTLEFT 到 HTBOTTOMRIGHT，其余是 HTTRANSPARENT（鼠标交给下面的窗口）。 */
  [[nodiscard]] long hitTest(int screenX, int screenY) const;

  /** 缩放区的窗口句柄（HWND），还没创建时为空。 */
  [[nodiscard]] void* handle() const { return border_; }

  /** 窗口过程收到非客户区的左键按下：命中的是缩放区时，开始缩放主窗口。 */
  void press(unsigned long hit, int screenX, int screenY);

 private:
  bool ensureWindow();

  void* main_;
  void* border_{nullptr};
  std::function<void()> onPressed_;
  int thickness_{0};
  bool suppressed_{false};
};

}  // namespace ember::win32::platform
