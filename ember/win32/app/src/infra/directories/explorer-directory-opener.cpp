/*
 * ------------------------------------------------------------------------
 * 名称：explorer-directory-opener.cpp
 * 说明：用资源管理器打开指定目录的实现。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-22
 * 备注：无。
 * ------------------------------------------------------------------------
 */

#include "infra/directories/explorer-directory-opener.hpp"

#include "platform/shell/directory-opener.hpp"
#include "platform/text/wide-text.hpp"

namespace ember::win32::infra {

std::string ExplorerDirectoryOpener::open(const std::string& path) { return platform::openDirectoryInFileManager(platform::pathFromUtf8(path)); }

}  // namespace ember::win32::infra
