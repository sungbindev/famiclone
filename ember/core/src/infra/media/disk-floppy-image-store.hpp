/*
 * ------------------------------------------------------------------------
 * 名称：disk-floppy-image-store.hpp
 * 说明：盘片镜像文件存取的磁盘实现：读取时检查文件存在、是普通文件、大小在上限之内；写回时先写临时文件再改名。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-15
 * ------------------------------------------------------------------------
 */

#pragma once

#include <cstdint>

#include "contracts/media/floppy-image-store.hpp"

namespace ember::infra::media {

/** 磁盘上的盘片镜像存取。 */
class DiskFloppyImageStore final : public contracts::media::FloppyImageStore {
 public:
  /** 镜像文件的大小上限（字节）。 */
  static constexpr std::uintmax_t MAX_FILE_SIZE = 16ULL * 1024 * 1024;

  [[nodiscard]] std::vector<std::uint8_t> read(const std::string& path) const override;
  void write(const std::string& path, const std::vector<std::uint8_t>& bytes) override;
};

}  // namespace ember::infra::media
