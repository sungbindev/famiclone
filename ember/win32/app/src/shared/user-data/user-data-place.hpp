/*
 * ------------------------------------------------------------------------
 * 名称：user-data-place.hpp
 * 说明：用户数据放在哪里：软件所在目录旁的 ember-data 目录（便携），或当前用户的应用数据目录（%APPDATA%\Ember）。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-21
 * 备注：软件所在目录旁有 ember-data 目录时用它，没有时用应用数据目录；切换位置时命令行用 executable 或 profile 表示目标位置。
 * ------------------------------------------------------------------------
 */

#pragma once

#include <optional>
#include <string>
#include <string_view>

namespace ember::win32::shared {

/** 用户数据的位置。 */
enum class UserDataPlace {
  /** 软件所在目录旁的 ember-data 目录。 */
  ExecutableDirectory,
  /** 当前用户的应用数据目录。 */
  UserProfile
};

/** 位置在命令行里的写法。 */
[[nodiscard]] inline std::string_view userDataPlaceName(UserDataPlace place) { return place == UserDataPlace::ExecutableDirectory ? "executable" : "profile"; }

/** 从命令行里的写法读出位置；不认识的写法没有值。 */
[[nodiscard]] inline std::optional<UserDataPlace> parseUserDataPlace(std::string_view text) {
  if (text == "executable") return UserDataPlace::ExecutableDirectory;
  if (text == "profile") return UserDataPlace::UserProfile;
  return std::nullopt;
}

}  // namespace ember::win32::shared
