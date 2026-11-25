/*
 * ------------------------------------------------------------------------
 * 名称：dialog-window.hpp
 * 说明：对话框窗口：归属于主窗口的独立顶层窗口，内容是控件库的对话框框架（标题栏、正文、按钮区），打开期间主窗口不可用（模态）。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-20
 * 备注：窗口外框（缩放边框、阴影、最小尺寸）与主窗口一样由 FramelessFrame 处理，标题栏空白处拖动窗口；
 *       系统菜单关闭（Alt+F4）、对话框框架的关闭按钮与 Esc 都只请求关闭，由使用方决定是否销毁本对象，窗口自己不会销毁（销毁窗口会让消息循环退出）；
 *       销毁本对象时先恢复主窗口可用再销毁窗口，避免系统把焦点交给别的程序；必须在窗口所在的线程使用。
 * ------------------------------------------------------------------------
 */

#pragma once

#include <functional>
#include <memory>
#include <set>
#include <string>
#include <utility>

#include "platform/window/frameless-frame.hpp"
#include "rujian/ui/dialog-frame.hpp"
#include "rujian/ui/theme.hpp"
#include "rujian/ui/tooltip.hpp"
#include "rujian/ui/window.hpp"

namespace ember::win32::platform {

/** 创建对话框窗口的选项。 */
struct DialogWindowOptions {
  /** 归属窗口（HWND），打开期间它不可用；对话框居中在它上面。 */
  void* owner{nullptr};
  /** 窗口标题（任务栏、Alt+Tab 与系统菜单显示）。 */
  std::wstring title;
  /** 内容区的初始大小（逻辑像素）；高度为 0 表示按内容。最小高度按内容算时，初始高度不小于最小高度；最小宽度按内容算时，初始宽度不小于最小宽度（宽度为 0 就是最小宽度）。 */
  double width{0};
  double height{0};
  /** 最小大小（逻辑像素），0 表示不限。 */
  double minimumWidth{0};
  double minimumHeight{0};
  /** 最小宽度、最小高度按内容算：取正文的最小尺寸（Widget::minimumSize）加标题栏与按钮区，不低于上面的最小大小；初始大小也不小于它。 */
  bool fitMinimumWidth{false};
  bool fitMinimumHeight{false};
  /** 是否有最大化按钮。 */
  bool maximizable{false};
  /** 是否有最小化按钮：模态对话框的归属窗口不可用，所以“最小化”是把归属窗口最小化（对话框跟着隐藏，还原归属窗口时一起回来）；非模态对话框只最小化自己，有自己的任务栏按钮。 */
  bool minimizable{false};
  /** 非模态：打开期间归属窗口仍然可用；窗口本身没有归属窗口，不会总盖在主窗口上面（归属窗口只用来决定打开时的位置）。 */
  bool modeless{false};
  rujian::ui::ThemeName theme{rujian::ui::ThemeName::Dark};
  /** 自绘阴影与边框的模式，见 FramelessFrame。 */
  bool customFrame{false};
  /** 请求关闭：Alt+F4、系统菜单、对话框框架的关闭按钮与 Esc。 */
  std::function<void()> onCloseRequested;
  /**
   * 物理按键拦截（学习键盘绑定用）：每次不重复的按下先交给它，参数是键位名（KeyboardEvent.code 的写法，由扫描码得出，与键盘布局无关）；
   * 返回 true 表示按键被吞掉，不再交给控件，这个键的后续字符与抬起消息也一并吞掉；没有设置时不拦截。
   */
  std::function<bool(const std::string& code)> onPhysicalKey;
  /**
   * 快捷键拦截（设置快捷键用）：每次不重复的按下（含带 Alt 的系统键）先交给它，参数是 Windows 虚拟键码与当时的修饰键（Shift=1、Ctrl=2、Alt=4 的位和）；
   * 返回 true 表示按键被吞掉，不再交给控件，这个键的后续字符与抬起消息也一并吞掉（Alt 键的抬起也吞掉，系统才不会把它当成单按 Alt 去激活系统菜单）；没有设置时不拦截。
   */
  std::function<bool(int virtualKey, unsigned modifiers)> onShortcutKey;
};

/** 对话框窗口。 */
class DialogWindow {
 public:
  /**
   * 创建窗口（尚未显示）。
   * @param options 选项。
   * @param frame 窗口的内容，正文与按钮已设好；窗口接管它。
   */
  DialogWindow(DialogWindowOptions options, std::unique_ptr<rujian::ui::DialogFrame> frame);
  ~DialogWindow();
  DialogWindow(const DialogWindow&) = delete;
  DialogWindow& operator=(const DialogWindow&) = delete;

  /** 显示并让归属窗口不可用。 */
  void show();

  /** 已经显示时把它带到最前面并取得焦点（最小化时先还原）。 */
  void activate();

  /** 切换主题。 */
  void setTheme(rujian::ui::ThemeName theme);

  /** 窗口句柄（HWND）。 */
  [[nodiscard]] void* handle() const;

  /** 消息观察者：窗口收到的每条消息先交给它，返回 true 表示已处理（result 是窗口过程的返回值），不再往下传；用来挂靠停靠等窗口级行为。销毁窗口前要先清除。 */
  void setMessageObserver(std::function<bool(unsigned message, std::uintptr_t wParam, std::intptr_t lParam, std::intptr_t& result)> observer) { messageObserver_ = std::move(observer); }

  /** 设置宽度调整的接管者：resizeWidthBy 算出新宽度（设备像素）后先交给它，返回 true 表示它已经调整了窗口（如停靠时连同主窗一起重新摆放），本窗口不再自己调整。 */
  void setWidthResizer(std::function<bool(int width)> resizer) { widthResizer_ = std::move(resizer); }

  /** 内容。 */
  [[nodiscard]] rujian::ui::DialogFrame& frame() { return *frame_; }

  /** 窗口的根容器。 */
  [[nodiscard]] rujian::ui::Root& root() { return window_.root(); }

  /** 把屏幕坐标（设备像素）换成窗口内容的坐标（逻辑像素）。 */
  [[nodiscard]] rujian::ui::Point clientPointFromScreen(int x, int y);

  /**
   * 把窗口宽度加上 delta（逻辑像素，负数是缩小）：左边缘不动，超出屏幕工作区右边时整体左移，宽度不小于最小宽度、不超过工作区；高度不变。
   * 设置了宽度调整的接管者（见 setWidthResizer）时，算出的新宽度先交给它。
   * @return 窗口处在可以调整的状态（没有最大化、最小化）并已调整时为真。
   */
  bool resizeWidthBy(double delta);

 private:
  void place();
  /** 当前的最小尺寸（逻辑像素的宽、高）：按内容算的方向取内容所需与固定最小值中较大的。 */
  std::pair<double, double> minimumSize();
  void minimize();
  void applyBorderColor();

  DialogWindowOptions options_;
  rujian::ui::HostWindow window_;
  rujian::ui::DialogFrame* frame_{nullptr};
  /** 悬停提示：对话框里的控件用 setTooltip 设置的提示靠它显示；先于窗口销毁。 */
  std::unique_ptr<rujian::ui::Tooltip> tooltip_;
  std::unique_ptr<FramelessFrame> frameless_;
  bool ownerLocked_{false};
  std::function<bool(unsigned, std::uintptr_t, std::intptr_t, std::intptr_t&)> messageObserver_;
  std::function<bool(int)> widthResizer_;
  /** 刚吞掉的那些键的标识（扫描码加扩展位），它们的字符与抬起消息也要吞掉；抬起后移出。 */
  std::set<int> swallowedKeys_;
};

}  // namespace ember::win32::platform
