/*
 * ------------------------------------------------------------------------
 * 名称：user-data-move.hpp
 * 说明：移动用户数据的运行方式：用 --move-user-data 启动时不打开窗口，等旧进程退出后复制并切换用户数据，再重新启动软件。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-21
 * 备注：失败时用系统消息框说明原因，用户数据保持原样，仍然重新启动软件。
 * ------------------------------------------------------------------------
 */

#pragma once

#include "shared/launch/launch-args.hpp"

namespace ember::win32::bootstrap {

/**
 * 运行一次用户数据的移动。
 * @param launch 启动参数，必须带 moveUserData。
 * @returns 进程退出码：成功为 0。
 */
int runUserDataMove(const shared::LaunchArguments& launch);

}  // namespace ember::win32::bootstrap
