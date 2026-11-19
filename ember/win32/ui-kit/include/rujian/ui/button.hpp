/*
 * ------------------------------------------------------------------------
 * 名称：button.hpp
 * 说明：按钮控件，对应网页版组件库的 Button：次要、主要、危险、无底色、链接五种样式，紧凑、纯图标、切换（按下）、禁用，图标可显式指定、按文字自动匹配或关闭。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-19
 * 备注：只实现 Ember 界面用到的属性（见 docs/08 的用法表）；强调色样式与“添加/修改/删除”预设没有用到，不做。
 *       “title” 悬停提示由外壳类控件阶段的提示控件统一处理，这里不显示。
 * ------------------------------------------------------------------------
 */

#pragma once

#include <functional>
#include <optional>
#include <string>

#include "rujian/ui/widget.hpp"

namespace rujian::ui {

/** 按钮样式。 */
enum class ButtonVariant { Secondary, Primary, Danger, Ghost, Link };

/** 按钮。 */
class Button : public Widget {
 public:
  /** @param label 按钮文字（UTF-8）。 */
  explicit Button(std::string label);

  /** 设置文字。 */
  void setLabel(std::string label);
  /** 设置样式，默认次要。 */
  void setVariant(ButtonVariant variant);
  /** 显式指定图标；空值表示不显式指定（按 setAutoIcon 决定）。 */
  void setIcon(std::optional<IconId> icon);
  /** 没有显式图标时是否按文字自动匹配图标，默认是；对应网页版的 icon={false} 就是关闭它。 */
  void setAutoIcon(bool enabled);
  /** 图标放在文字后面；空值表示用匹配规则里的位置（没有规则时在前）。 */
  void setIconAtEnd(std::optional<bool> atEnd);
  /** 只显示图标（没有图标时仍显示文字）。 */
  void setIconOnly(bool iconOnly);
  /** 紧凑按钮，只有最小宽度不同。 */
  void setCompact(bool compact);
  /** 切换按钮的按下状态；空值表示不是切换按钮。点击不会自己改变它，由使用方在回调里设置。 */
  void setToggled(std::optional<bool> toggled);
  /** 设置点击回调：鼠标在按钮上按下并抬起，或按 Enter、Space 键时触发。 */
  void setOnClick(std::function<void()> handler) { onClick_ = std::move(handler); }

  [[nodiscard]] Size measure(Size available) override;
  [[nodiscard]] bool focusable() const override { return true; }
  [[nodiscard]] bool stretchesHeight() const override { return variant_ == ButtonVariant::Link; }
  [[nodiscard]] CursorKind cursor() const override { return enabled() ? CursorKind::Hand : CursorKind::Arrow; }
  void paint(Canvas& canvas) override;
  bool onMouseDown(const MouseEvent& event) override;
  bool onMouseUp(const MouseEvent& event) override;
  bool onKeyDown(const KeyEvent& event) override;
  bool onKeyUp(const KeyEvent& event) override;
  void onStateChanged() override;

 private:
  /** 最终要显示的图标与位置。 */
  struct ResolvedIcon {
    IconId icon;
    bool atEnd;
  };

  [[nodiscard]] std::optional<ResolvedIcon> resolveIcon() const;
  [[nodiscard]] TextStyle textStyle(Color color) const;
  void click();

  std::string label_;
  ButtonVariant variant_{ButtonVariant::Secondary};
  std::optional<IconId> icon_;
  bool autoIcon_{true};
  std::optional<bool> iconAtEnd_;
  bool iconOnly_{false};
  bool compact_{false};
  std::optional<bool> toggled_;
  std::function<void()> onClick_;
  /** Space 键是否按在按钮上（抬起时才触发点击）。 */
  bool spaceDown_{false};
};

}  // namespace rujian::ui
