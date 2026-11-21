/*
 * ------------------------------------------------------------------------
 * 名称：file-association.hpp
 * 说明：把 .nes 文件的打开方式登记为本程序（双击 .nes 文件用 Ember 打开并运行）。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-21
 * 备注：只写当前用户的注册表（HKEY_CURRENT_USER\Software\Classes），不需要管理员权限；
 *       登记后 Ember 出现在 .nes 文件的“打开方式”里；.nes 还没有默认程序时它就是默认程序，已经有默认程序（含用户在系统里选定的）时不改，由用户在系统里选择；
 *       登记的内容已经是最新时不写注册表、不通知资源管理器。
 * ------------------------------------------------------------------------
 */

#pragma once

#include <filesystem>
#include <string>

namespace ember::win32::platform {

/**
 * 登记 .nes 文件的打开方式。
 * @param executable 本程序的完整路径。
 * @returns 成功（含已经是最新）时为空，失败时是错误说明。
 */
[[nodiscard]] std::string registerCartridgeAssociation(const std::filesystem::path& executable);

}  // namespace ember::win32::platform
