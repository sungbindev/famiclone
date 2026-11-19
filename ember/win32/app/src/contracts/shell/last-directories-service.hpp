/*
 * ------------------------------------------------------------------------
 * 名称：last-directories-service.hpp
 * 说明：记住上次目录的服务抽象：按用途记下文件选择窗口上次选定的目录。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-19
 * 备注：对应 Electron 外壳的 src/contracts/shell/last-directories-service.ts；保存在 config/last-directories.json，下次同一用途的窗口从那里打开。
 * ------------------------------------------------------------------------
 */

#pragma once

#include <optional>
#include <string>

namespace ember::win32::contracts {

/** 记住上次目录的服务。 */
class LastDirectoriesService {
 public:
  virtual ~LastDirectoriesService() = default;

  /** 该用途上次选定的目录；没记过时为空。 */
  [[nodiscard]] virtual std::optional<std::string> get(const std::string& purpose) const = 0;

  /** 记下该用途选定的目录；用途或目录不合格时忽略。 */
  virtual void set(const std::string& purpose, const std::string& directory) = 0;

  /** 立即写盘。 */
  virtual void flush() = 0;
};

}  // namespace ember::win32::contracts
