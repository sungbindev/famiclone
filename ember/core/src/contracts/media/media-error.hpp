/*
 * ------------------------------------------------------------------------
 * 名称：media-error.hpp
 * 说明：media 模块的错误码：没有主机或驱动器、盘片镜像文件读写失败、没有盘片、故障注入的参数不全，经 shared::CoreError 抛出，说明面向用户。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-15
 * 备注：错误码以模块名 media 开头。
 * ------------------------------------------------------------------------
 */

#pragma once

#include <string_view>

namespace ember::contracts::media {

/** 还没有主机：先打开卡带或用整机机型开机。 */
inline constexpr std::string_view ERROR_NO_MACHINE = "media.noMachine";
/** 主机没有这个驱动器。 */
inline constexpr std::string_view ERROR_NO_DRIVE = "media.noDrive";
/** 镜像文件不存在。 */
inline constexpr std::string_view ERROR_FILE_NOT_FOUND = "media.fileNotFound";
/** 镜像文件无法读取：不是普通文件、没有权限或读取中途失败。 */
inline constexpr std::string_view ERROR_FILE_UNREADABLE = "media.fileUnreadable";
/** 镜像文件不合格：空文件或超过大小上限。 */
inline constexpr std::string_view ERROR_IMAGE_INVALID = "media.imageInvalid";
/** 驱动器里没有盘片。 */
inline constexpr std::string_view ERROR_NO_DISK = "media.noDisk";
/** 镜像写回文件失败。 */
inline constexpr std::string_view ERROR_SAVE_FAILED = "media.saveFailed";
/** 指定扇区校验错的故障必须给出扇区号。 */
inline constexpr std::string_view ERROR_SECTOR_REQUIRED = "media.sectorRequired";
/** 这台主机没有打印机。 */
inline constexpr std::string_view ERROR_NO_PRINTER = "media.noPrinter";
/** 打印机当前页上没有内容，没有什么可输出的。 */
inline constexpr std::string_view ERROR_PRINTER_EMPTY = "media.printerEmpty";

}  // namespace ember::contracts::media
