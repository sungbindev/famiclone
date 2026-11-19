/*
 * ------------------------------------------------------------------------
 * 名称：dialog-frame.hpp
 * 说明：对话框窗口的框架，对应网页版组件库独立窗口对话框的内容：标题栏（标题、最大化或还原、关闭）、可滚动的正文、右对齐的按钮区。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-19
 * 备注：对话框是独立的顶层窗口，窗口外框（边框、缩放、阴影、模态）由系统负责，本控件只画窗口里面的内容；
 *       标题栏的空白处交给系统拖动：窗口的 WM_NCHITTEST 调用 hitTestWindow，空白处转成 HTCAPTION；最大化与关闭是标题栏里的普通按钮，由本控件处理点击；
 *       Esc 请求关闭，回车在输入框或没有焦点时触发默认按钮；按钮的含义与关闭的处理由使用方经回调决定。
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
  /** 设置正文（放在滚动区域里，四周留 10 像素的内边距）；空表示没有正文。 */
  void setContent(std::unique_ptr<Widget> content);
  /** 正文是否铺满正文区（减去四周的内边距），默认否：铺满时正文自己负责滚动，对话框不滚动；在 setContent 之前调用。 */
  void setContentFill(bool fill) { contentFill_ = fill; }
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

  /** 是否有最大化按钮、关闭按钮，默认没有最大化按钮、有关闭按钮。 */
  void setMaximizable(bool maximizable);
  void setClosable(bool closable);
  /** 窗口是否已最大化：最大化时按钮显示“还原”图标。 */
  void setMaximized(bool maximized);
  /** 窗口是否是活动窗口：不活动时标题栏换成失焦的底色。 */
  void setWindowActive(bool active);

  /** 初始焦点：默认按钮，没有时是正文里第一个可获得焦点的控件，再没有就不设置。 */
  void focusInitial();
  /** 标题栏上的某点是空白处还是控件上（窗口坐标）；不在标题栏内为 Client。 */
  [[nodiscard]] DialogHit hitTestWindow(Point point) const;
  /** 标题栏的区域（窗口坐标）。 */
  [[nodiscard]] Rect titleBarRect() const;
  /** 最大化与关闭按钮的控件（没有时为空）。 */
  [[nodiscard]] Widget* maximizeControl() const { return maximize_; }
  [[nodiscard]] Widget* closeControl() const { return close_; }
  /** 内容高度按正文时窗口需要的高度：标题栏加正文（不超过 maxBodyHeight）加按钮区。 */
  [[nodiscard]] double naturalHeight(double width, double maxBodyHeight);

  [[nodiscard]] Size measure(Size available) override { return available; }
  void arrange(const Rect& bounds) override;
  void paint(Canvas& canvas) override;
  bool onKeyDown(const KeyEvent& event) override;

 private:
  [[nodiscard]] double borderWidth() const;
  [[nodiscard]] double footerHeight() const;
  [[nodiscard]] std::string defaultButtonId() const;
  void activate(const std::string& id);
  void rebuildControls();

  std::string title_;
  Widget* content_{nullptr};
  Widget* wrapper_{nullptr};
  ScrollArea* scroll_{nullptr};
  std::vector<DialogButton> specs_;
  std::vector<Button*> buttons_;
  Widget* maximize_{nullptr};
  Widget* close_{nullptr};
  bool maximizable_{false};
  bool closable_{true};
  bool contentFill_{false};
  bool maximized_{false};
  bool windowActive_{true};
  std::function<void(const std::string&)> onButton_;
  std::function<void(DialogCloseReason)> onClose_;
  std::function<void()> onToggleMaximize_;
};

}  // namespace rujian::ui
