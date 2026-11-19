/*
 * ------------------------------------------------------------------------
 * 名称：host.hpp
 * 说明：控件宿主接口：控件通过它取得主题与缩放比例、度量文字、请求重绘、管理焦点与鼠标捕获。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-19
 * 备注：宿主由根容器 Root 实现，窗口与离屏绘制共用同一套实现；控件树挂到 Root 上之后才有宿主。
 * ------------------------------------------------------------------------
 */

#pragma once

#include <string_view>

#include "rujian/ui/text.hpp"
#include "rujian/ui/theme.hpp"

namespace rujian::ui {

class Widget;

/** 控件宿主。 */
class Host {
 public:
  virtual ~Host() = default;

  /** 当前主题。 */
  [[nodiscard]] virtual ThemeName theme() const = 0;

  /** 缩放比例（设备像素 / 逻辑像素）。 */
  [[nodiscard]] virtual double scale() const = 0;

  /** 请求重绘。 */
  virtual void invalidate() = 0;

  /** 度量文字；maxWidth 为 0 表示不限宽度。 */
  [[nodiscard]] virtual TextMetrics measureText(std::string_view text, const TextStyle& style, double maxWidth) = 0;

  /** 当前有键盘焦点的控件，没有则为空。 */
  [[nodiscard]] virtual Widget* focusedWidget() const = 0;

  /**
   * 设置键盘焦点。
   * @param widget 要获得焦点的控件，空表示清除焦点。
   * @param visible 焦点是否要显示焦点环（键盘切换焦点时为真，鼠标点击时为假）。
   */
  virtual void setFocus(Widget* widget, bool visible) = 0;

  /** 让控件捕获鼠标：之后的移动与抬起都发给它，直到 releaseMouse。 */
  virtual void captureMouse(Widget* widget) = 0;

  /** 释放鼠标捕获；widget 不是当前捕获者时无效。 */
  virtual void releaseMouse(Widget* widget) = 0;

  /** 控件离开控件树（被移除或销毁）时由控件调用，宿主据此清理对它的引用。 */
  virtual void widgetLeft(Widget* widget) = 0;
};

}  // namespace rujian::ui
