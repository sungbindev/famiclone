/*
 * ------------------------------------------------------------------------
 * 名称：user-data-location.hpp
 * 说明：用户数据位置的端口：当前用户数据放在哪里、能不能换到另一处、启动移动（由新进程在本进程退出后完成复制与切换）。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-21
 * 备注：具体实现在基础设施层；本进程只负责启动新进程，复制与切换由新进程做，因为本进程还占用着数据文件。
 * ------------------------------------------------------------------------
 */

#pragma once

#include <string>

#include "shared/user-data/user-data-place.hpp"

namespace ember::win32::contracts {

/** 当前用户数据的位置。 */
struct UserDataState {
  /** 当前使用的用户数据目录。 */
  std::string directory;
  shared::UserDataPlace place{shared::UserDataPlace::UserProfile};
  /** 用启动参数 --user-data-dir 指定了目录：这时不能切换位置。 */
  bool overridden{false};
};

/** 用户数据位置。 */
class UserDataLocation {
 public:
  virtual ~UserDataLocation() = default;

  [[nodiscard]] virtual UserDataState state() const = 0;
  /** 某个位置的目录（还没有创建也给出路径）。 */
  [[nodiscard]] virtual std::string directoryOf(shared::UserDataPlace place) const = 0;
  /** 不能移到这个位置的原因（中文）；能移时为空文字。 */
  [[nodiscard]] virtual std::string unavailableReason(shared::UserDataPlace target) const = 0;
  /**
   * 启动移动：启动一个新的进程，它等本进程退出后复制并切换，再重新启动软件。
   * @returns 启动成功时为空文字，失败时是错误说明。
   */
  [[nodiscard]] virtual std::string startMove(shared::UserDataPlace target) = 0;
};

}  // namespace ember::win32::contracts
