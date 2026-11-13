/*
 * ------------------------------------------------------------------------
 * 名称：hardware-error.hpp
 * 说明：硬件模块的错误码：硬件契约与实现共用，经 shared::CoreError 抛出。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-14
 * 备注：错误码以模块名 hardware 开头；这些错误都是装配或调用方的错误，不是运行中的正常情况。
 * ------------------------------------------------------------------------
 */

#pragma once

#include <string_view>

namespace ember::contracts::hardware {

/** 总线地址范围不合法：起点大于终点，或超出总线的地址宽度。 */
inline constexpr std::string_view ERROR_BUS_RANGE_INVALID = "hardware.busRangeInvalid";
/** 总线地址范围与已挂载的范围重叠。 */
inline constexpr std::string_view ERROR_BUS_RANGE_CONFLICT = "hardware.busRangeConflict";
/** 时钟被要求向后推进，或在事件回调里被推进。 */
inline constexpr std::string_view ERROR_CLOCK_INVALID_ADVANCE = "hardware.clockInvalidAdvance";
/** 信号线的来源数量超过上限。 */
inline constexpr std::string_view ERROR_LINE_SOURCE_LIMIT = "hardware.lineSourceLimit";
/** CPU 的 RDY 线长时间有效，暂停方没有释放。 */
inline constexpr std::string_view ERROR_CPU_STALLED = "hardware.cpuStalled";
/** 构造参数不合法（如分频比不大于 0）。 */
inline constexpr std::string_view ERROR_INVALID_ARGUMENT = "hardware.invalidArgument";
/** 存储区的偏移或读取范围超出容量。 */
inline constexpr std::string_view ERROR_STORAGE_OUT_OF_RANGE = "hardware.storageOutOfRange";
/** 端口编号超出主机上的端口数。 */
inline constexpr std::string_view ERROR_PORT_NOT_FOUND = "hardware.portNotFound";

}  // namespace ember::contracts::hardware
