/*
 * ------------------------------------------------------------------------
 * 名称：environment.hpp
 * 说明：读取环境变量，只供开发期的诊断开关使用。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-19
 * 备注：变量名与值都是 UTF-8；变量不存在或值为空时没有值。
 * ------------------------------------------------------------------------
 */

#pragma once

#include <optional>
#include <string>
#include <string_view>

namespace ember::win32::platform {

/**
 * 读取环境变量。
 * @param name 变量名。
 * @returns 变量的值；变量不存在或值为空时没有值。
 */
[[nodiscard]] std::optional<std::string> environmentVariable(std::string_view name);

}  // namespace ember::win32::platform
