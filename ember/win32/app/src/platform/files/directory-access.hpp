/*
 * ------------------------------------------------------------------------
 * 名称：directory-access.hpp
 * 说明：目录的访问检查：能不能在目录里创建文件（用来判断软件所在目录能不能放用户数据、能不能被新版本覆盖）。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-21
 * 备注：做法是在目录里真的创建并删除一个临时文件，比读权限位可靠（装在 Program Files 之类需要管理员权限的位置时会失败）。
 * ------------------------------------------------------------------------
 */

#pragma once

#include <filesystem>

namespace ember::win32::platform {

/** 能不能在目录里创建文件；目录为空路径或不存在时为 false。 */
[[nodiscard]] bool canCreateFilesIn(const std::filesystem::path& directory);

}  // namespace ember::win32::platform
