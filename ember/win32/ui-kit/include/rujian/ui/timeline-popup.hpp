/*
 * ------------------------------------------------------------------------
 * 名称：timeline-popup.hpp
 * 说明：时间轴弹层：一条长长的声音包络图加刻度，标出播放头与空白段，鼠标点击或拖动后松开即请求定位，用来在长录音里找到某一段。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-21
 * 备注：这是本控件库自己的控件，网页版没有；弹层显示在窗口的弹层上（Host::showLayer），由调用方拥有，用 open 在锚点（如信息行里被点击的那一列）附近打开；
 *       只在松开鼠标时回调一次 onSeek：拖动中只在图上显示将要定位到的位置与时间，播放头仍然是调用方给的当前位置（松开时先挪到新位置，等调用方给出真实位置）；
 *       按 Esc 或在弹层之外按下鼠标时关闭，但在 owner（打开它的控件，如那一行）上按下不关闭，由 owner 自己决定是否再次打开；窗口大小变化时关闭；
 *       没有包络数据时（分析中、分析失败）画一条空轨道，中间写 notice 的文字，播放头与定位仍然可用；
 *       包络的每一项是这一段时间里的最大幅度（0 到 100），图上每个像素列取它覆盖的各项的最大值，上下对称；空白段画成浅色底。
 * ------------------------------------------------------------------------
 */

#pragma once

#include <cstdint>
#include <functional>
#include <string>
#include <vector>

#include "rujian/ui/widget.hpp"

namespace rujian::ui {

/** 时间轴上的一段区间（秒）。 */
struct TimelineRange {
  double start{0};
  double end{0};
};

/** 时间轴弹层。 */
class TimelinePopup final : public Widget {
 public:
  TimelinePopup();
  ~TimelinePopup() override;

  /** 总时长（秒）；不大于 0 时不能定位。 */
  void setDuration(double seconds);
  /**
   * 设置声音包络。
   * @param peaks 每一项是 step 秒内的最大幅度，0 到 100。
   * @param step 每一项占的时间（秒）。
   * @param blanks 空白段。
   */
  void setEnvelope(std::vector<std::uint8_t> peaks, double step, std::vector<TimelineRange> blanks);
  /** 清除包络与空白段。 */
  void clearEnvelope();
  /** 轨道中间的说明文字（如“分析中 30%”）；不为空时不画包络。 */
  void setNotice(std::string text);
  /** 播放头的位置（秒）。 */
  void setPosition(double seconds);
  /** 用户松开鼠标后回调要定位到的位置（秒）。 */
  void setOnSeek(std::function<void(double)> handler) { onSeek_ = std::move(handler); }
  /** 弹层关闭后回调（包括 Esc、点击弹层之外与窗口大小变化引起的关闭，不包括调用 close 时）。 */
  void setOnClosed(std::function<void()> handler) { onClosed_ = std::move(handler); }

  /**
   * 在窗口的弹层上打开：显示在锚点上方（放不下时在下方），并获得键盘焦点以便 Esc 关闭。
   * @param owner 打开它的控件，必须已挂到窗口上；在它或它的后代上按下鼠标不关闭弹层。
   * @param anchor 锚点区域（窗口坐标）。
   */
  void open(Widget& owner, const Rect& anchor);
  /** 关闭；没有打开时无效。 */
  void close();
  [[nodiscard]] bool isOpen() const { return open_; }

  /** 轨道区域（窗口坐标）；测试与调用方据此定位。 */
  [[nodiscard]] Rect plotRect() const;
  /** 秒数的显示：不满 1 小时为“分:秒”，否则为“时:分:秒”。 */
  [[nodiscard]] static std::string formatTime(double seconds);

  [[nodiscard]] Size measure(Size available) override;
  [[nodiscard]] bool focusable() const override { return true; }
  [[nodiscard]] bool tabStop() const override { return false; }
  [[nodiscard]] CursorKind cursor() const override { return (hoverActive_ || dragging_) && duration_ > 0 ? CursorKind::Hand : CursorKind::Arrow; }
  void paint(Canvas& canvas) override;
  bool onMouseDown(const MouseEvent& event) override;
  bool onMouseUp(const MouseEvent& event) override;
  bool onMouseMove(const MouseEvent& event) override;
  bool onKeyDown(const KeyEvent& event) override;
  void onStateChanged() override;

 private:
  /** 水平坐标对应的时间（限制在 0 到总时长）。 */
  [[nodiscard]] double timeAt(double x) const;
  /** 时间对应的水平坐标。 */
  [[nodiscard]] double xAt(double seconds) const;
  /** 按锚点与窗口大小定位并摆放。 */
  void place(const Rect& anchor);
  /** 画包络（一个像素列一根竖条）。 */
  void paintEnvelope(Canvas& canvas, const Rect& inner) const;
  /** 画刻度与时间。 */
  void paintAxis(Canvas& canvas) const;
  /** 画鼠标悬停或拖动的位置线与时间提示。 */
  void paintMarker(Canvas& canvas, const Rect& inner, double x) const;
  /** 关闭并告知调用方。 */
  void dismiss();

  double duration_{0};
  double position_{0};
  std::vector<std::uint8_t> peaks_;
  double step_{0};
  std::vector<TimelineRange> blanks_;
  std::string notice_;
  std::function<void(double)> onSeek_;
  std::function<void()> onClosed_;
  Widget* owner_{nullptr};
  bool open_{false};
  bool dragging_{false};
  /** 鼠标是否在轨道上（悬停）与悬停或拖动的水平坐标。 */
  bool hoverActive_{false};
  double markerX_{0};
};

}  // namespace rujian::ui
