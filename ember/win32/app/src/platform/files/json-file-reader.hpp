/*
 * ------------------------------------------------------------------------
 * 名称：json-file-reader.hpp
 * 说明：一次性读取本地 JSON 文件：文件不存在、读不了或不是合法 JSON 时得到空值，不抛异常。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-19
 * 备注：用于界面线程和核心都还没起来的时候读配置（如首选项里的“允许同时运行多个实例”）；需要写回的文件用 JsonFileStore。
 * ------------------------------------------------------------------------
 */

#pragma once

#include <filesystem>
#include <optional>

#include <nlohmann/json.hpp>

namespace ember::win32::platform {

/** 读取 JSON 文件；读不了或内容不合法时为空。 */
[[nodiscard]] std::optional<nlohmann::json> readJsonFile(const std::filesystem::path& path);

}  // namespace ember::win32::platform
