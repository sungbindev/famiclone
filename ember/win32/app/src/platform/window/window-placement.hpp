/*
 * ------------------------------------------------------------------------
 * 名称：window-placement.hpp
 * 说明：窗口的初始位置与大小：大小固定（逻辑像素换成设备像素），位置取上次保存的，保存的位置已经不在任何显示器上时居中到主显示器的工作区。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-19
 * 备注：规则见 docs/05-界面.md 第 2、9 节：窗口大小不记忆、不恢复最大化，位置按上次保存的恢复。
 * ------------------------------------------------------------------------
 */

#pragma once

#include <optional>

#include "shared/shell-state/shell-state.hpp"

namespace ember::win32::platform {

/**
 * 摆放窗口。
 * @param window 窗口句柄（HWND）。
 * @param width 窗口的宽（设备像素）。
 * @param height 窗口的高（设备像素）。
 * @param saved 上次保存的位置；没有时居中。
 */
void placeWindow(void* window, int width, int height, const std::optional<shared::WindowPlacement>& saved);

}  // namespace ember::win32::platform
