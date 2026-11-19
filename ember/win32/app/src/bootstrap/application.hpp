/*
 * ------------------------------------------------------------------------
 * 名称：application.hpp
 * 说明：组装根：创建全部实现并按接口注入，创建主窗口，运行消息循环，退出时保存界面状态并停止核心。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-19
 * 备注：只创建实现并注入，不写业务判断。
 * ------------------------------------------------------------------------
 */

#pragma once

#include "shared/launch/launch-args.hpp"

namespace ember::win32::bootstrap {

/**
 * 运行应用。
 * @param launch 命令行启动参数。
 * @returns 进程的退出码：0 为正常退出（含“已有实例，已通知它提前窗口”），非 0 为无法启动。
 */
int runApplication(const shared::LaunchArguments& launch);

}  // namespace ember::win32::bootstrap
