/*
 * ------------------------------------------------------------------------
 * 名称：zip-archive.hpp
 * 说明：内存里的只读 zip 包：列出条目名称、按名称读出条目内容。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-21
 * 备注：包的内存由调用方保证在本对象存活期间不变；读取可能在本机服务的多个工作线程里同时进行，所以读取用互斥量保护（读取器本身不是线程安全的）；条目名称用 / 分隔。
 * ------------------------------------------------------------------------
 */

#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <vector>

#include "contracts/resources/resource-archive.hpp"

namespace ember::win32::infra {

/** 内存里的只读 zip 包。 */
class ZipArchive final : public contracts::ResourceArchive {
 public:
  /**
   * @param data 包的内容，调用方保证它在本对象存活期间不变。
   * @param size 字节数。
   * @throws std::runtime_error 不是合格的 zip 时。
   */
  ZipArchive(const unsigned char* data, std::size_t size);
  ~ZipArchive() override;
  ZipArchive(const ZipArchive&) = delete;
  ZipArchive& operator=(const ZipArchive&) = delete;

  /** 全部文件条目的名称（不含目录条目），按包里的顺序。 */
  [[nodiscard]] std::vector<std::string> names() const override;
  /** 读出一个条目的内容；不存在或解压失败时没有值。 */
  [[nodiscard]] std::optional<std::vector<std::uint8_t>> read(const std::string& name) const override;
  /** 整个包的 CRC-32 与长度的文字，用来判断包有没有变化。 */
  [[nodiscard]] std::string fingerprint() const override;

 private:
  struct Impl;
  std::unique_ptr<Impl> impl_;
  const unsigned char* data_;
  std::size_t size_;
  mutable std::mutex mutex_;
};

}  // namespace ember::win32::infra
