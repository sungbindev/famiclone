/*
 * ------------------------------------------------------------------------
 * 名称：inspector-error.hpp
 * 说明：inspector 契约：错误码，取值与 schema/inspector.schema.json 登记的一致。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-14
 * ------------------------------------------------------------------------
 */

#pragma once

#include <string_view>

namespace ember::contracts::inspector {

/** 还没有装入卡带。 */
inline constexpr std::string_view ERROR_NO_CARTRIDGE = "inspector.noCartridge";
/** 主机没有通电。 */
inline constexpr std::string_view ERROR_NOT_POWERED = "inspector.notPowered";
/** 当前主机型号不支持调试。 */
inline constexpr std::string_view ERROR_UNSUPPORTED = "inspector.unsupported";
/** 没有这个内存空间。 */
inline constexpr std::string_view ERROR_UNKNOWN_SPACE = "inspector.unknownSpace";
/** 访问范围超出内存空间的大小。 */
inline constexpr std::string_view ERROR_OUT_OF_RANGE = "inspector.outOfRange";
/** 目标是只读的。 */
inline constexpr std::string_view ERROR_READ_ONLY = "inspector.readOnly";
/** 状态模块、状态项不存在，或取值超出范围。 */
inline constexpr std::string_view ERROR_INVALID_STATE = "inspector.invalidState";
/** 断点的地址范围不合格。 */
inline constexpr std::string_view ERROR_INVALID_BREAKPOINT = "inspector.invalidBreakpoint";
/** 断点数量超过上限。 */
inline constexpr std::string_view ERROR_BREAKPOINT_LIMIT = "inspector.breakpointLimit";
/** 没有这个编号的断点。 */
inline constexpr std::string_view ERROR_NO_BREAKPOINT = "inspector.noBreakpoint";

}  // namespace ember::contracts::inspector
