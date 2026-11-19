/*
 * ------------------------------------------------------------------------
 * 名称：host.hpp
 * 说明：控件宿主接口：控件通过它取得主题与缩放比例、度量与定位文字、读写剪贴板、控制光标闪烁、请求重绘、管理焦点与鼠标捕获。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-19
 * 备注：宿主由根容器 Root 实现，窗口与离屏绘制共用同一套实现；控件树挂到 Root 上之后才有宿主。
 *       带 16 的文字接口按 UTF-16 单元计位置，用于文字编辑；其余文字接口按 UTF-8。
 * ------------------------------------------------------------------------
 */

#pragma once

#include <cstddef>
#include <functional>
#include <optional>
#include <string>
#include <string_view>

#include "rujian/ui/geometry.hpp"
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

  /** 度量文字（UTF-8）；maxWidth 为 0 表示不限宽度。 */
  [[nodiscard]] virtual TextMetrics measureText(std::string_view text, const TextStyle& style, double maxWidth) = 0;

  /** 度量文字（UTF-16）；maxWidth 为 0 表示不限宽度。 */
  [[nodiscard]] virtual TextMetrics measureText16(std::u16string_view text, const TextStyle& style, double maxWidth) = 0;

  /** 文字里第 index 个 UTF-16 单元之前的插入点，距文字起点的水平距离（不换行排版）。 */
  [[nodiscard]] virtual double caretOffset(std::u16string_view text, const TextStyle& style, std::size_t index) = 0;

  /** 距文字起点水平距离为 x 的位置最近的插入点（UTF-16 单元序号，不换行排版）。 */
  [[nodiscard]] virtual std::size_t caretIndexAt(std::u16string_view text, const TextStyle& style, double x) = 0;

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

  /** 窗口内容区的大小（逻辑像素），弹层据此避开窗口边缘。 */
  [[nodiscard]] virtual Size viewportSize() const = 0;

  /**
   * 在内容之上显示一个弹层：弹层由调用方拥有（宿主只引用），自己摆放到窗口坐标里；
   * 弹层盖住的区域里，鼠标事件先给弹层，在弹层上按下鼠标不改变键盘焦点。
   * @param layer 弹层控件的根，不能已经在控件树里。
   * @param onOutsidePress 在弹层之外按下鼠标（或窗口大小变化）时回调，参数是被按到的控件（窗口大小变化时为空）；弹层自己决定是否关闭。
   */
  virtual void showLayer(Widget* layer, std::function<void(Widget* hit)> onOutsidePress) = 0;

  /** 关闭弹层；弹层不在显示时无效。 */
  virtual void hideLayer(Widget* layer) = 0;

  /** 请求 milliseconds 毫秒后调用 owner 的 onTimer（一次性）；返回定时器编号。控件离开控件树时它的定时器自动取消。 */
  virtual unsigned startTimer(Widget* owner, unsigned milliseconds) = 0;

  /** 取消还没触发的定时器；编号不存在时无效。 */
  virtual void stopTimer(unsigned id) = 0;

  /** 是否播放渐显渐隐之类的动画；关闭时（离屏截图对照）控件直接切到目标状态。 */
  [[nodiscard]] virtual bool animationsEnabled() const = 0;

  /** 读剪贴板里的文字（UTF-8）；没有文字时为空。 */
  [[nodiscard]] virtual std::optional<std::string> clipboardText() = 0;

  /** 把文字（UTF-8）放进剪贴板。 */
  virtual void setClipboardText(std::string_view text) = 0;

  /** 文字光标此刻是否处于“显示”的闪烁相位。 */
  [[nodiscard]] virtual bool caretOn() const = 0;

  /** 重新开始光标闪烁（输入、移动光标、获得焦点时调用，让光标立刻显示）。 */
  virtual void restartCaretBlink() = 0;
};

}  // namespace rujian::ui
