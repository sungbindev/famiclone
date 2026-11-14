/*
 * ------------------------------------------------------------------------
 * 名称：machines-error.hpp
 * 说明：machines 模块的错误码：机型不存在、不需要或尚未支持 BIOS、BIOS 文件读取与校验失败、没有添加 BIOS，经 shared::CoreError 抛出，说明面向用户。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-15
 * 备注：错误码以模块名 machines 开头。
 * ------------------------------------------------------------------------
 */

#pragma once

#include <string_view>

namespace ember::contracts::machines {

/** 没有这个机型。 */
inline constexpr std::string_view ERROR_NOT_FOUND = "machines.notFound";
/** 这个机型不需要 BIOS（fc）。 */
inline constexpr std::string_view ERROR_BIOS_NOT_NEEDED = "machines.biosNotNeeded";
/** 这个机型程序里还没有实现。 */
inline constexpr std::string_view ERROR_UNSUPPORTED = "machines.unsupported";
/** BIOS 源文件不存在。 */
inline constexpr std::string_view ERROR_FILE_NOT_FOUND = "machines.fileNotFound";
/** BIOS 源文件无法读取：不是普通文件、没有权限或读取中途失败。 */
inline constexpr std::string_view ERROR_FILE_UNREADABLE = "machines.fileUnreadable";
/** BIOS 文件的格式或大小不符合这个机型。 */
inline constexpr std::string_view ERROR_BIOS_INVALID = "machines.biosInvalid";
/** 无法读写、删除 BIOS 目录里的文件。 */
inline constexpr std::string_view ERROR_BIOS_ACCESS_FAILED = "machines.biosAccessFailed";
/** 这个机型开机必须有卡带（fc），改用 cartridges.load。 */
inline constexpr std::string_view ERROR_CARTRIDGE_REQUIRED = "machines.cartridgeRequired";
/** 这个机型还没有添加 BIOS。 */
inline constexpr std::string_view ERROR_NO_BIOS = "machines.noBios";

}  // namespace ember::contracts::machines
