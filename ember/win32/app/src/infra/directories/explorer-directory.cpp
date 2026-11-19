/*
 * ------------------------------------------------------------------------
 * 名称：explorer-directory.cpp
 * 说明：用资源管理器打开的目录的实现。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-19
 * 备注：无。
 * ------------------------------------------------------------------------
 */

#include "infra/directories/explorer-directory.hpp"

#include "platform/shell/directory-opener.hpp"

namespace ember::win32::infra {

std::string ExplorerDirectory::open() { return platform::openDirectoryInFileManager(path_); }

}  // namespace ember::win32::infra
