/*
 * ------------------------------------------------------------------------
 * 名称：last-directories.hpp
 * 说明：记住上次目录的规则：用途标识与目录的合格条件、个数上限、从保存的内容里读出、转成要保存的内容。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-19
 * 备注：文件格式（config/last-directories.json）：{ "version": 1, "directories": { 用途: 目录 } }；
 *       用途不合格或目录为空、过长的项读出时跳过。
 * ------------------------------------------------------------------------
 */

#pragma once

#include <map>
#include <string>

#include <nlohmann/json.hpp>

namespace ember::win32::shared {

/** 最多记住的用途个数。 */
inline constexpr std::size_t kMaxLastDirectoryPurposes = 64;

/** 用途标识合格：字母开头，字母与数字，最长 64。 */
[[nodiscard]] bool isValidDirectoryPurpose(const std::string& purpose);

/** 目录合格：非空，不超过 1024 个字节。 */
[[nodiscard]] bool isValidRememberedDirectory(const std::string& directory);

/** 从保存的内容里取出合格的项（用途到目录）；内容缺失或损坏时为空。 */
[[nodiscard]] std::map<std::string, std::string> parseLastDirectories(const nlohmann::json& raw);

/** 转成要保存的内容。 */
[[nodiscard]] nlohmann::json toLastDirectoriesFile(const std::map<std::string, std::string>& directories);

}  // namespace ember::win32::shared
