/*
 * ------------------------------------------------------------------------
 * 名称：zip-builder.hpp
 * 说明：测试用的 zip 生成：把若干（名称，内容）条目打成内存里的 zip。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-21
 * 备注：基于核心自带的 miniz。
 * ------------------------------------------------------------------------
 */

#pragma once

#include <miniz.h>

#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace ember::win32::test {

/** 把条目打成 zip 的内容；名称以 / 结尾的是目录条目。 */
inline std::vector<unsigned char> makeZip(const std::vector<std::pair<std::string, std::string>>& entries) {
  mz_zip_archive zip{};
  if (mz_zip_writer_init_heap(&zip, 0, 0) == MZ_FALSE) throw std::runtime_error("无法创建 zip");
  for (const auto& [name, content] : entries) {
    if (mz_zip_writer_add_mem(&zip, name.c_str(), content.data(), content.size(), MZ_DEFAULT_LEVEL) == MZ_FALSE) throw std::runtime_error("无法加入条目 " + name);
  }
  void* buffer = nullptr;
  std::size_t size = 0;
  if (mz_zip_writer_finalize_heap_archive(&zip, &buffer, &size) == MZ_FALSE) throw std::runtime_error("无法完成 zip");
  std::vector<unsigned char> result(static_cast<const unsigned char*>(buffer), static_cast<const unsigned char*>(buffer) + size);
  mz_zip_writer_end(&zip);
  return result;
}

}  // namespace ember::win32::test
