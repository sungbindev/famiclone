/*
 * ------------------------------------------------------------------------
 * 名称：file-shell-state-service.hpp
 * 说明：界面状态服务的实现：启动时从本地文件读出并逐项校验（文件不存在或损坏时得到默认值，不报错弹窗），修改后交给文件存储防抖写盘。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-19
 * 备注：文件存储由组装根注入；文件是用户数据目录下的 config/shell-state.json，格式与 Electron 外壳相同。
 * ------------------------------------------------------------------------
 */

#pragma once

#include "contracts/shell/shell-state-service.hpp"
#include "platform/files/json-file-store.hpp"

namespace ember::win32::infra {

/** 保存在本地文件里的界面状态服务。 */
class FileShellStateService : public contracts::ShellStateService {
 public:
  explicit FileShellStateService(platform::JsonFileStore& store);

  [[nodiscard]] const shared::ShellState& get() const override { return state_; }
  void updateLayout(const shared::ShellLayout& layout) override;
  void updateWindow(const shared::WindowPlacement& placement) override;
  void updateReleaseNotesVersion(const std::string& version) override;
  void flush() override { store_.flush(); }

 private:
  void replace(shared::ShellState next);

  platform::JsonFileStore& store_;
  shared::ShellState state_;
};

}  // namespace ember::win32::infra
