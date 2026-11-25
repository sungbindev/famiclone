/*
 * ------------------------------------------------------------------------
 * 名称：window-dock.hpp
 * 说明：窗口停靠：让一个窗口（跟随窗）停靠在另一个窗口（主窗）的左边，两边贴合；拖动跟随窗靠近主窗的左边时磁吸，吸住后跟随主窗移动，高度与主窗保持一致，两个窗口的最小化与还原互相同步（不论是否停靠），置顶状态与主窗一致。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-26
 * 备注：“已停靠”就是跟随窗的右边缘与主窗的左边缘重合、上下方向有重叠，由跟随窗每次位置或大小变化后重新判断，所以拖开、调整右边的大小都会脱离，调整上边、下边、左边的大小仍然停靠；
 *       磁吸只在跟随窗从主窗左边靠近时生效（跟随窗的右边在主窗左边的左侧，距离在吸附范围内，且上下方向有重叠），跟随窗的右边越过主窗的左边时不吸；吸住时上沿在吸附范围内也对齐到主窗的上沿，两个方向各自吸附；
 *       吸住并松开鼠标后跟随窗立即上沿对齐、高度改成主窗的高度；主窗位置或大小变化时也对齐上沿与高度；
 *       已停靠时两个窗口的高度同步：调整任何一个的高度，另一个跟着改（各自的最小高度限制内）；
 *       已停靠时跟随窗改变宽度（resizeFollowerWidth）由本类把两个窗口当作一个整体重新摆放，以原来整体的中心为中心、不超出工作区；
 *       主窗最大化或全屏时不再停靠、也不再跟随，还原后原来停靠着的重新停靠（期间用户自己拖动过跟随窗则不再自动停靠）；跟随窗自己最小化时留在原地，还原时若仍是停靠状态就重新对齐；任何一个窗口最小化或还原，另一个同步最小化或还原；任何一个窗口被激活（点任务栏按钮、点窗口），另一个也一起提到最前面，不会留在别的程序后面；
 *       本类不订阅窗口消息：主窗位置、大小、最小化、置顶变化时由使用方调用 leaderChanged，主窗被激活时调用 leaderActivated，跟随窗的消息由使用方转给 followerMessage；必须在窗口所在的线程使用，两个窗口都要比本对象活得久。
 * ------------------------------------------------------------------------
 */

#pragma once

#include <cstdint>
#include <functional>

namespace ember::win32::platform {

/** 窗口停靠。 */
class WindowDock {
 public:
  /**
   * @param leader 主窗（HWND）。
   * @param follower 跟随窗（HWND）。
   * @param scale 取跟随窗当前的缩放比例（设备像素 / 逻辑像素），用来换算吸附范围。
   */
  WindowDock(void* leader, void* follower, std::function<double()> scale);

  /**
   * 把跟随窗贴到主窗的左边，上沿对齐，并让置顶状态与主窗一致。
   * 主窗最小化、最大化或全屏时什么都不动；主窗左边放不下跟随窗时，跟随窗靠到工作区左边（盖住主窗的一部分），不算停靠。
   * @param matchHeight 高度是否取主窗的高度（不低于跟随窗的最小高度）。
   * @return 是否已停靠。
   */
  bool dockNow(bool matchHeight);

  /** 现在是否停靠着。 */
  [[nodiscard]] bool docked() const { return docked_; }

  /** 主窗的位置、大小、最小化或置顶变化了：已停靠时跟着移动并对齐上沿与高度，并同步置顶；最小化与还原不论是否停靠都让跟随窗同步。 */
  void leaderChanged();

  /** 主窗被激活了：跟随窗一起提到最前面（不抢焦点）。 */
  void leaderActivated();

  /**
   * 跟随窗要把宽度改成 width（设备像素）：已停靠时把两个窗口当作一个整体，以原来整体的中心为中心重新摆放（跟随窗在左、主窗在右，整体不超出工作区）。
   * @return 已处理时为 true；没有停靠时返回 false，由窗口自己调整。
   */
  bool resizeFollowerWidth(int width);

  /**
   * 跟随窗收到的消息：用户拖动它时做磁吸（WM_MOVING），位置或大小变化后重新判断是否停靠并同步高度（WM_WINDOWPOSCHANGED），拖动松开后吸住的窗口对齐主窗（WM_EXITSIZEMOVE）。
   * @return 已处理（result 是窗口过程的返回值）时为 true；其余消息返回 false，由窗口自己照常处理。
   */
  bool followerMessage(unsigned message, std::uintptr_t wParam, std::intptr_t lParam, std::intptr_t& result);

 private:
  /** 主窗最大化或铺满了显示器（全屏）。 */
  [[nodiscard]] bool leaderCovering() const;
  /** 主窗最小化，或最大化、全屏。 */
  [[nodiscard]] bool leaderAway() const;
  /** 记下主窗现在的位置、大小与最小化状态。 */
  void rememberLeader();
  /** 把跟随窗摆回主窗左边，上沿与高度跟主窗一致（高度不低于跟随窗的最小高度）。 */
  void align();
  /** 把主窗的高度改成 height（不低于主窗的最小高度），位置不动。 */
  void matchLeaderHeight(int height);
  void syncTopmost();
  /** active 被激活后，把 other 提到最前面，再把 active 放回它的上面（都不激活）。 */
  void raiseTogether(void* active, void* other);
  /** 跟随窗位置或大小变化后重新判断。 */
  void followerChanged();

  void* leader_;
  void* follower_;
  std::function<double()> scale_;
  bool docked_{false};
  /** 本类自己改动两个窗口时置位，它引起的位置变化不当作用户的操作。 */
  bool adjusting_{false};
  /** 跟随窗是不是最小化着（上一次消息时的状态），用来发现还原。 */
  bool followerIconic_{false};
  /** 停靠着时主窗最大化或全屏了，还原后要重新停靠。 */
  bool coveredDocked_{false};
  /** 这次拖动里跟随窗是被移动过（而不是调整大小）。 */
  bool dragMoved_{false};
  /** 上一次看到的跟随窗高度，用来发现高度变化。 */
  int followerHeight_{0};
  /** 上一次看到的主窗位置、大小与最小化状态，只有变化了才跟着动（单纯的激活与层级变化不动）。 */
  int leaderLeft_{0};
  int leaderTop_{0};
  int leaderRight_{0};
  int leaderBottom_{0};
  bool leaderIconic_{false};
  /** 正在拖动跟随窗：系统每次给的矩形是上一次改过的位置加上鼠标的增量，吸住后永远拖不开，所以自己记下开始时的位置与鼠标位置，按累计的移动量算。 */
  bool dragging_{false};
  int dragLeft_{0};
  int dragTop_{0};
  int dragCursorX_{0};
  int dragCursorY_{0};
};

}  // namespace ember::win32::platform
