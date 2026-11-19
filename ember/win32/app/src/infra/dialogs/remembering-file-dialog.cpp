/*
 * ------------------------------------------------------------------------
 * 名称：remembering-file-dialog.cpp
 * 说明：带目录记忆的文件选择窗口的实现。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-19
 * 备注：无。
 * ------------------------------------------------------------------------
 */

#include "infra/dialogs/remembering-file-dialog.hpp"

#include "platform/text/wide-text.hpp"

namespace ember::win32::infra {

namespace {

/** 路径所在的目录（UTF-8）；盘符根目录保留结尾的反斜杠。 */
std::string parentDirectory(const std::string& path) { return platform::pathToUtf8(platform::pathFromUtf8(path).parent_path()); }

platform::FileDialogRequest toRequest(const contracts::FileDialogOptions& options, const std::optional<std::string>& start) {
  platform::FileDialogRequest request{options.title, {}, start};
  for (const contracts::FileDialogFilter& filter : options.filters) request.filters.push_back({filter.name, filter.extensions});
  return request;
}

}  // namespace

RememberingFileDialog::RememberingFileDialog(const platform::SystemDialogs& dialogs, contracts::LastDirectoriesService& memory) : dialogs_(dialogs), memory_(memory) {}

std::optional<std::string> RememberingFileDialog::pickFile(const contracts::FileDialogOptions& options) {
  const auto path = dialogs_.pickFile(toRequest(options, memory_.get(options.purpose)));
  if (path) memory_.set(options.purpose, parentDirectory(*path));
  return path;
}

std::optional<std::string> RememberingFileDialog::pickDirectory(const contracts::DirectoryDialogOptions& options) {
  const auto path = dialogs_.pickDirectory(platform::DirectoryDialogRequest{options.title, memory_.get(options.purpose)});
  if (path) memory_.set(options.purpose, *path);
  return path;
}

std::optional<std::string> RememberingFileDialog::pickSaveFile(const contracts::FileDialogOptions& options) {
  const auto path = dialogs_.pickSaveFile(toRequest(options, memory_.get(options.purpose)));
  if (path) memory_.set(options.purpose, parentDirectory(*path));
  return path;
}

}  // namespace ember::win32::infra
