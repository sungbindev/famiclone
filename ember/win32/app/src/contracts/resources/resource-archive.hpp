/*
 * ------------------------------------------------------------------------
 * 名称：resource-archive.hpp
 * 说明：随程序分发的资源包的抽象：列出条目名称、按名称读出条目内容、给出包的指纹。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-21
 * 备注：帮助文档站点与预装插件的解压只经这个抽象读取资源包；具体实现（内存里的 zip）在 infra/resources，只在组装根创建；条目名称用 / 分隔；读取可能在多个线程里同时进行，实现要保证线程安全。
 * ------------------------------------------------------------------------
 */

#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace ember::win32::contracts {

/** 只读的资源包。 */
class ResourceArchive {
 public:
  virtual ~ResourceArchive() = default;

  /** 全部文件条目的名称（不含目录条目），按包里的顺序。 */
  [[nodiscard]] virtual std::vector<std::string> names() const = 0;
  /** 读出一个条目的内容；不存在或读取失败时没有值。 */
  [[nodiscard]] virtual std::optional<std::vector<std::uint8_t>> read(const std::string& name) const = 0;
  /** 整个包的指纹，包的内容变了它就变，用来判断是否需要重新解压。 */
  [[nodiscard]] virtual std::string fingerprint() const = 0;
};

}  // namespace ember::win32::contracts
