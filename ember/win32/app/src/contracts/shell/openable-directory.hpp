/*
 * ------------------------------------------------------------------------
 * 名称：openable-directory.hpp
 * 说明：可以用系统文件管理器打开的目录的抽象（日志目录、ROM 目录）：目录的位置由外壳决定，界面不能指定。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-19
 * 备注：目录不存在时先创建再打开。
 * ------------------------------------------------------------------------
 */

#pragma once

#include <string>

namespace ember::win32::contracts {

/** 可以用系统的文件管理器打开的目录。 */
class OpenableDirectory {
 public:
  virtual ~OpenableDirectory() = default;

  /**
   * 打开目录。
   * @returns 成功时为空文字，失败时是错误说明。
   */
  [[nodiscard]] virtual std::string open() = 0;
};

}  // namespace ember::win32::contracts
