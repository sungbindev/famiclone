/*
 * ------------------------------------------------------------------------
 * 名称：zip-archive.cpp
 * 说明：内存里的只读 zip 包的实现。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-21
 * 备注：基于核心自带的 miniz。
 * ------------------------------------------------------------------------
 */

#include "infra/resources/zip-archive.hpp"

#include <miniz.h>

#include <stdexcept>

namespace ember::win32::infra {

struct ZipArchive::Impl {
  mz_zip_archive zip{};
  bool open{false};
};

ZipArchive::ZipArchive(const unsigned char* data, std::size_t size) : impl_(std::make_unique<Impl>()), data_(data), size_(size) {
  if (mz_zip_reader_init_mem(&impl_->zip, data, size, 0) == MZ_FALSE) throw std::runtime_error("资源包不是合格的 zip");
  impl_->open = true;
}

ZipArchive::~ZipArchive() {
  if (impl_->open) mz_zip_reader_end(&impl_->zip);
}

std::vector<std::string> ZipArchive::names() const {
  const std::lock_guard<std::mutex> lock(mutex_);
  std::vector<std::string> result;
  const mz_uint count = mz_zip_reader_get_num_files(&impl_->zip);
  for (mz_uint index = 0; index < count; ++index) {
    if (mz_zip_reader_is_file_a_directory(&impl_->zip, index) != MZ_FALSE) continue;
    char name[512];
    const mz_uint length = mz_zip_reader_get_filename(&impl_->zip, index, name, sizeof(name));
    if (length > 0) result.emplace_back(name, length - 1);
  }
  return result;
}

std::optional<std::vector<std::uint8_t>> ZipArchive::read(const std::string& name) const {
  const std::lock_guard<std::mutex> lock(mutex_);
  const int index = mz_zip_reader_locate_file(&impl_->zip, name.c_str(), nullptr, 0);
  if (index < 0 || mz_zip_reader_is_file_a_directory(&impl_->zip, static_cast<mz_uint>(index)) != MZ_FALSE) return std::nullopt;
  std::size_t size = 0;
  void* heap = mz_zip_reader_extract_to_heap(&impl_->zip, static_cast<mz_uint>(index), &size, 0);
  if (heap == nullptr) return std::nullopt;
  std::vector<std::uint8_t> content(static_cast<const std::uint8_t*>(heap), static_cast<const std::uint8_t*>(heap) + size);
  mz_free(heap);
  return content;
}

std::string ZipArchive::fingerprint() const {
  const mz_ulong crc = mz_crc32(MZ_CRC32_INIT, data_, size_);
  return std::to_string(static_cast<unsigned long>(crc)) + ":" + std::to_string(size_);
}

}  // namespace ember::win32::infra
