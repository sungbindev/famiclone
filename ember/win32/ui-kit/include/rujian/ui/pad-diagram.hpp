/*
 * ------------------------------------------------------------------------
 * 名称：pad-diagram.hpp
 * 说明：手柄图：画出手柄外形，每个键是图上的一个按钮，按钮上写键名与当前的绑定；点哪个键就由使用方开始设置哪个键。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-20
 * 备注：它不在网页版组件库里（Ember 自己的控件），所以没有网页基线，由单元测试覆盖；
 *       版面（画布尺寸、手柄外形、十字键中心、各键的位置与大小，设计单位）由使用方给出，控件按宽度等比缩放，宽度最大 560 并水平居中；
 *       键按钮的底色与状态同次要按钮，选中的键同列表选中项，正在等待按键的键是强调色的边框，整张图不可用时全部置灰。
 * ------------------------------------------------------------------------
 */

#pragma once

#include <functional>
#include <string>
#include <vector>

#include "rujian/ui/widget.hpp"

namespace rujian::ui {

/** 一个键在图上的位置：中心点与大小（设计单位）；圆形键的高度等于宽度。 */
struct PadKeyShape {
  /** 键的标识，与 PadHotspot::id 对应。 */
  std::string id;
  bool round{false};
  double cx{0};
  double cy{0};
  double width{0};
  double height{0};
};

/** 手柄图的版面（设计单位）。 */
struct PadDiagramLayout {
  double canvasWidth{560};
  double canvasHeight{220};
  /** 手柄外形：圆角矩形。 */
  Rect body;
  double bodyRadius{0};
  /** 十字键中间的装饰块。 */
  Rect crossCenter;
  std::vector<PadKeyShape> keys;
};

/** 图上一个键要显示的内容。 */
struct PadHotspot {
  std::string id;
  /** 键名，如“上”“A”“连发 A”。 */
  std::string role;
  /** 当前的绑定文字；没有绑定时为“未绑定”。 */
  std::string current;
  /** 没有绑定：绑定文字用弱化的颜色与常规字重。 */
  bool unbound{false};
  /** 悬停提示。 */
  std::string tooltip;
  bool selected{false};
  bool learning{false};
};

/** 图上的一个键按钮。 */
class PadKeyButton final : public Widget {
 public:
  PadKeyButton(std::string id, bool round);

  void setContent(const PadHotspot& hotspot);
  void setPickHandler(std::function<void(const std::string&)> handler) { onPick_ = std::move(handler); }

  [[nodiscard]] const std::string& id() const { return id_; }
  [[nodiscard]] bool selected() const { return hotspot_.selected; }
  [[nodiscard]] bool learning() const { return hotspot_.learning; }
  [[nodiscard]] const PadHotspot& hotspot() const { return hotspot_; }
  [[nodiscard]] bool round() const { return round_; }

  [[nodiscard]] bool focusable() const override { return true; }
  [[nodiscard]] CursorKind cursor() const override { return enabled() ? CursorKind::Hand : CursorKind::Arrow; }
  void paint(Canvas& canvas) override;
  bool onMouseDown(const MouseEvent& event) override;
  bool onMouseUp(const MouseEvent& event) override;
  bool onKeyDown(const KeyEvent& event) override;
  bool onKeyUp(const KeyEvent& event) override;
  void onStateChanged() override;

 private:
  void pick();

  std::string id_;
  bool round_;
  PadHotspot hotspot_;
  std::function<void(const std::string&)> onPick_;
  /** Space 键是否按在键上（抬起时才触发）。 */
  bool spaceDown_{false};
};

/** 手柄图。 */
class PadDiagram final : public Widget {
 public:
  explicit PadDiagram(PadDiagramLayout layout);

  /** 设置各键的内容；标识不在版面里的忽略。 */
  void setHotspots(const std::vector<PadHotspot>& hotspots);
  /** 整张图的键都不可点（如还没有指定手柄）。 */
  void setDisabled(bool disabled);
  [[nodiscard]] bool disabled() const { return disabled_; }
  /** 点了某个键：参数是键的标识。 */
  void setOnPick(std::function<void(const std::string&)> handler);
  /** 某个键的按钮；没有时为空。 */
  [[nodiscard]] PadKeyButton* key(const std::string& id) const;

  [[nodiscard]] Size measure(Size available) override;
  void arrange(const Rect& bounds) override;
  void paint(Canvas& canvas) override;

 private:
  /** 图的绘制区：宽度取可用宽度与 560 中较小的，水平居中，高度按画布宽高比。 */
  [[nodiscard]] Rect drawingRect(const Rect& bounds) const;

  PadDiagramLayout layout_;
  bool disabled_{false};
  std::vector<PadKeyButton*> keys_;
};

}  // namespace rujian::ui
