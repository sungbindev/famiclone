/*
 * ------------------------------------------------------------------------
 * 名称：memory-dropped-files-service.hpp
 * 说明：把拖入窗口的文件路径记在内存里的服务。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-19
 * 备注：最多保留 64 个文件：一次拖放用不了这么多，留出余量只为了界面还没来得及用完编号时不被顶掉；超出时丢弃最早登记的。
 * ------------------------------------------------------------------------
 */

#pragma once

#include <deque>
#include <utility>

#include "contracts/shell/dropped-files.hpp"

namespace ember::win32::infra {

/** 把拖入的文件路径记在内存里的服务。 */
class MemoryDroppedFilesService : public contracts::DroppedFilesService {
 public:
  [[nodiscard]] std::vector<contracts::DroppedFile> registerFiles(const std::vector<std::string>& paths) override;
  [[nodiscard]] std::optional<std::string> resolve(std::int64_t id) const override;

 private:
  std::int64_t nextId_{1};
  /** 按登记顺序排列的（编号，路径）。 */
  std::deque<std::pair<std::int64_t, std::string>> files_;
};

}  // namespace ember::win32::infra
