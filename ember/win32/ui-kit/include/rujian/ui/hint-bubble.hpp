/*
 * ------------------------------------------------------------------------
 * 名称：hint-bubble.hpp
 * 说明：短暂提示气泡：在指定区域旁显示一小段说明文字，过几秒自己消失；用来告诉用户“这个操作现在做不了”，不打断操作，也不用点掉。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-25
 * 备注：Ember 自己的控件，网页版没有。外观同纯文字的悬停提示（内边距 4×8、最宽 320、同样的底色、边框与阴影），没有角标；是弹层，不拦截鼠标；
 *       默认显示在锚点区域正下方、左边对齐，下方放不下时翻到上方，左右不超出窗口；再次显示时替换内容并重新计时；在弹层之外按下鼠标时立即收起。
 * ------------------------------------------------------------------------
 */

#pragma once

#include <string>
#include <vector>

#include "rujian/ui/host.hpp"
#include "rujian/ui/widget.hpp"

namespace rujian::ui {

/** 短暂提示气泡。 */
class HintBubble final : public Widget {
 public:
  /** 默认显示的时长（毫秒）。 */
  static constexpr unsigned kDefaultMilliseconds = 2000;

  /** @param host 窗口的宿主（Root），必须比本控件活得久。 */
  explicit HintBubble(Host& host);
  ~HintBubble() override;

  /** 在 anchor（窗口坐标）旁显示 text，milliseconds 毫秒后自动收起。 */
  void show(const Rect& anchor, const std::string& text, unsigned milliseconds = kDefaultMilliseconds);
  /** 立即收起。 */
  void hide();
  [[nodiscard]] bool shown() const { return shown_; }
  [[nodiscard]] const std::string& text() const { return text_; }

  [[nodiscard]] Widget* hitTest(Point /*point*/) override { return nullptr; }
  void layerRects(std::vector<Rect>& out) const override;
  void paint(Canvas& canvas) override;
  void onTimer(unsigned id) override;

 private:
  /** 内容区的文字宽度。 */
  [[nodiscard]] double textWidth() const;
  [[nodiscard]] double borderWidth() const;

  Host& owner_;
  std::string text_;
  bool shown_{false};
  unsigned timer_{0};
};

}  // namespace rujian::ui
