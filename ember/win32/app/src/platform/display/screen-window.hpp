/*
 * ------------------------------------------------------------------------
 * 名称：screen-window.hpp
 * 说明：运行画面的子窗口：主窗口里的一个子窗口，用 Direct3D 11 显示核心给出的 RGBA 帧，按画面比例与缩放方式居中放大，周围黑底，上面可以叠一张提示位图。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-19
 * 备注：规则见 docs/08-Win32原生外壳.md 第 5.5 节：每帧把像素上传到动态纹理，适应窗口用双线性采样，整数倍用最近邻采样；交换链用 DXGI 1.0 的位块传送模式（Windows 7 没有翻转模型），
 *       设备功能级别最低取 9_1，只用硬件设备，创建不出时构造函数抛出异常，由组装根弹出提示并退出，不做 GDI 降级、不用 WARP；着色器是预先编译好的字节码（screen-shaders.hpp）。
 *       子窗口对鼠标透明（WM_NCHITTEST 返回 HTTRANSPARENT），鼠标与键盘都由主窗口的控件处理；主窗口的弹层（菜单、通知、悬停提示）盖到画面上时，由 setHoles 把子窗口对应的区域挖掉；
 *       必须在窗口所在的线程使用；本头文件不含任何 Direct3D 与 Windows 类型。
 * ------------------------------------------------------------------------
 */

#pragma once

#include <cstdint>
#include <memory>
#include <vector>

#include "shared/screen/screen-layout.hpp"

namespace ember::win32::platform {

/** 运行画面的子窗口。 */
class ScreenWindow {
 public:
  /**
   * 在父窗口里创建画面子窗口（先不显示，设置位置后显示）。
   * @param parentWindow 父窗口句柄（HWND）。
   * @throws std::runtime_error 创建窗口失败，或创建不出硬件的 Direct3D 11 设备（没有显卡、没有驱动、虚拟机没有 3D 加速）。
   */
  explicit ScreenWindow(void* parentWindow);
  ~ScreenWindow();
  ScreenWindow(const ScreenWindow&) = delete;
  ScreenWindow& operator=(const ScreenWindow&) = delete;

  /** 窗口句柄（HWND）。 */
  [[nodiscard]] void* handle() const;

  /** 放到父窗口客户区里的这个矩形（设备像素）；宽或高为 0 时隐藏。 */
  void setBounds(const shared::ScreenRect& rect);
  /** 子窗口现在的大小（设备像素），也就是画布的大小。 */
  [[nodiscard]] shared::ScreenRect bounds() const;
  /** 子窗口在屏幕上的矩形（屏幕坐标，设备像素）；还没有显示时宽高为 0。 */
  [[nodiscard]] shared::ScreenRect screenBounds() const;

  /** 显示方式：画面比例、缩放方式与上下各裁掉的行数。 */
  void setView(shared::ScreenAspect aspect, shared::ScreenScaling scaling, int cropLines);

  /** 把这些矩形（父窗口客户区坐标，设备像素）从子窗口里挖掉，弹层才能显示在画面之上。 */
  void setHoles(const std::vector<shared::ScreenRect>& holes);

  /**
   * 上传新的一帧（只上传，要调用 render 才显示）。
   * @param pixels 像素，每个 4 字节，内存顺序 R、G、B、A，按行从上到下，共 width × height 个。
   */
  void setFrame(const std::uint32_t* pixels, std::uint32_t width, std::uint32_t height);
  /** 画面是否显示：没有开机与断电时机器没有画面，不显示纹理里残留的上一帧。 */
  void setFrameVisible(bool visible);

  /**
   * 设置叠在画面上的提示位图，铺满子窗口；空指针清除。
   * @param pixels 预乘透明度的 BGRA 像素，每行紧密排列，共 width × height 个。
   * @param glass 提示框的矩形（设备像素，相对子窗口）：画面在这块区域里先模糊（毛玻璃）再叠上位图；宽或高为 0 表示没有。
   */
  void setOverlay(const std::uint8_t* pixels, int width, int height, const shared::ScreenRect& glass);

  /** 把当前内容画出来并显示。 */
  void render();

 private:
  struct Impl;
  std::unique_ptr<Impl> impl_;
};

}  // namespace ember::win32::platform
