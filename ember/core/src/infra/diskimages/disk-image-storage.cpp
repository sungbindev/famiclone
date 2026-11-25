/*
 * ------------------------------------------------------------------------
 * 名称：disk-image-storage.cpp
 * 说明：镜像目录里文件存取的磁盘实现。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-25
 * 备注：无。
 * ------------------------------------------------------------------------
 */

#include "infra/diskimages/disk-image-storage.hpp"

#include <algorithm>
#include <filesystem>
#include <fstream>

#include "contracts/diskimages/diskimages-error.hpp"
#include "shared/errors/core-error.hpp"

namespace ember::infra::diskimages {

namespace api = contracts::diskimages;
namespace fs = std::filesystem;

namespace {

/** 扫描时文件夹的最大深度。 */
constexpr int MAX_SCAN_DEPTH = 32;
/** 读取拖入内容的上限：层数、个数、总字节数。 */
constexpr int MAX_HOST_DEPTH = 16;
constexpr std::size_t MAX_HOST_ITEMS = 5000;
constexpr std::size_t MAX_HOST_BYTES = 64U * 1024U * 1024U;
/** 新建镜像文件时重名后缀的最大序号。 */
constexpr int MAX_NAME_SUFFIX = 9999;

[[noreturn]] void fail(std::string_view code, const std::string& message) { throw shared::CoreError(code, message); }

fs::path toPath(const std::string& text) { return fs::path(std::u8string(text.begin(), text.end())); }

std::string toUtf8(const fs::path& path) {
  const auto text = path.u8string();
  return std::string(text.begin(), text.end());
}

std::string lowered(std::string text) {
  for (char& c : text) {
    if (c >= 'A' && c <= 'Z') c = static_cast<char>(c - 'A' + 'a');
  }
  return text;
}

/** 文件扩展名（小写，不带点）是不是支持的镜像扩展名。 */
bool isSupportedExtension(const fs::path& path) {
  std::string extension = lowered(toUtf8(path.extension()));
  if (!extension.empty()) extension.erase(extension.begin());
  return std::any_of(std::begin(api::SUPPORTED_EXTENSIONS), std::end(api::SUPPORTED_EXTENSIONS), [&extension](const char* supported) { return extension == supported; });
}

/** 相对路径的写法：不能是空的、绝对的、带盘符或反斜杠，每一级不能是空、“.”、“..”，也不能含 Windows 文件名里不允许的字符与控制字符。 */
void requireSafeRelative(const std::string& relative) {
  const std::string message = "路径不合格：" + relative;
  if (relative.empty() || relative.front() == '/' || relative.find('\\') != std::string::npos || relative.find(':') != std::string::npos) fail(api::ERROR_INVALID_PATH, message);
  std::size_t start = 0;
  while (true) {
    const std::size_t slash = relative.find('/', start);
    const std::string part = relative.substr(start, slash == std::string::npos ? std::string::npos : slash - start);
    if (part.empty() || part == "." || part == ".." || part.find_first_of("<>\"|?*") != std::string::npos) fail(api::ERROR_INVALID_PATH, message);
    if (std::any_of(part.begin(), part.end(), [](char c) { return static_cast<unsigned char>(c) < 0x20; })) fail(api::ERROR_INVALID_PATH, message);
    if (slash == std::string::npos) break;
    start = slash + 1;
  }
}

/** path 规范化之后是否在 root 里面（符号链接指到外面也不算）。 */
bool isInside(const fs::path& root, const fs::path& path, bool allowSame) {
  std::error_code error;
  const fs::path canonicalRoot = fs::weakly_canonical(root, error);
  if (error) return false;
  const fs::path canonicalPath = fs::weakly_canonical(path, error);
  if (error) return false;
  const auto mismatch = std::mismatch(canonicalRoot.begin(), canonicalRoot.end(), canonicalPath.begin(), canonicalPath.end());
  if (mismatch.first != canonicalRoot.end()) return false;
  return allowSame || canonicalPath != canonicalRoot;
}

/** 镜像目录下相对路径指向的镜像文件：路径要合格、扩展名要支持、规范化后要在目录里。 */
fs::path resolveImage(const std::string& root, const std::string& relative) {
  requireSafeRelative(relative);
  const fs::path rootPath = toPath(root);
  const fs::path full = rootPath / toPath(relative);
  if (!isSupportedExtension(full) || !isInside(rootPath, full, false)) fail(api::ERROR_INVALID_PATH, "不是镜像目录里的镜像文件：" + relative);
  return full;
}

/** 存在的普通文件。 */
fs::path resolveExistingImage(const std::string& root, const std::string& relative) {
  const fs::path full = resolveImage(root, relative);
  std::error_code error;
  if (!fs::is_regular_file(full, error) || error) fail(api::ERROR_IMAGE_NOT_FOUND, "找不到镜像文件：" + relative);
  return full;
}

/** 一次扫描的累计。 */
struct ScanState {
  std::size_t limit{0};
  std::size_t count{0};
  bool limited{false};
};

void walk(const fs::path& directory, const std::string& prefix, int depth, ScanState& state, std::vector<api::StoredImageInfo>& out) {
  if (depth > MAX_SCAN_DEPTH || state.limited) return;
  std::error_code error;
  fs::directory_iterator iterator(directory, fs::directory_options::skip_permission_denied, error);
  if (error) return;
  for (; iterator != fs::directory_iterator(); iterator.increment(error)) {
    if (error) break;
    const fs::directory_entry& entry = *iterator;
    std::error_code statusError;
    // 符号链接与联接点不跟随：它们可能指到镜像目录之外
    if (entry.is_symlink(statusError) || statusError) continue;
    const std::string name = toUtf8(entry.path().filename());
    const std::string relative = prefix.empty() ? name : prefix + "/" + name;
    if (entry.is_directory(statusError)) {
      std::vector<api::StoredImageInfo> inner;
      walk(entry.path(), relative, depth + 1, state, inner);
      if (!inner.empty()) {
        out.push_back(api::StoredImageInfo{relative, true, 0, 0});
        out.insert(out.end(), inner.begin(), inner.end());
      }
    } else if (entry.is_regular_file(statusError) && isSupportedExtension(entry.path())) {
      if (state.count >= state.limit) {
        state.limited = true;
        return;
      }
      ++state.count;
      std::error_code sizeError;
      std::error_code timeError;
      const std::uintmax_t size = entry.file_size(sizeError);
      const auto written = entry.last_write_time(timeError);
      out.push_back(api::StoredImageInfo{relative, false, sizeError ? 0 : size, timeError ? 0 : static_cast<std::int64_t>(written.time_since_epoch().count())});
    }
  }
}

/** 读取拖入内容时的累计。 */
struct HostBudget {
  std::size_t items{0};
  std::size_t bytes{0};
};

api::HostItem readHost(const fs::path& path, int depth, HostBudget& budget) {
  const std::string name = toUtf8(path.filename());
  if (depth > MAX_HOST_DEPTH || ++budget.items > MAX_HOST_ITEMS) fail(api::ERROR_INVALID_PATH, "拖入的内容太多（最多 5000 个文件、16 层文件夹）：" + name);
  std::error_code error;
  api::HostItem item;
  item.name = name;
  if (fs::is_directory(path, error)) {
    item.directory = true;
    fs::directory_iterator iterator(path, error);
    if (error) fail(api::ERROR_INVALID_PATH, "无法读取文件夹：" + name);
    for (; iterator != fs::directory_iterator(); iterator.increment(error)) {
      if (error) fail(api::ERROR_INVALID_PATH, "无法读取文件夹：" + name);
      std::error_code statusError;
      if (iterator->is_symlink(statusError) || statusError) continue;
      item.children.push_back(readHost(iterator->path(), depth + 1, budget));
    }
    std::sort(item.children.begin(), item.children.end(), [](const api::HostItem& a, const api::HostItem& b) {
      if (a.directory != b.directory) return a.directory;
      return lowered(a.name) < lowered(b.name);
    });
    return item;
  }
  if (!fs::is_regular_file(path, error)) fail(api::ERROR_INVALID_PATH, "找不到拖入的文件，或它不是普通文件：" + name);
  const std::uintmax_t size = fs::file_size(path, error);
  if (error) fail(api::ERROR_INVALID_PATH, "无法读取文件：" + name);
  if (size + budget.bytes > MAX_HOST_BYTES) fail(api::ERROR_INVALID_PATH, "拖入的内容超过 64 MB：" + name);
  std::ifstream stream(path, std::ios::binary);
  item.data.resize(static_cast<std::size_t>(size));
  stream.read(reinterpret_cast<char*>(item.data.data()), static_cast<std::streamsize>(item.data.size()));
  if (!stream || stream.gcount() != static_cast<std::streamsize>(item.data.size())) fail(api::ERROR_INVALID_PATH, "无法读取文件：" + name);
  budget.bytes += item.data.size();
  return item;
}

}  // namespace

bool DiskImageStorageOnDisk::isDirectory(const std::string& path) const {
  std::error_code error;
  return fs::is_directory(toPath(path), error) && !error;
}

std::vector<api::StoredImageInfo> DiskImageStorageOnDisk::scan(const std::string& root, std::size_t limit, bool& limited, bool& available) const {
  limited = false;
  available = isDirectory(root);
  std::vector<api::StoredImageInfo> result;
  if (!available) return result;
  ScanState state;
  state.limit = limit;
  walk(toPath(root), {}, 0, state, result);
  limited = state.limited;
  return result;
}

std::vector<std::uint8_t> DiskImageStorageOnDisk::read(const std::string& root, const std::string& relative) const {
  const fs::path path = resolveExistingImage(root, relative);
  std::error_code error;
  const std::uintmax_t size = fs::file_size(path, error);
  if (error) fail(api::ERROR_IMAGE_UNREADABLE, "无法读取镜像文件：" + relative);
  if (size == 0) fail(api::ERROR_IMAGE_UNREADABLE, "镜像文件是空的：" + relative);
  if (size > api::MAX_IMAGE_SIZE) fail(api::ERROR_IMAGE_UNREADABLE, "镜像文件超过 16 MB，不是软盘镜像：" + relative);
  std::ifstream stream(path, std::ios::binary);
  if (!stream) fail(api::ERROR_IMAGE_UNREADABLE, "无法打开镜像文件：" + relative);
  std::vector<std::uint8_t> bytes(static_cast<std::size_t>(size));
  stream.read(reinterpret_cast<char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
  if (stream.gcount() != static_cast<std::streamsize>(bytes.size())) fail(api::ERROR_IMAGE_UNREADABLE, "读取镜像文件时中断：" + relative);
  return bytes;
}

void DiskImageStorageOnDisk::writeAt(const std::string& root, const std::string& relative, std::size_t offset, std::span<const std::uint8_t> bytes) {
  const fs::path path = resolveExistingImage(root, relative);
  std::error_code error;
  const std::uintmax_t size = fs::file_size(path, error);
  // 文件被别的程序改短或换掉时不在这里重建，直接报错让用户知道
  if (error || size < offset + bytes.size()) fail(api::ERROR_SAVE_FAILED, "镜像文件被改动过，无法写入：" + relative);
  std::fstream stream(path, std::ios::binary | std::ios::in | std::ios::out);
  stream.seekp(static_cast<std::streamoff>(offset));
  stream.write(reinterpret_cast<const char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
  stream.flush();
  if (!stream) fail(api::ERROR_SAVE_FAILED, "无法写入镜像文件：" + relative);
}

std::string DiskImageStorageOnDisk::create(const std::string& root, const std::string& relativeDirectory, const std::string& baseName, const std::string& extension, const std::vector<std::uint8_t>& bytes) {
  const fs::path rootPath = toPath(root);
  fs::path directory = rootPath;
  if (!relativeDirectory.empty()) {
    requireSafeRelative(relativeDirectory);
    directory = rootPath / toPath(relativeDirectory);
  }
  std::error_code error;
  if (!isInside(rootPath, directory, true) || !fs::is_directory(directory, error) || error) fail(api::ERROR_INVALID_PATH, "不是镜像目录里的文件夹：" + relativeDirectory);
  std::string fileName;
  fs::path target;
  for (int number = 1; number <= MAX_NAME_SUFFIX; ++number) {
    fileName = baseName + (number == 1 ? std::string() : " (" + std::to_string(number) + ")") + "." + extension;
    target = directory / toPath(fileName);
    if (!fs::exists(target, error)) break;
    if (number == MAX_NAME_SUFFIX) fail(api::ERROR_SAVE_FAILED, "同名的镜像文件太多");
  }
  fs::path temporary = target;
  temporary += ".tmp";
  {
    std::ofstream stream(temporary, std::ios::binary | std::ios::trunc);
    stream.write(reinterpret_cast<const char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
    stream.flush();
    if (!stream) {
      stream.close();
      fs::remove(temporary, error);
      fail(api::ERROR_SAVE_FAILED, "无法创建镜像文件：" + fileName);
    }
  }
  fs::rename(temporary, target, error);
  if (error) {
    fs::remove(temporary, error);
    fail(api::ERROR_SAVE_FAILED, "无法创建镜像文件：" + fileName);
  }
  return relativeDirectory.empty() ? fileName : relativeDirectory + "/" + fileName;
}

std::string DiskImageStorageOnDisk::absolutePath(const std::string& root, const std::string& relative) const {
  std::error_code error;
  const fs::path absolute = fs::absolute(resolveExistingImage(root, relative), error);
  if (error) fail(api::ERROR_IMAGE_NOT_FOUND, "找不到镜像文件：" + relative);
  return toUtf8(absolute);
}

bool DiskImageStorageOnDisk::samePath(const std::string& a, const std::string& b) const {
  std::error_code error;
  const fs::path left = toPath(a);
  const fs::path right = toPath(b);
  const bool equivalent = fs::equivalent(left, right, error);
  if (!error) return equivalent;
  // 其中一个已经不存在：只能按写法比较
  return lowered(toUtf8(left.lexically_normal())) == lowered(toUtf8(right.lexically_normal()));
}

api::HostItem DiskImageStorageOnDisk::readHostItem(const std::string& path) const {
  HostBudget budget;
  return readHost(toPath(path), 0, budget);
}

}  // namespace ember::infra::diskimages
