/*
 * ------------------------------------------------------------------------
 * 名称：user-data-controller.hpp
 * 说明：“应用”分组里“用户数据目录”一行的控制器：给出当前目录与切换按钮的状态；打开目录；切换位置时先询问，同意后启动移动并退出软件。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-21
 * 备注：移动本身由新进程在本进程退出后完成（本进程还占着数据文件），见 docs/05-界面.md 第 4.5 节；错误用外壳通知给出。
 * ------------------------------------------------------------------------
 */

#pragma once

#include <functional>

#include "contracts/preferences/preferences-model.hpp"
#include "contracts/shell/confirmation.hpp"
#include "contracts/shell/openable-directory.hpp"
#include "contracts/shell/shell-registry.hpp"
#include "contracts/system/user-data-location.hpp"

namespace ember::win32::logic {

/** 用户数据目录一行的控制器。 */
class UserDataController final : public contracts::UserDataModel {
 public:
  /**
   * @param location 用户数据位置。
   * @param directory 当前用户数据目录（用文件管理器打开）。
   * @param confirmation 确认询问。
   */
  UserDataController(contracts::UserDataLocation& location, contracts::OpenableDirectory& directory, contracts::Confirmation& confirmation);

  /**
   * 接上外壳：通知与退出软件都经它。
   * @returns 断开的函数。
   */
  std::function<void()> attach(contracts::ShellApi& shell);

  // ---- UserDataModel ----
  [[nodiscard]] contracts::UserDataView view() const override;
  void openDirectory() override;
  void switchPlace() override;

 private:
  /** 切换的目标位置：与当前相反的一处。 */
  [[nodiscard]] shared::UserDataPlace targetPlace() const;

  contracts::UserDataLocation& location_;
  contracts::OpenableDirectory& directory_;
  contracts::Confirmation& confirmation_;
  contracts::ShellApi* shell_{nullptr};
};

}  // namespace ember::win32::logic
