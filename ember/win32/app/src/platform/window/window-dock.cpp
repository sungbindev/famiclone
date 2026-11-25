/*
 * ------------------------------------------------------------------------
 * 名称：window-dock.cpp
 * 说明：窗口停靠的实现。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-26
 * 备注：无。
 * ------------------------------------------------------------------------
 */

#include "platform/window/window-dock.hpp"

#include <windows.h>

#include <algorithm>
#include <cmath>
#include <cstdlib>

namespace ember::win32::platform {

namespace {

/** 磁吸范围（逻辑像素）：跟随窗的右边、上沿离主窗的左边、上沿在这个距离之内就吸住。 */
constexpr double kSnapDistance = 16.0;

HWND asWindow(void* handle) { return static_cast<HWND>(handle); }

RECT windowRect(void* handle) {
  RECT rect{};
  GetWindowRect(asWindow(handle), &rect);
  return rect;
}

int heightOf(const RECT& rect) { return static_cast<int>(rect.bottom - rect.top); }

int widthOf(const RECT& rect) { return static_cast<int>(rect.right - rect.left); }

/** 窗口自己的最小高度（设备像素）。 */
int minimumHeightOf(void* handle) {
  MINMAXINFO limits{};
  SendMessageW(asWindow(handle), WM_GETMINMAXINFO, 0, reinterpret_cast<LPARAM>(&limits));
  return static_cast<int>(limits.ptMinTrackSize.y);
}

}  // namespace

WindowDock::WindowDock(void* leader, void* follower, std::function<double()> scale) : leader_(leader), follower_(follower), scale_(std::move(scale)) {
  rememberLeader();
  followerHeight_ = heightOf(windowRect(follower_));
}

void WindowDock::rememberLeader() {
  const RECT rect = windowRect(leader_);
  leaderLeft_ = static_cast<int>(rect.left);
  leaderTop_ = static_cast<int>(rect.top);
  leaderRight_ = static_cast<int>(rect.right);
  leaderBottom_ = static_cast<int>(rect.bottom);
  leaderIconic_ = IsIconic(asWindow(leader_)) != FALSE;
}

bool WindowDock::leaderCovering() const {
  HWND leader = asWindow(leader_);
  if (IsZoomed(leader) != FALSE) return true;
  MONITORINFO monitor{sizeof(monitor)};
  if (GetMonitorInfoW(MonitorFromWindow(leader, MONITOR_DEFAULTTONEAREST), &monitor) == FALSE) return false;
  const RECT rect = windowRect(leader_);
  return rect.left <= monitor.rcMonitor.left && rect.top <= monitor.rcMonitor.top && rect.right >= monitor.rcMonitor.right && rect.bottom >= monitor.rcMonitor.bottom;
}

bool WindowDock::leaderAway() const { return IsIconic(asWindow(leader_)) != FALSE || leaderCovering(); }

bool WindowDock::dockNow(bool matchHeight) {
  docked_ = false;
  if (leaderAway()) return false;
  rememberLeader();
  const RECT leaderRect = windowRect(leader_);
  const RECT current = windowRect(follower_);
  const int width = widthOf(current);
  int height = heightOf(current);
  if (matchHeight) height = std::max(heightOf(leaderRect), minimumHeightOf(follower_));
  MONITORINFO monitor{sizeof(monitor)};
  const bool hasMonitor = GetMonitorInfoW(MonitorFromWindow(asWindow(leader_), MONITOR_DEFAULTTONEAREST), &monitor) != FALSE;
  int left = static_cast<int>(leaderRect.left) - width;
  const bool fits = !hasMonitor || left >= monitor.rcWork.left;
  if (!fits) left = static_cast<int>(monitor.rcWork.left);
  adjusting_ = true;
  SetWindowPos(asWindow(follower_), nullptr, left, leaderRect.top, width, height, SWP_NOZORDER | SWP_NOACTIVATE);
  adjusting_ = false;
  followerHeight_ = heightOf(windowRect(follower_));
  docked_ = fits;
  syncTopmost();
  return docked_;
}

void WindowDock::align() {
  const RECT leaderRect = windowRect(leader_);
  const RECT current = windowRect(follower_);
  const int width = widthOf(current);
  const int height = std::max(heightOf(leaderRect), minimumHeightOf(follower_));
  const int left = static_cast<int>(leaderRect.left) - width;
  if (current.left == left && current.top == leaderRect.top && heightOf(current) == height) return;
  adjusting_ = true;
  SetWindowPos(asWindow(follower_), nullptr, left, leaderRect.top, width, height, SWP_NOZORDER | SWP_NOACTIVATE);
  adjusting_ = false;
}

void WindowDock::matchLeaderHeight(int height) {
  const RECT leaderRect = windowRect(leader_);
  const int target = std::max(height, minimumHeightOf(leader_));
  if (heightOf(leaderRect) == target) return;
  adjusting_ = true;
  SetWindowPos(asWindow(leader_), nullptr, 0, 0, widthOf(leaderRect), target, SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
  adjusting_ = false;
}

void WindowDock::syncTopmost() {
  const bool leaderTopmost = (GetWindowLongPtrW(asWindow(leader_), GWL_EXSTYLE) & WS_EX_TOPMOST) != 0;
  const bool followerTopmost = (GetWindowLongPtrW(asWindow(follower_), GWL_EXSTYLE) & WS_EX_TOPMOST) != 0;
  if (leaderTopmost == followerTopmost) return;
  const bool wasAdjusting = adjusting_;
  adjusting_ = true;
  SetWindowPos(asWindow(follower_), leaderTopmost ? HWND_TOPMOST : HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
  adjusting_ = wasAdjusting;
}

void WindowDock::leaderChanged() {
  const RECT now = windowRect(leader_);
  const bool iconic = IsIconic(asWindow(leader_)) != FALSE;
  const bool geometryChanged = now.left != leaderLeft_ || now.top != leaderTop_ || now.right != leaderRight_ || now.bottom != leaderBottom_ || iconic != leaderIconic_;
  const bool wasIconic = leaderIconic_;
  rememberLeader();
  // 自己改动主窗引起的变化不用再处理
  if (adjusting_) return;
  syncTopmost();
  // 只是激活、层级变化时不动跟随窗
  if (!geometryChanged) return;
  HWND follower = asWindow(follower_);
  if (iconic != wasIconic) {
    // 最小化与还原两个窗口同步（不论是否停靠）
    adjusting_ = true;
    if (iconic && IsIconic(follower) == FALSE) ShowWindow(follower, SW_SHOWMINNOACTIVE);
    else if (!iconic && IsIconic(follower) != FALSE) ShowWindow(follower, SW_SHOWNOACTIVATE);
    adjusting_ = false;
  }
  if (iconic) return;
  if (coveredDocked_ && !leaderCovering()) {
    // 主窗从最大化或全屏还原：原来停靠着的重新停靠
    coveredDocked_ = false;
    docked_ = true;
  }
  if (!docked_) return;
  if (leaderCovering()) {
    docked_ = false;
    coveredDocked_ = true;
    return;
  }
  // 跟随窗自己最小化着时留在原地，还原时再对齐
  if (IsIconic(follower) == FALSE) align();
}

bool WindowDock::resizeFollowerWidth(int width) {
  if (!docked_ || leaderAway() || IsIconic(asWindow(follower_)) != FALSE) return false;
  const RECT leaderRect = windowRect(leader_);
  const RECT current = windowRect(follower_);
  const int total = width + widthOf(leaderRect);
  // 以原来整体的中心为中心，整体不超出工作区
  int left = static_cast<int>((current.left + leaderRect.right) / 2) - total / 2;
  MONITORINFO monitor{sizeof(monitor)};
  if (GetMonitorInfoW(MonitorFromWindow(asWindow(leader_), MONITOR_DEFAULTTONEAREST), &monitor) != FALSE) {
    const int workLeft = static_cast<int>(monitor.rcWork.left);
    left = std::clamp(left, workLeft, std::max(workLeft, static_cast<int>(monitor.rcWork.right) - total));
  }
  adjusting_ = true;
  SetWindowPos(asWindow(follower_), nullptr, left, current.top, width, heightOf(current), SWP_NOZORDER | SWP_NOACTIVATE);
  SetWindowPos(asWindow(leader_), nullptr, left + width, leaderRect.top, 0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
  adjusting_ = false;
  return true;
}

bool WindowDock::followerMessage(unsigned message, std::uintptr_t /*wParam*/, std::intptr_t lParam, std::intptr_t& result) {
  switch (message) {
    case WM_ENTERSIZEMOVE: {
      const RECT start = windowRect(follower_);
      POINT cursor{};
      GetCursorPos(&cursor);
      dragging_ = true;
      dragMoved_ = false;
      dragLeft_ = static_cast<int>(start.left);
      dragTop_ = static_cast<int>(start.top);
      dragCursorX_ = static_cast<int>(cursor.x);
      dragCursorY_ = static_cast<int>(cursor.y);
      return false;
    }
    case WM_EXITSIZEMOVE:
      dragging_ = false;
      // 用户自己操作过跟随窗，主窗还原后不再自动重新停靠
      coveredDocked_ = false;
      // 拖动后吸住了：松开鼠标时立即上沿对齐、高度取主窗的高度
      if (dragMoved_ && docked_ && !leaderAway()) align();
      dragMoved_ = false;
      return false;
    case WM_MOVING: {
      dragMoved_ = true;
      if (!dragging_ || leaderAway()) return false;
      RECT& rect = *reinterpret_cast<RECT*>(lParam);
      // 先按累计的鼠标移动量恢复没有吸附时的位置，再在它上面判断吸附
      POINT cursor{};
      GetCursorPos(&cursor);
      const int width = widthOf(rect);
      const int height = heightOf(rect);
      rect.left = dragLeft_ + cursor.x - dragCursorX_;
      rect.top = dragTop_ + cursor.y - dragCursorY_;
      rect.right = rect.left + width;
      rect.bottom = rect.top + height;
      const RECT leaderRect = windowRect(leader_);
      const int distance = std::max(1, static_cast<int>(std::lround(kSnapDistance * (scale_ ? scale_() : 1.0))));
      // 只从主窗的左边靠近时吸：跟随窗的右边要在主窗左边的左侧，上下方向有重叠
      const int gap = static_cast<int>(leaderRect.left - rect.right);
      if (gap >= 0 && gap <= distance && rect.top < leaderRect.bottom && rect.bottom > leaderRect.top) {
        OffsetRect(&rect, gap, 0);
        if (std::abs(static_cast<int>(rect.top - leaderRect.top)) <= distance) OffsetRect(&rect, 0, leaderRect.top - rect.top);
      }
      result = TRUE;
      return true;
    }
    case WM_WINDOWPOSCHANGED:
      followerChanged();
      return false;
    default:
      return false;
  }
}

void WindowDock::followerChanged() {
  HWND leader = asWindow(leader_);
  const bool iconic = IsIconic(asWindow(follower_)) != FALSE;
  const bool restored = followerIconic_ && !iconic;
  const bool iconicChanged = iconic != followerIconic_;
  followerIconic_ = iconic;
  if (iconicChanged && !adjusting_) {
    // 最小化与还原两个窗口同步（不论是否停靠）：主窗从最大化最小化的，还原回最大化
    adjusting_ = true;
    if (iconic && IsIconic(leader) == FALSE) ShowWindow(leader, SW_SHOWMINNOACTIVE);
    else if (!iconic && IsIconic(leader) != FALSE) ShowWindow(leader, IsZoomed(leader) != FALSE ? SW_RESTORE : SW_SHOWNOACTIVATE);
    adjusting_ = false;
  }
  if (iconic) return;
  const RECT current = windowRect(follower_);
  const int height = heightOf(current);
  const bool heightChanged = height != followerHeight_;
  followerHeight_ = height;
  if (adjusting_) return;
  const bool away = leaderAway();
  if (restored && docked_ && !away) {
    // 最小化期间主窗可能移动过，还原后重新对齐
    align();
    return;
  }
  // 主窗最小化着（位置不可靠）时不重新判断
  if (IsIconic(leader) != FALSE) return;
  const RECT leaderRect = windowRect(leader_);
  docked_ = !away && current.right == leaderRect.left && current.top < leaderRect.bottom && current.bottom > leaderRect.top;
  if (!docked_) return;
  // 已停靠时调整跟随窗的高度，主窗跟着改
  if (heightChanged) matchLeaderHeight(height);
}

}  // namespace ember::win32::platform
