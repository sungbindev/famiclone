/*
 * ------------------------------------------------------------------------
 * 名称：user-data-location.cpp
 * 说明：用户数据目录的位置规则与移动的实现。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-21
 * 备注：不抛异常，文件系统的错误都用 error_code 接住并转成中文说明。
 * ------------------------------------------------------------------------
 */

#include "platform/files/user-data-location.hpp"

#include <windows.h>

#include <cwctype>
#include <fstream>
#include <map>
#include <system_error>

#include "platform/files/directory-access.hpp"
#include "platform/text/wide-text.hpp"

namespace ember::win32::platform {

namespace {

namespace fs = std::filesystem;

constexpr const wchar_t* kLockFileName = L"lockfile";
constexpr const wchar_t* kRunDirectoryName = L"run";
constexpr const wchar_t* kStagingSuffix = L".tmp";
constexpr const wchar_t* kRetiredDirectoryName = L"ember-data.old";

/** 相对路径 -> 文件大小（路径统一小写，Windows 的文件名不分大小写）。 */
using FileSizes = std::map<std::wstring, std::uintmax_t>;

std::wstring lowerCase(std::wstring text) {
  for (wchar_t& c : text) c = static_cast<wchar_t>(::towlower(c));
  return text;
}

/** 相对路径的第一段是不是运行时才有的东西（单实例锁、调试服务的端点目录）。 */
bool isRuntimeEntry(const fs::path& relative) {
  if (relative.empty()) return false;
  const std::wstring first = lowerCase(relative.begin()->wstring());
  return first == kLockFileName || first == kRunDirectoryName;
}

/** 目录不存在或里面什么都没有。 */
bool isEmptyOrMissing(const fs::path& directory) {
  std::error_code error;
  if (!fs::exists(directory, error)) return true;
  if (!fs::is_directory(directory, error)) return false;
  return fs::directory_iterator(directory, error) == fs::directory_iterator();
}

/** 列出目录里的全部文件与大小；skipRuntime 为真时跳过运行时才有的东西。 */
bool listFiles(const fs::path& root, bool skipRuntime, FileSizes& sizes, std::error_code& error) {
  fs::recursive_directory_iterator iterator(root, error);
  if (error) return false;
  for (const fs::recursive_directory_iterator end; iterator != end; iterator.increment(error)) {
    if (error) return false;
    const fs::path relative = iterator->path().lexically_relative(root);
    if (skipRuntime && isRuntimeEntry(relative)) {
      iterator.disable_recursion_pending();
      continue;
    }
    if (iterator->is_regular_file(error)) {
      const std::uintmax_t size = iterator->file_size(error);
      if (error) return false;
      sizes[lowerCase(relative.wstring())] = size;
    }
  }
  return !error;
}

/** 把 source 里的内容复制到 staging（不含运行时才有的东西）；source 不存在时只创建空的 staging。 */
bool copyTree(const fs::path& source, const fs::path& staging, std::error_code& error) {
  fs::create_directories(staging, error);
  if (error) return false;
  if (!fs::is_directory(source, error)) {
    error.clear();
    return true;
  }
  fs::recursive_directory_iterator iterator(source, error);
  if (error) return false;
  for (const fs::recursive_directory_iterator end; iterator != end; iterator.increment(error)) {
    if (error) return false;
    const fs::path relative = iterator->path().lexically_relative(source);
    if (isRuntimeEntry(relative)) {
      iterator.disable_recursion_pending();
      continue;
    }
    if (iterator->is_directory(error)) {
      fs::create_directories(staging / relative, error);
    } else if (iterator->is_regular_file(error)) {
      fs::copy_file(iterator->path(), staging / relative, fs::copy_options::overwrite_existing, error);
    }
    if (error) return false;
  }
  return !error;
}

/** 复制出来的内容与原处的文件清单和大小一致。 */
bool sameContent(const fs::path& source, const fs::path& staging) {
  FileSizes expected;
  FileSizes actual;
  std::error_code error;
  if (fs::is_directory(source, error) && !listFiles(source, true, expected, error)) return false;
  return listFiles(staging, false, actual, error) && expected == actual;
}

UserDataMoveResult failure(std::string message) { return UserDataMoveResult{false, std::move(message)}; }

}  // namespace

UserDataResolution resolveUserDataDirectory(const fs::path& executableDirectory, const fs::path& profileDirectory) {
  const fs::path portable = executableDirectory / kPortableDataDirectoryName;
  std::error_code error;
  if (!executableDirectory.empty() && fs::is_directory(portable, error)) return UserDataResolution{portable, shared::UserDataPlace::ExecutableDirectory};
  return UserDataResolution{profileDirectory, shared::UserDataPlace::UserProfile};
}

std::string portableDataUnavailableReason(const fs::path& executableDirectory) {
  if (canCreateFilesIn(executableDirectory)) return {};
  return "软件所在的目录不能写入（可能在需要管理员权限的位置），不能把用户数据放在这里。";
}

UserDataMoveResult moveUserData(const fs::path& executableDirectory, const fs::path& profileDirectory, shared::UserDataPlace target) {
  const bool toExecutable = target == shared::UserDataPlace::ExecutableDirectory;
  const fs::path portable = executableDirectory / kPortableDataDirectoryName;
  const fs::path retired = executableDirectory / kRetiredDirectoryName;
  const fs::path source = toExecutable ? profileDirectory : portable;
  const fs::path destination = toExecutable ? portable : profileDirectory;
  const fs::path staging = fs::path(destination.wstring() + kStagingSuffix);
  std::error_code error;

  if (executableDirectory.empty() || profileDirectory.empty()) return failure("找不到软件所在的目录或应用数据目录。");
  if (toExecutable) {
    if (fs::exists(portable, error)) return failure("软件所在的目录里已经有 ember-data 目录。");
    if (const std::string reason = portableDataUnavailableReason(executableDirectory); !reason.empty()) return failure(reason);
  } else if (!isEmptyOrMissing(destination)) {
    return failure("应用数据目录里已经有 Ember 目录，并且不是空的：" + pathToUtf8(destination) + "。请先处理它再切换。");
  }

  fs::remove_all(staging, error);
  error.clear();
  if (!copyTree(source, staging, error)) {
    const std::string reason = error.message();
    fs::remove_all(staging, error);
    return failure("复制用户数据失败：" + reason);
  }
  if (!sameContent(source, staging)) {
    fs::remove_all(staging, error);
    return failure("复制后核对发现文件与原来的不一致，已放弃。");
  }

  if (toExecutable) {
    // 临时目录改名为 ember-data 就是切换
    fs::rename(staging, portable, error);
    if (error) {
      const std::string reason = error.message();
      fs::remove_all(staging, error);
      return failure("切换失败：" + reason);
    }
    fs::remove_all(source, error);
    return UserDataMoveResult{true, {}};
  }

  // 先把软件旁的 ember-data 改名（切换），再把临时目录改成应用数据目录；后一步失败时改回去
  fs::remove_all(retired, error);
  error.clear();
  fs::rename(portable, retired, error);
  if (error) {
    const std::string reason = error.message();
    fs::remove_all(staging, error);
    return failure("切换失败：" + reason);
  }
  fs::remove(destination, error);
  error.clear();
  fs::create_directories(destination.parent_path(), error);
  error.clear();
  fs::rename(staging, destination, error);
  if (error) {
    const std::string reason = error.message();
    std::error_code ignored;
    fs::rename(retired, portable, ignored);
    fs::remove_all(staging, ignored);
    return failure("切换失败：" + reason);
  }
  fs::remove_all(retired, error);
  return UserDataMoveResult{true, {}};
}

}  // namespace ember::win32::platform
