/*
 * ------------------------------------------------------------------------
 * 名称：shell-state-service.hpp
 * 说明：界面状态服务的抽象：读出与修改要保存的界面状态（窗口位置、窗口是否始终置顶、选择的机型、面板自动收缩的开关、上次显示更新记录的版本）。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-19
 * 备注：规则与格式见 shared/shell-state 与 docs/05-界面.md 第 9 节。
 * ------------------------------------------------------------------------
 */

#pragma once

#include <optional>
#include <string>

#include "shared/shell-state/shell-state.hpp"

namespace ember::win32::contracts {

/** 界面状态服务。 */
class ShellStateService {
 public:
  virtual ~ShellStateService() = default;

  /** 当前的界面状态。 */
  [[nodiscard]] virtual const shared::ShellState& get() const = 0;

  /** 替换布局里保存的选择。 */
  virtual void updateLayout(const shared::ShellLayout& layout) = 0;

  /** 记下窗口位置。 */
  virtual void updateWindow(const shared::WindowPlacement& placement) = 0;

  /** 记下上次显示更新记录时的应用版本。 */
  virtual void updateReleaseNotesVersion(const std::string& version) = 0;

  /** 记下磁盘镜像管理的镜像目录；没有值表示取消。 */
  virtual void updateDiskImagesRoot(const std::optional<std::string>& path) = 0;

  /** 立即写盘。 */
  virtual void flush() = 0;
};

}  // namespace ember::win32::contracts
