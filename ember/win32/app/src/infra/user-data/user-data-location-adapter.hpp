/*
 * ------------------------------------------------------------------------
 * 名称：user-data-location-adapter.hpp
 * 说明：用户数据位置的实现：当前目录由启动时确定；能不能换到软件所在目录由目录能否写入决定；启动移动就是启动软件自己的新进程，带上目标位置和本进程号。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-21
 * 备注：新进程先等本进程退出，再复制、核对、切换，最后重新启动软件，见 platform/files/user-data-location.hpp。
 * ------------------------------------------------------------------------
 */

#pragma once

#include <filesystem>

#include "contracts/system/user-data-location.hpp"
#include "platform/files/user-data-location.hpp"

namespace ember::win32::infra {

/** 用户数据位置。 */
class UserDataLocationAdapter final : public contracts::UserDataLocation {
 public:
  /**
   * @param resolution 启动时确定的用户数据目录与位置。
   * @param overridden 目录是否由启动参数 --user-data-dir 指定。
   * @param executable 软件自己的完整路径。
   * @param profileDirectory 应用数据目录下的 Ember 目录。
   */
  UserDataLocationAdapter(platform::UserDataResolution resolution, bool overridden, std::filesystem::path executable, std::filesystem::path profileDirectory);

  [[nodiscard]] contracts::UserDataState state() const override;
  [[nodiscard]] std::string directoryOf(shared::UserDataPlace place) const override;
  [[nodiscard]] std::string unavailableReason(shared::UserDataPlace target) const override;
  [[nodiscard]] std::string startMove(shared::UserDataPlace target) override;

 private:
  platform::UserDataResolution resolution_;
  bool overridden_;
  std::filesystem::path executable_;
  std::filesystem::path profileDirectory_;
};

}  // namespace ember::win32::infra
