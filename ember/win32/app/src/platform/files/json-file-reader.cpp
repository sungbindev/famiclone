/*
 * ------------------------------------------------------------------------
 * 名称：json-file-reader.cpp
 * 说明：一次性读取本地 JSON 文件的实现。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-19
 * 备注：无。
 * ------------------------------------------------------------------------
 */

#include "platform/files/json-file-reader.hpp"

#include <fstream>
#include <iterator>
#include <string>

namespace ember::win32::platform {

std::optional<nlohmann::json> readJsonFile(const std::filesystem::path& path) {
  std::ifstream file(path, std::ios::binary);
  if (!file) return std::nullopt;
  const std::string text((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
  nlohmann::json value = nlohmann::json::parse(text, nullptr, false);
  if (value.is_discarded()) return std::nullopt;
  return value;
}

}  // namespace ember::win32::platform
