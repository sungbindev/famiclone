/*
 * ------------------------------------------------------------------------
 * 名称：status-label.hpp
 * 说明：状态标签：图标加文字表达一个状态，用于表格里的状态列。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-19
 * 备注：对应组件库的 StatusLabel，只做 Ember 用到的成功（绿色对勾）与已停止（弱化色叉）两种；等待、进行中（像素格动画）、失败不做。
 * ------------------------------------------------------------------------
 */

#pragma once

#include <string>

#include "rujian/ui/widget.hpp"

namespace rujian::ui {

class Label;

/** 状态标签的状态。 */
enum class StatusTone { Success, Stopped };

/** 状态标签。 */
class StatusLabel : public Widget {
 public:
  StatusLabel();

  /** 状态，决定图标与颜色。 */
  void setTone(StatusTone tone);
  /** 状态文字。 */
  void setText(std::string text);

  [[nodiscard]] Size measure(Size available) override;
  void arrange(const Rect& bounds) override;
  void paint(Canvas& canvas) override;
  [[nodiscard]] bool stretchesWidth() const override { return false; }
  [[nodiscard]] bool stretchesHeight() const override { return false; }

 private:
  Label* text_{nullptr};
  StatusTone tone_{StatusTone::Stopped};
};

}  // namespace rujian::ui
