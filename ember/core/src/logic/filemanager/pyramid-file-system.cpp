/*
 * ------------------------------------------------------------------------
 * 名称：pyramid-file-system.cpp
 * 说明：金字塔 DOS 文件系统（只读）的实现。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-27
 * 备注：无。
 * ------------------------------------------------------------------------
 */

#include "logic/filemanager/pyramid-file-system.hpp"

#include <algorithm>

#include "contracts/filemanager/filemanager-error.hpp"
#include "logic/filemanager/disk-text.hpp"
#include "logic/filemanager/dos-catalog.hpp"
#include "shared/errors/core-error.hpp"

namespace ember::logic::filemanager {

namespace api = contracts::filemanager;
using namespace dos_catalog;

namespace {

constexpr unsigned PYRAMID_TRACKS = 40;
constexpr unsigned PYRAMID_SECTORS = 14;
constexpr std::size_t SECTOR_SIZE = 256;
constexpr std::size_t PYRAMID_IMAGE_SIZE = PYRAMID_TRACKS * PYRAMID_SECTORS * SECTOR_SIZE;

[[noreturn]] void fail(std::string_view code, const std::string& message) { throw shared::CoreError(code, message); }

[[noreturn]] void failReadOnly() { fail(api::ERROR_READ_ONLY, "金字塔 DOS 的镜像目前只能查看，不能修改"); }

}  // namespace

std::unique_ptr<PyramidFileSystem> PyramidFileSystem::open(std::vector<std::uint8_t>& image) {
  if (image.size() != PYRAMID_IMAGE_SIZE) return nullptr;
  // 卷目录表：目录起始磁道 17、每扇区 122 对、磁道数 40；“每道扇区数”字节是 16，不校验
  const std::uint8_t* table = image.data() + (VTOC_TRACK * PYRAMID_SECTORS) * SECTOR_SIZE;
  if (table[1] != VTOC_TRACK || table[0x27] != PAIRS_PER_LIST || table[0x34] != PYRAMID_TRACKS) return nullptr;
  return std::unique_ptr<PyramidFileSystem>(new PyramidFileSystem(image));
}

PyramidFileSystem::PyramidFileSystem(std::vector<std::uint8_t>& image) : image_(image) {
  traits_.formatId = "pyramid";
  traits_.formatName = "金字塔 DOS";
  traits_.writable = false;
  traits_.subdirectories = false;
}

void PyramidFileSystem::format(std::vector<std::uint8_t>& image) {
  std::fill(image.begin(), image.end(), static_cast<std::uint8_t>(0));
  const auto sector = [&](unsigned track, unsigned index) { return image.data() + (static_cast<std::size_t>(track) * PYRAMID_SECTORS + index) * SECTOR_SIZE; };
  // 卷目录表：结构同 DOS 3.3，每道扇区数字节沿用 16 的写法，磁道数 40，目录从 17 道 13 扇区开始
  std::uint8_t* table = sector(VTOC_TRACK, 0);
  table[1] = VTOC_TRACK;
  table[2] = PYRAMID_SECTORS - 1;
  table[3] = 3;
  table[6] = 254;
  table[0x27] = PAIRS_PER_LIST;
  table[0x30] = 1;
  table[0x34] = PYRAMID_TRACKS;
  table[0x35] = 16;
  table[0x37] = 1;
  // 空闲位图：每道只看前 14 位；系统占用 0 到 2 道与目录道
  for (unsigned track = 0; track < PYRAMID_TRACKS; ++track) {
    const bool used = track <= 2 || track == VTOC_TRACK;
    table[0x38 + 4 * track] = used ? 0x00 : 0x3F;
    table[0x38 + 4 * track + 1] = used ? 0x00 : 0xFF;
  }
  // 目录扇区从 13 到 1 依次链接，最后一个的下一个是 0
  for (unsigned index = PYRAMID_SECTORS - 1; index >= 1; --index) {
    std::uint8_t* catalog = sector(VTOC_TRACK, index);
    catalog[1] = index > 1 ? VTOC_TRACK : 0;
    catalog[2] = index > 1 ? static_cast<std::uint8_t>(index - 1) : 0;
  }
}

const std::uint8_t* PyramidFileSystem::sectorAt(unsigned track, unsigned sector) const {
  if (track >= PYRAMID_TRACKS || sector >= PYRAMID_SECTORS) fail(api::ERROR_CORRUPT, "磁盘的磁道扇区号超出范围，文件系统已损坏");
  return image_.data() + (static_cast<std::size_t>(track) * PYRAMID_SECTORS + sector) * SECTOR_SIZE;
}

std::vector<const std::uint8_t*> PyramidFileSystem::catalogEntries() const {
  std::vector<const std::uint8_t*> entries;
  unsigned track = vtoc()[1];
  unsigned sector = vtoc()[2];
  for (std::size_t visited = 0; track != 0 && visited < MAX_CHAIN; ++visited) {
    const std::uint8_t* page = sectorAt(track, sector);
    for (std::size_t i = 0; i < CATALOG_ENTRIES_PER_SECTOR; ++i) entries.push_back(page + CATALOG_FIRST_ENTRY + i * CATALOG_ENTRY_SIZE);
    track = page[1];
    sector = page[2];
  }
  return entries;
}

const std::uint8_t* PyramidFileSystem::findEntry(const api::EntryPath& path) const {
  if (path.size() != 1) fail(api::ERROR_ENTRY_NOT_FOUND, "找不到 " + (path.empty() ? std::string("条目") : path.back()));
  for (const std::uint8_t* entry : catalogEntries()) {
    if (entry[0] == 0) break;
    if (entry[0] == ENTRY_DELETED) continue;
    if (equalsIgnoreCase(entryName(entry), path.front())) return entry;
  }
  fail(api::ERROR_ENTRY_NOT_FOUND, "找不到 " + path.front());
}

bool PyramidFileSystem::sectorFree(unsigned track, unsigned sector) const {
  const std::uint8_t* bits = vtoc() + 0x38 + 4 * track;
  return (bits[sector >= 8 ? 0 : 1] & (1U << (sector & 7U))) != 0;
}

void PyramidFileSystem::collectFile(const std::uint8_t* entry, std::vector<TrackSector>& data) const {
  unsigned track = entry[0];
  unsigned sector = entry[1];
  for (std::size_t visited = 0; track != 0 && visited < MAX_CHAIN; ++visited) {
    const std::uint8_t* list = sectorAt(track, sector);
    const std::size_t base = readLe16(list + 5);
    for (std::size_t i = 0; i < PAIRS_PER_LIST; ++i) {
      const unsigned dataTrack = list[LIST_FIRST_PAIR + 2 * i];
      const unsigned dataSector = list[LIST_FIRST_PAIR + 2 * i + 1];
      if (dataTrack == 0 && dataSector == 0) continue;
      if (data.size() <= base + i) data.resize(base + i + 1, TrackSector{0, 0});
      data[base + i] = TrackSector{dataTrack, dataSector};
    }
    track = list[1];
    sector = list[2];
  }
}

api::FileSystemSpace PyramidFileSystem::space() {
  std::uint64_t freeSectors = 0;
  for (unsigned track = 0; track < PYRAMID_TRACKS; ++track) {
    for (unsigned sector = 0; sector < PYRAMID_SECTORS; ++sector) {
      if (sectorFree(track, sector)) ++freeSectors;
    }
  }
  return api::FileSystemSpace{PYRAMID_IMAGE_SIZE, freeSectors * SECTOR_SIZE};
}

std::vector<api::DiskEntry> PyramidFileSystem::list(const api::EntryPath& directory) {
  if (!directory.empty()) fail(api::ERROR_ENTRY_NOT_FOUND, "金字塔 DOS 磁盘没有文件夹");
  std::vector<api::DiskEntry> result;
  for (const std::uint8_t* entry : catalogEntries()) {
    if (entry[0] == 0) break;
    if (entry[0] == ENTRY_DELETED) continue;
    api::DiskEntry item;
    item.name = entryName(entry);
    item.type = typeName(entry[2]);
    item.size = static_cast<std::int64_t>(readLe16(entry + 0x21)) * static_cast<std::int64_t>(SECTOR_SIZE);
    item.sizeIsAllocated = true;
    item.locked = (entry[2] & LOCKED_FLAG) != 0;
    result.push_back(std::move(item));
  }
  return result;
}

void PyramidFileSystem::rename(const api::EntryPath&, const std::string&) { failReadOnly(); }

void PyramidFileSystem::remove(const api::EntryPath&) { failReadOnly(); }

void PyramidFileSystem::move(const api::EntryPath&, const api::EntryPath&) { fail(api::ERROR_UNSUPPORTED, "金字塔 DOS 磁盘没有文件夹，不能移动文件"); }

void PyramidFileSystem::makeDirectory(const api::EntryPath&, const std::string&) { fail(api::ERROR_UNSUPPORTED, "金字塔 DOS 磁盘不能新建文件夹"); }

api::ExportedFile PyramidFileSystem::readFile(const api::EntryPath& entry) {
  const std::uint8_t* found = findEntry(entry);
  std::vector<TrackSector> data;
  collectFile(found, data);
  std::vector<std::uint8_t> content;
  content.reserve(data.size() * SECTOR_SIZE);
  for (const auto& [track, sector] : data) {
    if (track == 0) {
      content.insert(content.end(), SECTOR_SIZE, 0);
      continue;
    }
    const std::uint8_t* page = sectorAt(track, sector);
    content.insert(content.end(), page, page + SECTOR_SIZE);
  }
  const std::uint8_t type = found[2] & 0x7FU;
  std::size_t length = content.size();
  if (type == 0x04 && content.size() >= 4) length = std::min<std::size_t>(content.size(), 4U + readLe16(content.data() + 2));
  else if ((type == 0x01 || type == 0x02) && content.size() >= 2) length = std::min<std::size_t>(content.size(), 2U + readLe16(content.data()));
  else if (type == 0x00) {
    while (length > 0 && content[length - 1] == 0) --length;
  }
  content.resize(length);
  api::ExportedFile file;
  file.name = windowsSafeName(entryName(found)) + typeExtension(found[2]);
  file.data = std::move(content);
  return file;
}

std::string PyramidFileSystem::writeFile(const api::EntryPath&, const std::string&, std::span<const std::uint8_t>) { failReadOnly(); }

}  // namespace ember::logic::filemanager
