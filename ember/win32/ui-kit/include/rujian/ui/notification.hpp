/*
 * ------------------------------------------------------------------------
 * 名称：notification.hpp
 * 说明：通知，对应网页版组件库的 NotificationProvider、NotificationCenter 与 NotificationCenterButton：
 *       新通知在右下角弹出、到时自动收起；通知面板列出本次运行的通知历史（新的在前，带时间，可单条关闭、全部清除）；
 *       状态栏上的通知按钮（铃铛与未读数）；较长的消息只显示摘要，点击后在“通知详情”里看全文。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-19
 * 备注：NotificationCenter 是一个铺满窗口的弹层，只有通知与面板接收鼠标，其余位置的事件穿到下面；窗口大小变化时由弹层的回调重新摆放。
 *       通知详情由使用方经 setOnShowDetail 接入（对话框窗口），本控件只在点击较长的消息时调用它。
 *       弹出的通知停留 6 秒（错误通知 12 秒），最多同时显示 3 条；通知历史最多 100 条；面板打开期间新通知不弹出，直接进入历史。
 * ------------------------------------------------------------------------
 */

#pragma once

#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "rujian/ui/root.hpp"
#include "rujian/ui/widget.hpp"

namespace rujian::ui {

/** 通知级别；除颜色外还以图标形状区分。 */
enum class NoticeLevel { Info, Success, Warning, Error };

/** 一条通知记录。 */
struct NoticeRecord {
  int id{0};
  std::string text;
  NoticeLevel level{NoticeLevel::Info};
  /** 发出的时间，显示为“时:分”。 */
  std::string time;
};

/** 消息是否较长：超过 60 个字符或含换行时只显示摘要，可点击查看全文。 */
[[nodiscard]] bool isLongNotice(const std::string& text);

/** 通知中心：弹出的通知与通知面板。 */
class NotificationCenter final : public Widget {
 public:
  /** @param root 窗口的 Root，必须比本控件活得久；构造时把自己作为弹层显示。 */
  explicit NotificationCenter(Root& root);
  ~NotificationCenter() override;

  /** 发一条通知；空文字不发送。 */
  void notify(std::string text, NoticeLevel level = NoticeLevel::Info);
  /** 通知历史，新的在前。 */
  [[nodiscard]] const std::vector<NoticeRecord>& records() const { return records_; }
  /** 正在弹出的通知，先弹出的在前（显示时在上）。 */
  [[nodiscard]] const std::vector<NoticeRecord>& toasts() const { return toasts_; }
  /** 未读条数：面板关闭期间收到的通知数，打开面板时清零。 */
  [[nodiscard]] int unread() const { return unread_; }
  [[nodiscard]] bool open() const { return open_; }
  /** 打开或关闭面板；打开时收起弹出的通知并把未读清零。 */
  void setOpen(bool open);
  /** 删除一条通知（历史与弹出的都删）。 */
  void remove(int id);
  /** 清除全部通知历史。 */
  void clear();
  /** 收起一条弹出的通知（历史里仍保留）。 */
  void dismissToast(int id);
  /** 在“通知详情”里显示全文：调用使用方设置的回调。 */
  void showDetail(const std::string& text);
  void setOnShowDetail(std::function<void(const std::string&)> handler) { onShowDetail_ = std::move(handler); }

  /** 弹出通知停留的时间（毫秒），默认 6000；错误通知停留两倍时间。 */
  void setToastDuration(unsigned milliseconds) { toastDuration_ = milliseconds; }
  /** 通知与面板离窗口底边的距离，默认 10；应用框架把它设成状态栏之上。 */
  void setBottomOffset(double offset);
  /** 通知按钮：点它不算点在面板之外。 */
  void setToggleWidget(const Widget* widget) { toggleWidget_ = widget; }
  /** 时间来源（返回“时:分”）；不设置时用系统本地时间，测试用它固定时间。 */
  void setTimeSource(std::function<std::string()> source) { timeSource_ = std::move(source); }

  /** 按了 Esc：面板打开时收起并返回 true，由应用的按键处理在其他处理之前调用。 */
  bool handleEscape(int virtualKey);
  /** 窗口失去活动状态：收起面板。 */
  void windowDeactivated();

  /** 面板的区域（窗口坐标）；没打开时为空。 */
  [[nodiscard]] Rect panelRect() const;
  /** 第 index 条弹出通知的区域（窗口坐标）。 */
  [[nodiscard]] Rect toastRect(std::size_t index) const;
  /** 弹出通知的控件（与 toasts 一一对应）。 */
  [[nodiscard]] const std::vector<Widget*>& toastWidgets() const { return toastWidgets_; }
  /** 面板里历史列表的条数（面板没打开或没有历史时为 0）与第 index 条的区域（窗口坐标）。 */
  [[nodiscard]] std::size_t rowCount() const;
  [[nodiscard]] Rect rowRect(std::size_t index) const;

  [[nodiscard]] Widget* hitTest(Point point) override;
  [[nodiscard]] Size measure(Size available) override;
  void arrange(const Rect& bounds) override;
  void onTimer(unsigned id) override;

 private:
  void rebuild();
  void relayout();
  void armTimer(const NoticeRecord& record);
  void clearTimers();
  void onOutsidePress(Widget* hit);

  Root& root_;
  std::vector<NoticeRecord> records_;
  std::vector<NoticeRecord> toasts_;
  int unread_{0};
  bool open_{false};
  int nextId_{1};
  unsigned toastDuration_{6000};
  double bottomOffset_{10.0};
  /** 定时器编号对应的通知编号。 */
  std::vector<std::pair<unsigned, int>> timers_;
  const Widget* toggleWidget_{nullptr};
  std::function<void(const std::string&)> onShowDetail_;
  std::function<std::string()> timeSource_;
  /** 当前的子控件：弹出的通知（与 toasts_ 一一对应）与面板。 */
  std::vector<Widget*> toastWidgets_;
  Widget* panel_{nullptr};
};

/** 状态栏上的通知按钮：铃铛与未读数，点击展开或收起通知面板。 */
class NotificationButton final : public Widget {
 public:
  /** @param center 通知中心，必须比按钮活得久。 */
  explicit NotificationButton(NotificationCenter& center);

  [[nodiscard]] Size measure(Size available) override;
  [[nodiscard]] bool focusable() const override { return true; }
  [[nodiscard]] bool stretchesHeight() const override { return false; }
  [[nodiscard]] CursorKind cursor() const override { return CursorKind::Hand; }
  void paint(Canvas& canvas) override;
  bool onMouseDown(const MouseEvent& event) override;
  bool onMouseUp(const MouseEvent& event) override;
  bool onKeyDown(const KeyEvent& event) override;
  bool onKeyUp(const KeyEvent& event) override;
  void onStateChanged() override;

 private:
  [[nodiscard]] std::string badge() const;
  /** 所在的状态栏是否是大尺寸。 */
  [[nodiscard]] bool large() const;
  void toggle();

  NotificationCenter& center_;
  bool spaceDown_{false};
};

}  // namespace rujian::ui
