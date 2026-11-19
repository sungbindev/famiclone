/*
 * ------------------------------------------------------------------------
 * 名称：temp-directory.hpp
 * 说明：测试用的临时目录：创建一个唯一的目录，对象销毁时连同里面的内容一起删除。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-19
 * 备注：目录放在系统临时目录下，名字带进程号与序号。
 * ------------------------------------------------------------------------
 */

#pragma once

#include <windows.h>

#include <atomic>
#include <filesystem>
#include <string>
#include <system_error>

namespace ember::win32::test {

/** 测试用的临时目录。 */
class TempDirectory {
 public:
  TempDirectory() {
    static std::atomic<int> counter{0};
    path_ = std::filesystem::temp_directory_path() / ("ember-app-test-" + std::to_string(GetCurrentProcessId()) + "-" + std::to_string(counter++));
    std::filesystem::create_directories(path_);
  }
  ~TempDirectory() {
    std::error_code ignored;
    std::filesystem::remove_all(path_, ignored);
  }
  TempDirectory(const TempDirectory&) = delete;
  TempDirectory& operator=(const TempDirectory&) = delete;

  /** 目录路径。 */
  [[nodiscard]] const std::filesystem::path& path() const { return path_; }

 private:
  std::filesystem::path path_;
};

}  // namespace ember::win32::test
