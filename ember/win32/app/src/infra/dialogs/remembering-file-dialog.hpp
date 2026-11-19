/*
 * ------------------------------------------------------------------------
 * 名称：remembering-file-dialog.hpp
 * 说明：带目录记忆的文件选择窗口：用系统的文件选择窗口，同一用途的窗口从上次选定的目录打开，选定后记下新的目录。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-19
 * 备注：文件记它所在的目录，文件夹记它自己；记下的目录不存在时系统退回默认位置。
 * ------------------------------------------------------------------------
 */

#pragma once

#include "contracts/shell/file-dialog.hpp"
#include "contracts/shell/last-directories-service.hpp"
#include "platform/dialogs/system-dialogs.hpp"

namespace ember::win32::infra {

/** 带目录记忆的文件选择窗口。 */
class RememberingFileDialog : public contracts::FileDialog {
 public:
  RememberingFileDialog(const platform::SystemDialogs& dialogs, contracts::LastDirectoriesService& memory);

  [[nodiscard]] std::optional<std::string> pickFile(const contracts::FileDialogOptions& options) override;
  [[nodiscard]] std::optional<std::string> pickDirectory(const contracts::DirectoryDialogOptions& options) override;
  [[nodiscard]] std::optional<std::string> pickSaveFile(const contracts::FileDialogOptions& options) override;

 private:
  const platform::SystemDialogs& dialogs_;
  contracts::LastDirectoriesService& memory_;
};

}  // namespace ember::win32::infra
