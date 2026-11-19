/*
 * ------------------------------------------------------------------------
 * 名称：file-last-directories-service.hpp
 * 说明：记住上次目录服务的实现：启动时从本地文件读出并逐项校验（文件不存在或损坏时得到空记录），修改后交给文件存储防抖写盘。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-19
 * 备注：文件存储由组装根注入；文件是 config/last-directories.json。
 * ------------------------------------------------------------------------
 */

#pragma once

#include <map>

#include "contracts/shell/last-directories-service.hpp"
#include "platform/files/json-file-store.hpp"

namespace ember::win32::infra {

/** 保存在本地文件里的记住上次目录服务。 */
class FileLastDirectoriesService : public contracts::LastDirectoriesService {
 public:
  explicit FileLastDirectoriesService(platform::JsonFileStore& store);

  [[nodiscard]] std::optional<std::string> get(const std::string& purpose) const override;
  void set(const std::string& purpose, const std::string& directory) override;
  void flush() override { store_.flush(); }

 private:
  platform::JsonFileStore& store_;
  std::map<std::string, std::string> directories_;
};

}  // namespace ember::win32::infra
