/*
 * ------------------------------------------------------------------------
 * 名称：screen-host.hpp
 * 说明：运行画面的宿主：界面层的画面控件经它设置画面子窗口的位置与显示方式、叠上提示位图、换算指针位置，并控制键盘独占与鼠标捕获；实现把这些接到平台层的 Direct3D 子窗口、原始输入与手柄。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-19
 * 备注：位置与大小都是设备像素，相对主窗口的客户区；键盘模式只表达“画面控件现在要怎么采集”，窗口不在前台、失去激活时由使用方先关掉；
 *       鼠标捕获由宿主在双击右键（400 毫秒内两次右键按下）时自动释放，并经 setOnMouseReleased 告诉画面控件。
 * ------------------------------------------------------------------------
 */

#pragma once

#include <cstdint>
#include <functional>
#include <optional>
#include <vector>

#include "shared/screen/pointer-mapping.hpp"
#include "shared/screen/screen-layout.hpp"

namespace ember::win32::contracts {

/** 键盘怎么交给机器：Off 不采集；Shared 采集按键并上报（F 键、Tab、Esc 与带 Ctrl、Alt、Meta 的组合留给外壳，快捷键照常生效）；Exclusive 独占，所有物理按键都只交给机器。 */
enum class KeyboardMode { Off, Shared, Exclusive };

/** 运行画面的宿主。 */
class ScreenHost {
 public:
  virtual ~ScreenHost() = default;

  /** 画面子窗口放到这个矩形（设备像素，相对主窗口客户区）；宽或高为 0 时隐藏。 */
  virtual void setBounds(const shared::ScreenRect& rect) = 0;
  /** 显示方式：画面比例、缩放方式与是否显示全部扫描线（否则上下各裁掉一部分）。 */
  virtual void setView(shared::ScreenAspect aspect, shared::ScreenScaling scaling, bool showAllLines) = 0;
  /** 按当前显示方式，画面（含像素宽高比）的标称宽高比。 */
  [[nodiscard]] virtual double nominalRatio() const = 0;
  /** 画面是否显示：没有开机与断电时机器没有画面，不显示纹理里残留的上一帧。 */
  virtual void setFrameVisible(bool visible) = 0;
  /**
   * 设置叠在画面上的提示位图（预乘透明度的 BGRA，铺满画面子窗口）；空指针清除。
   * @param glass 各个提示框的矩形（设备像素，相对画面子窗口）：画面子窗口先把这些区域里的画面模糊（毛玻璃），再叠上位图；宽或高为 0 的矩形、空列表表示没有。
   * @param glassRadius 提示框的圆角半径（设备像素）。
   */
  virtual void setOverlay(const std::uint8_t* pixels, int width, int height, const std::vector<shared::ScreenRect>& glass, int glassRadius) = 0;

  /** 指针（相对画面子窗口左上角，设备像素）在标准帧上的位置；还没有画面时没有值。 */
  [[nodiscard]] virtual std::optional<shared::FramePoint> toFramePoint(double x, double y) const = 0;

  /** 设置键盘怎么交给机器；窗口不在前台、失去激活时由使用方先设为 Off。 */
  virtual void setKeyboardMode(KeyboardMode mode) = 0;
  /** 开启或关闭鼠标捕获：开启时指针隐藏并只给相对位移；返回之后是否捕获着。 */
  virtual bool setMouseCaptured(bool captured) = 0;
  /** 设置鼠标捕获被自动释放（双击右键）时的回调。 */
  virtual void setOnMouseReleased(std::function<void()> handler) = 0;
};

}  // namespace ember::win32::contracts
