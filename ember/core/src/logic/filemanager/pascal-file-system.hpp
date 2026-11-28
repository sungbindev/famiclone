/*
 * ------------------------------------------------------------------------
 * 名称：pascal-file-system.hpp
 * 说明：Apple Pascal 磁盘的文件系统：只读（列目录、导出文件）。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-28
 * 备注：块排列与 ProDOS 相同（相邻两个扇区组成一块，块 b 在 b × 512）；块 0、1 是引导块，块 2 到 5 是卷目录：
 *       块 2 开头 22 字节是卷头（起始块 0、下一块 6、类型 0、卷名、总块数、文件数），后面是 26 字节一项的目录，
 *       一项放不下本块剩余空间时从下一块开头继续；文件占连续的一段块，没有子目录。
 * ------------------------------------------------------------------------
 */

#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "contracts/filemanager/disk-file-system.hpp"
#include "logic/filemanager/apple-image.hpp"

namespace ember::logic::filemanager {

/** Apple Pascal 文件系统（只读）。 */
class PascalFileSystem final : public contracts::filemanager::DiskFileSystem {
 public:
  /**
   * 打开识别为 Apple Pascal 的镜像。
   * @param image 镜像字节，只读；必须活得比返回的对象久。
   * @param identity 识别结果（扇区顺序与道数）。
   */
  [[nodiscard]] static std::unique_ptr<PascalFileSystem> open(std::vector<std::uint8_t>& image, const AppleIdentity& identity);

  [[nodiscard]] const contracts::filemanager::FileSystemTraits& traits() const override { return traits_; }
  [[nodiscard]] contracts::filemanager::FileSystemSpace space() override;
  [[nodiscard]] std::vector<contracts::filemanager::DiskEntry> list(const contracts::filemanager::EntryPath& directory) override;
  void rename(const contracts::filemanager::EntryPath& entry, const std::string& name) override;
  void remove(const contracts::filemanager::EntryPath& entry) override;
  void move(const contracts::filemanager::EntryPath& entry, const contracts::filemanager::EntryPath& targetDirectory) override;
  void makeDirectory(const contracts::filemanager::EntryPath& directory, const std::string& name) override;
  [[nodiscard]] contracts::filemanager::ExportedFile readFile(const contracts::filemanager::EntryPath& entry) override;
  std::string writeFile(const contracts::filemanager::EntryPath& directory, const std::string& windowsName, std::span<const std::uint8_t> content) override;

 private:
  /** 一个文件占用的块区间：[first, last)。 */
  struct BlockRange {
    std::uint32_t first{0};
    std::uint32_t last{0};
  };

  PascalFileSystem(std::vector<std::uint8_t>& image, const AppleIdentity& identity);

  [[nodiscard]] const std::uint8_t* blockAt(std::uint32_t block) const;
  /** 卷目录里全部目录项的位置，按磁盘上的顺序。 */
  [[nodiscard]] std::vector<const std::uint8_t*> catalogEntries() const;
  /** 按名称找到目录项。 */
  [[nodiscard]] const std::uint8_t* findEntry(const contracts::filemanager::EntryPath& path) const;
  /** 目录项占用的块区间；区间不合法时抛 filemanager.corrupt。 */
  [[nodiscard]] BlockRange rangeOf(const std::uint8_t* entry) const;
  /** 目录项的文件名。 */
  [[nodiscard]] static std::string entryName(const std::uint8_t* entry);
  /** 目录项的文件大小（字节）：整块数乘 512 加最后一块里的字节数（最后一块字节数为 0 表示占满整块）。 */
  [[nodiscard]] static std::int64_t entrySize(const std::uint8_t* entry, const BlockRange& range);

  std::vector<std::uint8_t> linear_;
  std::uint32_t totalBlocks_{0};
  contracts::filemanager::FileSystemTraits traits_;
};

}  // namespace ember::logic::filemanager