/*
 * ------------------------------------------------------------------------
 * 名称：window-placement.hpp
 * 说明：窗口的初始位置与大小：大小固定（逻辑像素换成设备像素），位置每次启动都居中到主显示器的工作区。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-19
 * 备注：规则见 docs/05-界面.md 第 2、9 节：窗口大小、位置都不记忆，也不恢复最大化。
 * ------------------------------------------------------------------------
 */

#pragma once

namespace ember::win32::platform {

/**
 * 把窗口摆到主显示器工作区的中央。
 * @param window 窗口句柄（HWND）。
 * @param width 窗口的宽（设备像素）。
 * @param height 窗口的高（设备像素）。
 */
void placeWindow(void* window, int width, int height);

}  // namespace ember::win32::platform
