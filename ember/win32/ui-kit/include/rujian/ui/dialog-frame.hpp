/*
 * ------------------------------------------------------------------------
 * 名称：dialog-frame.hpp
 * 说明：对话框窗口的框架，对应网页版组件库独立窗口对话框的内容：标题栏（可选的图标、标题、最小化、最大化或还原、关闭）、可滚动的正文、右对齐的按钮区。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-19
 * 备注：对话框是独立的顶层窗口，窗口外框（边框、缩放、阴影、模态）由系统负责，本控件只画窗口里面的内容；
 *       标题栏的空白处交给系统拖动：窗口的 WM_NCHITTEST 调用 hitTestWindow，空白处转成 HTCAPTION；最小化、最大化与关闭是标题栏里的普通按钮，由本控件处理点击；
 *       按钮有两种样子：默认是网页版的 24×24 小方块；setSystemButtons(true) 后换成和主窗口标题栏一样的系统按钮（宽 46、占满标题栏高度、线条图形）；
 *       Esc 请求关闭，回车在输入框或没有焦点时触发默认按钮；按钮的含义与关闭的处理由使用方经回调决定；
 *       打开时的初始焦点是框架自己（不显示焦点框，不在 Tab 顺序里），不聚焦到某个按钮或输入框上，Esc 与回车照常生效。
 * ------------------------------------------------------------------------
 */

#pragma once

#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "rujian/ui/button.hpp"
#include "rujian/ui/scroll-area.hpp"
#include "rujian/ui/widget.hpp"

namespace rujian::ui {

/** 对话框里一个位置的命中结果。 */
enum class DialogHit {
  /** 在标题栏的按钮或正文、按钮区里，由控件自己处理。 */
  Client,
  /** 标题栏的空白处，用来拖动窗口。 */
  Caption
};

/** 请求关闭对话框的原因。 */
enum class DialogCloseReason { Escape, CloseButton };

/** 按钮区的一个按钮。 */
struct DialogButton {
  /** 标识，点击时传回。 */
  std::string id;
  /** 文字。 */
  std::string text;
  ButtonVariant variant{ButtonVariant::Secondary};
  /** 显式的图标；空表示按文字自动匹配。 */
  std::optional<IconId> icon;
  /** 默认按钮：在输入框里按回车时触发。 */
  bool isDefault{false};
  /** 取消按钮：使用方据此在 Esc 时决定结果。 */
  bool isCancel{false};
  bool disabled{false};
};

/** 对话框框架。 */
class DialogFrame : public Widget {
 public:
  DialogFrame();

  void setTitle(std::string title);
  /** 标题后面的说明文字（弱化色，标题栏空间不够时用省略号）；空表示没有。 */
  void setTitleDetail(std::string detail);
  /** 设置正文（放在滚动区域里，四周留 10 像素的内边距）；空表示没有正文。 */
  void setContent(std::unique_ptr<Widget> content);
  /** 正文是否铺满正文区（减去四周的内边距），默认否：铺满时正文自己负责滚动，对话框不滚动；在 setContent 之前调用。 */
  void setContentFill(bool fill) { contentFill_ = fill; }
  /** 铺满时正文区右边不留内边距（正文里的滚动条贴到窗口右边，正文自己给右边留空），默认否；在 setContent 之前调用。 */
  void setContentFlushRight(bool flush) { contentFlushRight_ = flush; }
  [[nodiscard]] Widget* content() const { return content_; }
  /** 设置按钮区的按钮；空表示没有按钮区。 */
  void setButtons(std::vector<DialogButton> buttons);
  [[nodiscard]] Button* button(const std::string& id) const;
  void setButtonDisabled(const std::string& id, bool disabled);
  /** 点击按钮区的按钮（或回车触发默认按钮）时以按钮标识调用。 */
  void setOnButton(std::function<void(const std::string& id)> handler) { onButton_ = std::move(handler); }
  /** 点击关闭按钮或按 Esc 时调用；使用方决定是否真的关闭。 */
  void setOnClose(std::function<void(DialogCloseReason)> handler) { onClose_ = std::move(handler); }
  /** 点击最大化或还原按钮时调用。 */
  void setOnToggleMaximize(std::function<void()> handler) { onToggleMaximize_ = std::move(handler); }
  /** 点击最小化按钮时调用。 */
  void setOnMinimize(std::function<void()> handler) { onMinimize_ = std::move(handler); }

  /** 标题栏左边的图标：16×16 的控件，空表示没有。 */
  void setLogo(std::unique_ptr<Widget> logo);
  /** 标题栏按钮的样子：为真时换成和主窗口标题栏一样的系统按钮，默认是网页版的小方块。 */
  void setSystemButtons(bool systemButtons);

  /** 是否有最小化按钮（只有系统按钮样式才有）、最大化按钮、关闭按钮，默认没有最小化与最大化、有关闭。 */
  void setMinimizable(bool minimizable);
  void setMaximizable(bool maximizable);
  void setClosable(bool closable);
  /** 窗口是否已最大化：最大化时按钮显示“还原”图标。 */
  void setMaximized(bool maximized);
  /** 窗口是否是活动窗口：不活动时标题栏换成失焦的底色。 */
  void setWindowActive(bool active);

  /** 初始焦点：框架自己（不显示焦点框，不在 Tab 顺序里），不聚焦到按钮或正文里的控件上。 */
  void focusInitial();
  /** 标题栏上的某点是空白处还是控件上（窗口坐标）；不在标题栏内为 Client。 */
  [[nodiscard]] DialogHit hitTestWindow(Point point) const;
  /** 标题栏的区域（窗口坐标）。 */
  [[nodiscard]] Rect titleBarRect() const;
  /** 最小化、最大化与关闭按钮的控件（没有时为空）。 */
  [[nodiscard]] Widget* minimizeControl() const { return minimize_; }
  [[nodiscard]] Widget* maximizeControl() const { return maximize_; }
  [[nodiscard]] Widget* closeControl() const { return close_; }
  /** 内容高度按正文时窗口需要的高度：标题栏加正文（不超过 maxBodyHeight）加按钮区。 */
  [[nodiscard]] double naturalHeight(double width, double maxBodyHeight);
  /** 窗口刚好装下标题栏上的按钮、正文的最小尺寸（见 Widget::minimumSize）与按钮区所需的最小尺寸；正文没有要求的方向上只算标题栏与按钮区。 */
  [[nodiscard]] Size minimumWindowSize();

  [[nodiscard]] Size measure(Size available) override { return available; }
  void arrange(const Rect& bounds) override;
  void paint(Canvas& canvas) override;
  bool onKeyDown(const KeyEvent& event) override;
  /** 自己可以获得焦点（打开时的初始焦点、点在空白处），这样 Esc 与回车总能到达；不在 Tab 顺序里。 */
  [[nodiscard]] bool focusable() const override { return true; }
  [[nodiscard]] bool tabStop() const override { return false; }

 private:
  [[nodiscard]] double borderWidth() const;
  [[nodiscard]] double footerHeight() const;
  [[nodiscard]] std::string defaultButtonId() const;
  void activate(const std::string& id);
  void rebuildControls();

  std::string title_;
  std::string titleDetail_;
  Widget* content_{nullptr};
  Widget* wrapper_{nullptr};
  ScrollArea* scroll_{nullptr};
  std::vector<DialogButton> specs_;
  std::vector<Button*> buttons_;
  Widget* minimize_{nullptr};
  Widget* maximize_{nullptr};
  Widget* close_{nullptr};
  Widget* logo_{nullptr};
  bool systemButtons_{false};
  bool minimizable_{false};
  bool maximizable_{false};
  bool closable_{true};
  bool contentFill_{false};
  bool contentFlushRight_{false};
  bool maximized_{false};
  bool windowActive_{true};
  std::function<void(const std::string&)> onButton_;
  std::function<void(DialogCloseReason)> onClose_;
  std::function<void()> onToggleMaximize_;
  std::function<void()> onMinimize_;
};

}  // namespace rujian::ui
