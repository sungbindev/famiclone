/*
 * ------------------------------------------------------------------------
 * 名称：update-apply.hpp
 * 说明：应用升级的运行方式：用 --apply-update=旧程序路径 启动暂存的新程序（文件名 Ember-<版本号>.exe）时不打开窗口，等旧进程退出后把自己复制到旧程序的位置覆盖它，再启动它。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-21
 * 备注：新程序在启动前已经由旧程序核对过签名、大小与 SHA-256；这里再检查自己的文件名与目标路径（目标必须是已存在的 .exe 文件、不能是自己），不合格就拒绝，避免被人用命令行拿来覆盖别的文件；
 *       覆盖时旧程序可能刚退出、文件还被杀毒软件占着，所以重试最多 30 秒；失败时用系统消息框说明原因，仍然启动旧程序，让用户回到软件里。
 * ------------------------------------------------------------------------
 */

#pragma once

#include "shared/launch/launch-args.hpp"

namespace ember::win32::bootstrap {

/**
 * 运行一次升级的接替。
 * @param launch 启动参数，必须带 applyUpdateTarget。
 * @returns 进程退出码：成功为 0。
 */
int runUpdateApply(const shared::LaunchArguments& launch);

}  // namespace ember::win32::bootstrap
