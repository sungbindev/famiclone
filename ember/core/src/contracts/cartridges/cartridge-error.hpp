/*
 * ------------------------------------------------------------------------
 * 名称：cartridge-error.hpp
 * 说明：cartridges 模块的错误码：NES 文件解析失败时经 shared::CoreError 抛出，说明面向用户。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-14
 * 备注：错误码以模块名 cartridges 开头。
 * ------------------------------------------------------------------------
 */

#pragma once

#include <string_view>

namespace ember::contracts::cartridges {

/** 文件不是 iNES 或 NES 2.0 格式：开头的魔数不对。 */
inline constexpr std::string_view ERROR_NOT_INES = "cartridges.notInes";
/** 文件被截断：比文件头声明的内容短。 */
inline constexpr std::string_view ERROR_TRUNCATED = "cartridges.truncated";
/** 不支持的格式：UNIF、FDS，或文件头里无法处理的取值。 */
inline constexpr std::string_view ERROR_UNSUPPORTED_FORMAT = "cartridges.unsupportedFormat";
/** 文件头不合法：如 PRG 大小为 0。 */
inline constexpr std::string_view ERROR_INVALID_HEADER = "cartridges.invalidHeader";

}  // namespace ember::contracts::cartridges
