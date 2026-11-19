/*
 * ------------------------------------------------------------------------
 * 名称：param-reader.cpp
 * 说明：请求参数读取器的实现。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-19
 * 备注：无。
 * ------------------------------------------------------------------------
 */

#include "entry/main-requests/param-reader.hpp"

#include <algorithm>

namespace ember::win32::entry {

ParamReader::ParamReader(const nlohmann::json& params, std::initializer_list<std::string_view> known) : params_(params) {
  if (!params.is_object()) throw ParamError("参数必须是对象");
  for (const auto& [key, value] : params.items()) {
    if (std::find(known.begin(), known.end(), std::string_view(key)) == known.end()) throw ParamError("不认识的字段 " + key);
  }
}

std::optional<std::int64_t> ParamReader::integer(std::string_view name, std::int64_t minimum, std::int64_t maximum) const {
  const auto found = params_.find(std::string(name));
  if (found == params_.end()) return std::nullopt;
  if (!found->is_number_integer()) throw ParamError(std::string(name) + " 必须是整数");
  const auto value = found->get<std::int64_t>();
  if (value < minimum || value > maximum) throw ParamError(std::string(name) + " 超出范围");
  return value;
}

std::int64_t ParamReader::requiredInteger(std::string_view name, std::int64_t minimum, std::int64_t maximum) const {
  const auto value = integer(name, minimum, maximum);
  if (!value) throw ParamError(std::string(name) + " 缺少");
  return *value;
}

std::string ParamReader::requiredString(std::string_view name) const {
  const auto found = params_.find(std::string(name));
  if (found == params_.end()) throw ParamError(std::string(name) + " 缺少");
  if (!found->is_string()) throw ParamError(std::string(name) + " 必须是字符串");
  std::string value = found->get<std::string>();
  if (value.empty()) throw ParamError(std::string(name) + " 不能为空");
  return value;
}

std::optional<std::string> ParamReader::enumeration(std::string_view name, std::initializer_list<std::string_view> allowed) const {
  const auto found = params_.find(std::string(name));
  if (found == params_.end()) return std::nullopt;
  if (!found->is_string()) throw ParamError(std::string(name) + " 必须是字符串");
  std::string value = found->get<std::string>();
  if (std::find(allowed.begin(), allowed.end(), std::string_view(value)) == allowed.end()) throw ParamError(std::string(name) + " 的取值不在允许的范围内");
  return value;
}

}  // namespace ember::win32::entry
