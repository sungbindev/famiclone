/*
 * ------------------------------------------------------------------------
 * 名称：embedded-archive.cpp
 * 说明：取出嵌在可执行文件里的资源包的实现。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-21
 * 备注：无。
 * ------------------------------------------------------------------------
 */

#include "platform/resources/embedded-archive.hpp"

#include <windows.h>

namespace ember::win32::platform {

std::optional<EmbeddedArchive> embeddedResourceArchive() {
  HRSRC resource = FindResourceW(nullptr, L"EMBER_RESOURCES", reinterpret_cast<LPCWSTR>(RT_RCDATA));
  if (resource == nullptr) return std::nullopt;
  const DWORD size = SizeofResource(nullptr, resource);
  HGLOBAL loaded = LoadResource(nullptr, resource);
  if (size == 0 || loaded == nullptr) return std::nullopt;
  const void* data = LockResource(loaded);
  if (data == nullptr) return std::nullopt;
  return EmbeddedArchive{static_cast<const unsigned char*>(data), size};
}

}  // namespace ember::win32::platform
