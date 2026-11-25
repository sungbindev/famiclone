/*
 * ------------------------------------------------------------------------
 * 名称：explorer-directory-opener.hpp
 * 说明：用资源管理器打开指定目录的实现。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-22
 * 备注：无。
 * ------------------------------------------------------------------------
 */

#pragma once

#include "contracts/shell/directory-opener.hpp"

namespace ember::win32::infra {

/** 用资源管理器打开指定目录。 */
class ExplorerDirectoryOpener : public contracts::DirectoryOpener {
 public:
  [[nodiscard]] std::string open(const std::string& path) override;
  [[nodiscard]] std::string reveal(const std::string& path) override;
  [[nodiscard]] std::string openFile(const std::string& path) override;
};

}  // namespace ember::win32::infra
