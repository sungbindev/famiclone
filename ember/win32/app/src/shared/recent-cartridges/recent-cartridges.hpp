/*
 * ------------------------------------------------------------------------
 * 名称：recent-cartridges.hpp
 * 说明：最近打开的卡带清单的规则：最多保留的个数、从保存的内容里读出清单、加入与移出、显示名、转成要保存的内容。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-19
 * 备注：文件格式见 config/recent-cartridges.json；
 *       清单只存卡带文件的路径（UTF-8），由外壳自己使用，界面只能拿到显示名。
 * ------------------------------------------------------------------------
 */

#pragma once

#include <string>
#include <vector>

#include <nlohmann/json.hpp>

namespace ember::win32::shared {

/** 最近打开的卡带最多保留的个数。 */
inline constexpr std::size_t kMaxRecentCartridges = 10;

/**
 * 从保存的内容里读出清单：只接受结构合格的项，去掉重复，超出个数的丢弃；内容缺失或损坏时得到空清单。
 * @param raw 读到的内容。
 * @returns 路径清单，最近的在前。
 */
[[nodiscard]] std::vector<std::string> parseRecentCartridges(const nlohmann::json& raw);

/**
 * 把一个卡带加到清单最前面；已在清单里的移到最前，超出个数的丢弃最早的。
 * @returns 新清单；不是 .nes 路径时原样返回。
 */
[[nodiscard]] std::vector<std::string> addRecentCartridge(const std::vector<std::string>& list, const std::string& path);

/** 把一个卡带移出清单；不在清单里时原样返回。 */
[[nodiscard]] std::vector<std::string> removeRecentCartridge(const std::vector<std::string>& list, const std::string& path);

/** 路径里的文件名，作为界面上的显示名。 */
[[nodiscard]] std::string recentCartridgeName(const std::string& path);

/** 转成要保存的内容：{"version":1,"items":[{"path":…}]}。 */
[[nodiscard]] nlohmann::json toRecentCartridgesFile(const std::vector<std::string>& list);

}  // namespace ember::win32::shared
