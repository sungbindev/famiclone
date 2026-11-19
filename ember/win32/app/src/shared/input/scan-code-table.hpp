/*
 * ------------------------------------------------------------------------
 * 名称：scan-code-table.hpp
 * 说明：Windows 原始输入的扫描码（第 1 套扫描码）到 KeyboardEvent.code 键位名的对照。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-19
 * 备注：扫描码到键位名的对照表；
 *       键位名与学习机键盘模块使用的名字一致（物理键位，与键盘布局无关）；数字键盘区关闭 NumLock 时系统会额外发出“假 Shift”（E0 2A、E0 36），
 *       它们不是真实按键，对照表里不收录；Pause 键由 E1 前缀的序列表示，不收录，其余没有收录的扫描码返回空值，由调用方忽略。
 * ------------------------------------------------------------------------
 */

#pragma once

#include <cstdint>
#include <optional>
#include <string_view>

namespace ember::win32::shared {

/**
 * 扫描码转键位名。
 * @param make 扫描码。
 * @param extended 是否带 E0 前缀。
 * @param prefixE1 是否带 E1 前缀（Pause 键）；带 E1 的一律忽略。
 * @returns 键位名；假 Shift、Pause 与未收录的扫描码没有值。
 */
[[nodiscard]] std::optional<std::string_view> scanCodeToKeyCode(std::uint16_t make, bool extended, bool prefixE1);

}  // namespace ember::win32::shared
