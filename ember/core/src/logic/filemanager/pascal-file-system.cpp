/*
 * ------------------------------------------------------------------------
 * 名称：pascal-file-system.cpp
 * 说明：Apple Pascal 文件系统（只读）的实现。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-28
 * 备注：无。
 * ------------------------------------------------------------------------
 */

#include "logic/filemanager/pascal-file-system.hpp"

#include <algorithm>
#include <cstdio>

#include "contracts/filemanager/filemanager-error.hpp"
#include "logic/filemanager/disk-text.hpp"
#include "shared/errors/core-error.hpp"

namespace ember::logic::filemanager {

namespace api = contracts::filemanager;

namespace {

constexpr std::size_t BLOCK_SIZE = 512;
/** 卷目录从块 2 开始，占块 2 到 5。 */
constexpr std::uint32_t KEY_BLOCK = 2;
constexpr std::uint32_t DIRECTORY_BLOCKS = 4;
/** 卷头的字节数；目录项从它后面开始。 */
constexpr std::size_t HEADER_SIZE = 22;
constexpr std::size_t ENTRY_SIZE = 26;
/** 引导块与卷目录占用的块数，文件从块 6 开始。 */
constexpr std::uint32_t FIRST_FILE_BLOCK = 6;
constexpr std::size_t NAME_LENGTH = 15;

[[noreturn]] void fail(std::string_view code, const std::string& message) { throw shared::CoreError(code, message); }

[[noreturn]] void failReadOnly() { fail(api::ERROR_READ_ONLY, "Apple Pascal 的镜像目前只能查看，不能修改"); }

/** 文件种类的名称。 */
struct TypeName {
  std::uint8_t kind;
  const char* name;
};
constexpr TypeName TYPE_NAMES[] = {{0, "VOL"}, {1, "BAD"}, {2, "CODE"}, {3, "TEXT"}, {4, "INFO"}, {5, "DATA"}, {6, "GRAF"}, {7, "FOTO"}, {8, "SEC"}};

std::string typeText(std::uint8_t kind) {
  for (const TypeName& entry : TYPE_NAMES) {
    if (entry.kind == kind) return entry.name;
  }
  char text[8];
  std::snprintf(text, sizeof(text), "$%02X", kind);
  return text;
}

/** 目录项的日期（两个字节：日、月、年）写成 YYYY-MM-DD；没有日期或日期不合法时为空。 */
std::string dateText(const std::uint8_t* entry) {
  const std::uint16_t date = readLe16(entry + 2);
  const int day = date & 0x1FU;
  const int month = (date >> 5) & 0x0FU;
  int year = (date >> 9) & 0x7FU;
  if (date == 0 || day < 1 || day > 31 || month < 1 || month > 12) return {};
  year = year < 40 ? 2000 + year : 1900 + year;
  char text[16];
  std::snprintf(text, sizeof(text), "%04d-%02d-%02d", year, month, day);
  return text;
}

}  // namespace

std::unique_ptr<PascalFileSystem> PascalFileSystem::open(std::vector<std::uint8_t>& image, const AppleIdentity& identity) {
  return std::unique_ptr<PascalFileSystem>(new PascalFileSystem(image, identity));
}

PascalFileSystem::PascalFileSystem(std::vector<std::uint8_t>& image, const AppleIdentity& identity)
    : linear_(appleToLinear(image, identity.fileOrder, AppleOrder::ProDos, identity.tracks)) {
  totalBlocks_ = static_cast<std::uint32_t>(std::min<std::size_t>(readLe16(linear_.data() + KEY_BLOCK * BLOCK_SIZE + 0x0E), linear_.size() / BLOCK_SIZE));
  traits_.formatId = "pascal";
  traits_.formatName = "Apple Pascal";
  traits_.writable = false;
  traits_.subdirectories = false;
}

const std::uint8_t* PascalFileSystem::blockAt(std::uint32_t block) const {
  if (block >= totalBlocks_) fail(api::ERROR_CORRUPT, "磁盘的块号超出范围，文件系统已损坏");
  return linear_.data() + static_cast<std::size_t>(block) * BLOCK_SIZE;
}

std::vector<const std::uint8_t*> PascalFileSystem::catalogEntries() const {
  const std::uint32_t fileCount = readLe16(blockAt(KEY_BLOCK) + 0x10);
  std::vector<const std::uint8_t*> entries;
  std::size_t position = KEY_BLOCK * BLOCK_SIZE + HEADER_SIZE;
  const std::size_t end = (KEY_BLOCK + DIRECTORY_BLOCKS) * BLOCK_SIZE;
  for (std::uint32_t index = 0; index < fileCount; ++index) {
    // 一项放不下本块剩余空间时从下一块开头继续
    if (position % BLOCK_SIZE + ENTRY_SIZE > BLOCK_SIZE) position = (position / BLOCK_SIZE + 1) * BLOCK_SIZE;
    if (position + ENTRY_SIZE > end) fail(api::ERROR_CORRUPT, "磁盘的目录超出范围，文件系统已损坏");
    entries.push_back(linear_.data() + position);
    position += ENTRY_SIZE;
  }
  return entries;
}

PascalFileSystem::BlockRange PascalFileSystem::rangeOf(const std::uint8_t* entry) const {
  const BlockRange range{readLe16(entry + 4), readLe16(entry + 6)};
  if (range.first < FIRST_FILE_BLOCK || range.last <= range.first || range.last > totalBlocks_) fail(api::ERROR_CORRUPT, "磁盘的块号超出范围，文件系统已损坏");
  return range;
}

std::string PascalFileSystem::entryName(const std::uint8_t* entry) {
  const std::size_t length = std::min<std::size_t>(entry[10], NAME_LENGTH);
  return oemToUtf8(std::string_view(reinterpret_cast<const char*>(entry + 11), length));
}

std::int64_t PascalFileSystem::entrySize(const std::uint8_t* entry, const BlockRange& range) {
  const std::uint32_t bytesInLast = readLe16(entry);
  const std::int64_t blocks = static_cast<std::int64_t>(range.last - range.first);
  if (bytesInLast == 0 || bytesInLast >= BLOCK_SIZE) return blocks * static_cast<std::int64_t>(BLOCK_SIZE);
  return (blocks - 1) * static_cast<std::int64_t>(BLOCK_SIZE) + bytesInLast;
}

const std::uint8_t* PascalFileSystem::findEntry(const api::EntryPath& path) const {
  if (path.size() != 1) fail(api::ERROR_ENTRY_NOT_FOUND, "找不到 " + (path.empty() ? std::string("条目") : path.back()));
  for (const std::uint8_t* entry : catalogEntries()) {
    if (equalsIgnoreCase(entryName(entry), path.front())) return entry;
  }
  fail(api::ERROR_ENTRY_NOT_FOUND, "找不到 " + path.front());
}

api::FileSystemSpace PascalFileSystem::space() {
  std::vector<char> used(totalBlocks_, 0);
  for (std::uint32_t block = 0; block < FIRST_FILE_BLOCK && block < totalBlocks_; ++block) used[block] = 1;
  for (const std::uint8_t* entry : catalogEntries()) {
    const BlockRange range = rangeOf(entry);
    for (std::uint32_t block = range.first; block < range.last; ++block) used[block] = 1;
  }
  std::uint64_t freeBlocks = 0;
  for (std::uint32_t block = FIRST_FILE_BLOCK; block < totalBlocks_; ++block) {
    if (used[block] == 0) ++freeBlocks;
  }
  const std::uint64_t capacityBlocks = totalBlocks_ > FIRST_FILE_BLOCK ? totalBlocks_ - FIRST_FILE_BLOCK : 0;
  return api::FileSystemSpace{capacityBlocks * BLOCK_SIZE, freeBlocks * BLOCK_SIZE};
}

std::vector<api::DiskEntry> PascalFileSystem::list(const api::EntryPath& directory) {
  if (!directory.empty()) fail(api::ERROR_ENTRY_NOT_FOUND, "Apple Pascal 磁盘没有文件夹");
  std::vector<api::DiskEntry> result;
  for (const std::uint8_t* entry : catalogEntries()) {
    const BlockRange range = rangeOf(entry);
    const std::uint8_t kind = static_cast<std::uint8_t>(readLe16(entry + 8) & 0x0FU);
    api::DiskEntry item;
    item.name = entryName(entry);
    item.type = typeText(kind);
    item.size = entrySize(entry, range);
    item.modified = dateText(entry);
    item.locked = kind == 8;
    result.push_back(std::move(item));
  }
  return result;
}

void PascalFileSystem::rename(const api::EntryPath&, const std::string&) { failReadOnly(); }

void PascalFileSystem::remove(const api::EntryPath&) { failReadOnly(); }

void PascalFileSystem::move(const api::EntryPath&, const api::EntryPath&) { fail(api::ERROR_UNSUPPORTED, "Apple Pascal 磁盘没有文件夹，不能移动文件"); }

void PascalFileSystem::makeDirectory(const api::EntryPath&, const std::string&) { fail(api::ERROR_UNSUPPORTED, "Apple Pascal 磁盘不能新建文件夹"); }

api::ExportedFile PascalFileSystem::readFile(const api::EntryPath& entry) {
  const std::uint8_t* found = findEntry(entry);
  const BlockRange range = rangeOf(found);
  std::vector<std::uint8_t> content;
  content.reserve(static_cast<std::size_t>(range.last - range.first) * BLOCK_SIZE);
  for (std::uint32_t block = range.first; block < range.last; ++block) {
    const std::uint8_t* page = blockAt(block);
    content.insert(content.end(), page, page + BLOCK_SIZE);
  }
  content.resize(static_cast<std::size_t>(entrySize(found, range)));
  api::ExportedFile file;
  file.name = windowsSafeName(entryName(found));
  file.data = std::move(content);
  return file;
}

std::string PascalFileSystem::writeFile(const api::EntryPath&, const std::string&, std::span<const std::uint8_t>) { failReadOnly(); }

}  // namespace ember::logic::filemanager