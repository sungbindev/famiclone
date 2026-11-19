/*
 * ------------------------------------------------------------------------
 * 名称：embedded-archive.hpp
 * 说明：取出嵌在可执行文件里的资源包（zip）：预装插件、帮助文档、许可全文。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-21
 * 备注：资源包在可执行文件的资源段里（RCDATA，名称 EMBER_RESOURCES），由构建时嵌入；返回的指针在进程存活期间一直有效，不需要释放；没有嵌入时（只构建了部分目标）没有值。
 * ------------------------------------------------------------------------
 */

#pragma once

#include <cstddef>
#include <optional>

namespace ember::win32::platform {

/** 内存里的资源包。 */
struct EmbeddedArchive {
  const unsigned char* data{nullptr};
  std::size_t size{0};
};

/** 取嵌入的资源包；没有嵌入时没有值。 */
[[nodiscard]] std::optional<EmbeddedArchive> embeddedResourceArchive();

}  // namespace ember::win32::platform
