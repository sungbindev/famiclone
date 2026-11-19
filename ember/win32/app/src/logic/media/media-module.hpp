/*
 * ------------------------------------------------------------------------
 * 名称：media-module.hpp
 * 说明：介质模块在外壳里的登记项：底部面板的“设备”（系统栏左侧的设备按钮，图标里带软驱读写灯）、系统栏左侧的已载入软盘名、拖入窗口的软盘镜像与声音文件的处理项和常驻部分。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-19
 * 备注：对应 Electron 外壳的 media-module.tsx；控制器由组装根创建并传入，面板内容控件与登记项共用；规则见 docs/05-界面.md 第 4.3 节。
 *       读写灯亮着 = 任一软驱的马达在转（真机的读写灯跟马达一起亮灭），没有软驱时不显示；软盘名：只有一个软驱时直接显示镜像名，有多个软驱时每项前面带驱动器名（“A: 镜像名”），没有载入盘片的软驱不显示；
 *       拖入的软盘镜像载入软驱 A，拖入的声音文件载入磁带机（只载入，不自动播放）。
 * ------------------------------------------------------------------------
 */

#pragma once

#include "contracts/shell/shell-registry.hpp"
#include "logic/media/media-controller.hpp"

namespace ember::win32::logic {

/** 底部面板“设备”页签的标识。 */
inline constexpr const char* kMediaPanelId = "media.devices";

/** 创建介质模块的登记项；控制器必须比登记表活得久。 */
[[nodiscard]] contracts::ShellModule createMediaModule(MediaController& controller);

}  // namespace ember::win32::logic
