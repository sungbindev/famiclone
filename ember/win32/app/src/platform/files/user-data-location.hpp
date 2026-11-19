/*
 * ------------------------------------------------------------------------
 * 名称：user-data-location.hpp
 * 说明：用户数据目录的位置规则与移动：软件所在目录旁有 ember-data 目录时用它，否则用当前用户的应用数据目录；把整份数据从一处复制到另一处，核对无误后再切换并删除原处。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-21
 * 备注：移动时不复制运行时才有的东西：根目录下的 lockfile（单实例锁）和 run 目录（调试服务的端点文件）；
 *       先复制到目标旁的临时目录（名字是目标加 .tmp），核对文件清单与大小一致，再用改名一步切换：
 *       移到软件所在目录时，临时目录改名为 ember-data 就是切换；移到应用数据目录时，把软件旁的 ember-data 改名为 ember-data.old 就是切换；
 *       切换之前出任何错都只删临时目录，原数据保持原样；切换之后删原处，删不干净不算失败。
 * ------------------------------------------------------------------------
 */

#pragma once

#include <filesystem>
#include <string>

#include "shared/user-data/user-data-place.hpp"

namespace ember::win32::platform {

/** 软件所在目录旁存放便携数据的目录名。 */
inline constexpr const wchar_t* kPortableDataDirectoryName = L"ember-data";

/** 解析出的用户数据目录。 */
struct UserDataResolution {
  std::filesystem::path directory;
  shared::UserDataPlace place{shared::UserDataPlace::UserProfile};
};

/** 一次移动的结果。 */
struct UserDataMoveResult {
  bool ok{false};
  /** 失败的原因（中文），成功时为空。 */
  std::string message;
};

/**
 * 解析用户数据目录：软件所在目录旁有 ember-data 目录时用它，否则用应用数据目录。
 * @param executableDirectory 软件所在的目录。
 * @param profileDirectory 应用数据目录下的 Ember 目录。
 */
[[nodiscard]] UserDataResolution resolveUserDataDirectory(const std::filesystem::path& executableDirectory, const std::filesystem::path& profileDirectory);

/**
 * 能不能把用户数据放到软件所在目录：目录要能创建文件（装在 Program Files 之类需要管理员权限的位置时不能）。
 * @returns 能时为空文字，不能时是原因（中文）。
 */
[[nodiscard]] std::string portableDataUnavailableReason(const std::filesystem::path& executableDirectory);

/**
 * 把用户数据移到另一处。调用时当前的软件进程必须已经退出（数据文件不能被占用）。
 * @param executableDirectory 软件所在的目录。
 * @param profileDirectory 应用数据目录下的 Ember 目录。
 * @param target 要移到哪里。
 */
[[nodiscard]] UserDataMoveResult moveUserData(const std::filesystem::path& executableDirectory, const std::filesystem::path& profileDirectory, shared::UserDataPlace target);

}  // namespace ember::win32::platform
