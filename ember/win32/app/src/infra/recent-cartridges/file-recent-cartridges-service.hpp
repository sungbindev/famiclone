/*
 * ------------------------------------------------------------------------
 * 名称：file-recent-cartridges-service.hpp
 * 说明：最近打开的卡带服务的实现：启动时从本地文件读出并逐项校验（文件不存在或损坏时得到空清单，不报错弹窗），修改后交给文件存储防抖写盘。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-19
 * 备注：文件存储由组装根注入；文件是 config/recent-cartridges.json；每个路径在本次运行内有固定的编号（从 1 递增），重新加入已有的路径时编号不变。
 * ------------------------------------------------------------------------
 */

#pragma once

#include <map>

#include "contracts/cartridges/recent-cartridges-service.hpp"
#include "platform/files/json-file-store.hpp"

namespace ember::win32::infra {

/** 保存在本地文件里的最近打开的卡带服务。 */
class FileRecentCartridgesService : public contracts::RecentCartridgesService {
 public:
  explicit FileRecentCartridgesService(platform::JsonFileStore& store);

  [[nodiscard]] std::vector<contracts::RecentCartridge> list() const override;
  void add(const std::string& path) override;
  void remove(const std::string& path) override;
  [[nodiscard]] std::optional<std::string> resolve(std::int64_t id) const override;
  void flush() override { store_.flush(); }

 private:
  std::int64_t idOf(const std::string& path) const;
  void replace(std::vector<std::string> next);

  platform::JsonFileStore& store_;
  std::vector<std::string> paths_;
  mutable std::map<std::string, std::int64_t> ids_;
  mutable std::int64_t nextId_{1};
};

}  // namespace ember::win32::infra
