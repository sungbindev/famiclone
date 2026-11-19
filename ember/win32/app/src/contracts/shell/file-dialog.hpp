/*
 * ------------------------------------------------------------------------
 * 名称：file-dialog.hpp
 * 说明：文件选择窗口的抽象：外壳经它请用户选一个文件、文件夹，或选一个要保存的文件；实现用系统的文件选择窗口。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-19
 * 备注：对应 Electron 外壳的 src/contracts/shell/file-dialog.ts；路径只来自用户在系统窗口里的选择，界面不能指定路径；
 *       每次调用用 purpose 标明用途，相同用途的窗口从上次选定的目录打开（记忆见 last-directories-service.hpp）；用户取消时返回空值。
 * ------------------------------------------------------------------------
 */

#pragma once

#include <optional>
#include <string>
#include <vector>

namespace ember::win32::contracts {

/** 文件类型过滤。 */
struct FileDialogFilter {
  /** 显示名称。 */
  std::string name;
  /** 扩展名，不带点，如 nes；"*" 表示所有文件。 */
  std::vector<std::string> extensions;
};

/** 选择文件或保存文件的参数。 */
struct FileDialogOptions {
  /** 窗口标题。 */
  std::string title;
  /** 文件类型过滤。 */
  std::vector<FileDialogFilter> filters;
  /** 用途标识（字母开头的字母与数字），相同用途的窗口记住上次打开的目录。 */
  std::string purpose;
};

/** 选择文件夹的参数。 */
struct DirectoryDialogOptions {
  /** 窗口标题。 */
  std::string title;
  /** 用途标识，同上。 */
  std::string purpose;
};

/** 文件选择窗口。 */
class FileDialog {
 public:
  virtual ~FileDialog() = default;

  /** 请用户选一个文件；用户取消时为空。 */
  [[nodiscard]] virtual std::optional<std::string> pickFile(const FileDialogOptions& options) = 0;

  /** 请用户选一个文件夹；用户取消时为空。 */
  [[nodiscard]] virtual std::optional<std::string> pickDirectory(const DirectoryDialogOptions& options) = 0;

  /** 请用户选一个要保存的文件；用户取消时为空。 */
  [[nodiscard]] virtual std::optional<std::string> pickSaveFile(const FileDialogOptions& options) = 0;
};

}  // namespace ember::win32::contracts
