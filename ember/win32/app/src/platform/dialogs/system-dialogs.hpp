/*
 * ------------------------------------------------------------------------
 * 名称：system-dialogs.hpp
 * 说明：系统的文件选择窗口：打开单个文件、选择文件夹、保存文件；用 Windows 的 IFileOpenDialog 与 IFileSaveDialog（Windows 7 可用）。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-19
 * 备注：必须在界面线程调用，窗口显示期间阻塞（系统自己的模态消息循环）；有父窗口时以它为父窗口，没有时单独弹出；
 *       路径与文字都是 UTF-8；用户取消时返回空值；起始目录不存在时系统退回默认位置；扩展名写 "*" 表示所有文件。
 * ------------------------------------------------------------------------
 */

#pragma once

#include <functional>
#include <optional>
#include <string>
#include <vector>

namespace ember::win32::platform {

/** 文件类型过滤。 */
struct DialogFilter {
  /** 显示名称。 */
  std::string name;
  /** 扩展名，不带点，如 nes；"*" 表示所有文件。 */
  std::vector<std::string> extensions;
};

/** 选择文件或保存文件的参数。 */
struct FileDialogRequest {
  std::string title;
  std::vector<DialogFilter> filters;
  /** 窗口打开时所在的目录。 */
  std::optional<std::string> startDirectory;
};

/** 选择文件夹的参数。 */
struct DirectoryDialogRequest {
  std::string title;
  std::optional<std::string> startDirectory;
};

/** 系统的文件选择窗口。 */
class SystemDialogs {
 public:
  /** 取父窗口的函数：返回窗口句柄（HWND），没有窗口时返回空。 */
  using ParentProvider = std::function<void*()>;

  explicit SystemDialogs(ParentProvider parent = {});

  /** 请用户选一个文件；用户取消时为空。 */
  [[nodiscard]] std::optional<std::string> pickFile(const FileDialogRequest& request) const;

  /** 请用户选一个文件夹；用户取消时为空。 */
  [[nodiscard]] std::optional<std::string> pickDirectory(const DirectoryDialogRequest& request) const;

  /** 请用户选一个要保存的文件（文件可以还不存在，已存在时系统窗口会询问是否覆盖）；用户取消时为空。 */
  [[nodiscard]] std::optional<std::string> pickSaveFile(const FileDialogRequest& request) const;

 private:
  ParentProvider parent_;
};

}  // namespace ember::win32::platform
