/*
 * ------------------------------------------------------------------------
 * 名称：param-reader.hpp
 * 说明：请求参数的读取与校验：确认参数是只含已知字段的对象，按类型与范围读出各字段，不合格时抛出 ParamError。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-19
 * 备注：规则与 Schema 的子集一致（类型、整数范围、字符串最小长度、不允许多余字段）；错误说明会作为 core.invalidParams 的说明返回给界面。
 * ------------------------------------------------------------------------
 */

#pragma once

#include <cstdint>
#include <initializer_list>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>

#include <nlohmann/json.hpp>

namespace ember::win32::entry {

/** 参数不合格。 */
class ParamError : public std::runtime_error {
 public:
  using std::runtime_error::runtime_error;
};

/** 请求参数的读取器。 */
class ParamReader {
 public:
  /**
   * @param params 请求参数。
   * @param known 已知的字段名；出现其他字段时抛出 ParamError。
   * @throws ParamError 参数不是对象或含有未知字段。
   */
  ParamReader(const nlohmann::json& params, std::initializer_list<std::string_view> known);

  /** 读整数字段；字段不存在时为空，类型或范围不合格时抛出 ParamError。 */
  [[nodiscard]] std::optional<std::int64_t> integer(std::string_view name, std::int64_t minimum, std::int64_t maximum) const;

  /** 读必填的整数字段。 */
  [[nodiscard]] std::int64_t requiredInteger(std::string_view name, std::int64_t minimum, std::int64_t maximum) const;

  /** 读必填的字符串字段，不能为空。 */
  [[nodiscard]] std::string requiredString(std::string_view name) const;

  /** 读字符串字段，只能取 allowed 里的值；字段不存在时为空。 */
  [[nodiscard]] std::optional<std::string> enumeration(std::string_view name, std::initializer_list<std::string_view> allowed) const;

 private:
  const nlohmann::json& params_;
};

}  // namespace ember::win32::entry
