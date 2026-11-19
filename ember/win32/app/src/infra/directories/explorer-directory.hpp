/*
 * ------------------------------------------------------------------------
 * 名称：explorer-directory.hpp
 * 说明：用资源管理器打开的目录：目录的位置固定，不存在时先创建，再交给系统的文件管理器打开。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-19
 * 备注：日志目录与 BIOS 目录各创建一个。
 * ------------------------------------------------------------------------
 */

#pragma once

#include <filesystem>

#include "contracts/shell/openable-directory.hpp"

namespace ember::win32::infra {

/** 用资源管理器打开的目录。 */
class ExplorerDirectory : public contracts::OpenableDirectory {
 public:
  explicit ExplorerDirectory(std::filesystem::path path) : path_(std::move(path)) {}

  [[nodiscard]] std::string open() override;

 private:
  std::filesystem::path path_;
};

}  // namespace ember::win32::infra
