/*
 * ------------------------------------------------------------------------
 * 名称：pyramid-file-system.hpp
 * 说明：金字塔 PEC-9588 磁盘操作系统（PEC LM-DOS）的文件系统：40 道、每道 14 个扇区，只读（列目录、导出文件）。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-27
 * 备注：目录结构与 DOS 3.3 相同（17 道 0 扇区是卷目录表，目录项与 T/S 列表也一样），T/S 引用是逻辑扇区号；
 *       镜像里的扇区按逻辑扇区号排列（偏移 = (磁道 × 14 + 逻辑扇区) × 256），与金字塔 .dsk 相同，
 *       .nib、.woz 由位流解码器按交错表解成这种排列后交给这里；
 *       卷目录表的“每道扇区数”字节仍是 16（沿用 DOS 3.3 的写法），不按它校验；空闲位图只看每道前 14 位。
 * ------------------------------------------------------------------------
 */

#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "contracts/filemanager/disk-file-system.hpp"

namespace ember::logic::filemanager {

/** 金字塔 DOS 文件系统（只读）。 */
class PyramidFileSystem final : public contracts::filemanager::DiskFileSystem {
 public:
  /**
   * 打开按逻辑扇区号排列的金字塔盘镜像（143360 字节）。
   * @param image 镜像字节，只读；必须活得比返回的对象久。
   * @return 卷目录表不像金字塔盘（大小不对、目录起始磁道不是 17、每扇区不是 122 对、磁道数不是 40）时为空。
   */
  [[nodiscard]] static std::unique_ptr<PyramidFileSystem> open(std::vector<std::uint8_t>& image);

  /** 把 143360 字节的镜像格式化成空的金字塔盘：卷目录表、目录链与空闲位图，数据扇区清零（同机器里 INIT 后的结果）。 */
  static void format(std::vector<std::uint8_t>& image);

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
  /** 一个磁道扇区位置（逻辑扇区号）。 */
  using TrackSector = std::pair<unsigned, unsigned>;

  explicit PyramidFileSystem(std::vector<std::uint8_t>& image);

  [[nodiscard]] const std::uint8_t* sectorAt(unsigned track, unsigned sector) const;
  [[nodiscard]] const std::uint8_t* vtoc() const { return sectorAt(17, 0); }
  /** 目录里全部目录项的位置（含空的与已删除的）。 */
  [[nodiscard]] std::vector<const std::uint8_t*> catalogEntries() const;
  /** 按名称找到有效的目录项。 */
  [[nodiscard]] const std::uint8_t* findEntry(const contracts::filemanager::EntryPath& path) const;
  [[nodiscard]] bool sectorFree(unsigned track, unsigned sector) const;
  /** 一个文件占用的全部数据扇区，按文件内的顺序（没有数据的空洞是 {0, 0}）。 */
  void collectFile(const std::uint8_t* entry, std::vector<TrackSector>& data) const;

  std::vector<std::uint8_t>& image_;
  contracts::filemanager::FileSystemTraits traits_;
};

}  // namespace ember::logic::filemanager
