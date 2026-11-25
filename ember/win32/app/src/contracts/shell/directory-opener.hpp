/*
 * ------------------------------------------------------------------------
 * 名称：directory-opener.hpp
 * 说明：用系统文件管理器打开指定目录的抽象：目录由外壳从核心取得，界面不能指定。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-22
 * 备注：与 OpenableDirectory 的区别是目录不固定（如打印机图片目录由首选项决定）。
 * ------------------------------------------------------------------------
 */

#pragma once

#include <string>

namespace ember::win32::contracts {

/** 可以用系统的文件管理器打开任意目录的服务。 */
class DirectoryOpener {
 public:
  virtual ~DirectoryOpener() = default;

  /**
   * 打开目录；目录不存在时先创建。
   * @param path UTF-8 路径。
   * @returns 成功时为空文字，失败时是错误说明。
   */
  [[nodiscard]] virtual std::string open(const std::string& path) = 0;

  /**
   * 在文件管理器里显示一个文件（打开它所在的目录并选中它）。
   * @param path UTF-8 的文件完整路径。
   * @returns 成功时为空文字，失败时是错误说明。
   */
  [[nodiscard]] virtual std::string reveal(const std::string& path) = 0;

  /**
   * 用系统默认的程序打开一个文件（如用图片查看程序打开 PNG）。
   * @param path UTF-8 的文件完整路径。
   * @returns 成功时为空文字，失败时是错误说明。
   */
  [[nodiscard]] virtual std::string openFile(const std::string& path) = 0;
};

}  // namespace ember::win32::contracts
