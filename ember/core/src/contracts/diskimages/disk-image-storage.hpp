/*
 * ------------------------------------------------------------------------
 * 名称：disk-image-storage.hpp
 * 说明：diskimages 契约：镜像目录里的文件存取：扫描目录、读取镜像、就地改写镜像里的一段字节、新建镜像文件，以及读取拖入的 Windows 文件与文件夹。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-25
 * 备注：镜像用“镜像目录 + 相对路径”定位，相对路径用 / 分隔；实现必须拒绝越出镜像目录的路径（含上级目录、盘符、符号链接指到目录外）与不支持的扩展名；
 *       读取拖入的路径只来自用户的拖放，由外壳登记后交给核心；实现在 infra/diskimages；错误经 shared::CoreError 抛出，错误码见 diskimages-error.hpp。
 * ------------------------------------------------------------------------
 */

#pragma once

#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <vector>

namespace ember::contracts::diskimages {

/** 支持的镜像扩展名（小写，不带点）。 */
inline constexpr const char* SUPPORTED_EXTENSIONS[] = {"img", "ima", "vfd", "dsk", "flp", "do", "po", "nib", "woz"};
/** 镜像文件的大小上限（字节），与软驱载入的上限一致。 */
inline constexpr std::uintmax_t MAX_IMAGE_SIZE = 16ULL * 1024 * 1024;

/** 扫描得到的一项：镜像文件或含有镜像的文件夹。 */
struct StoredImageInfo {
  /** 相对镜像目录的路径，用 / 分隔。 */
  std::string path;
  bool directory{false};
  /** 文件大小（字节）；文件夹为 0。 */
  std::uint64_t size{0};
  /** 修改时间的戳，只用来判断文件有没有变化。 */
  std::int64_t stamp{0};
};

/** 拖入的 Windows 文件或文件夹。 */
struct HostItem {
  std::string name;
  bool directory{false};
  /** 文件的内容；文件夹没有。 */
  std::vector<std::uint8_t> data;
  /** 文件夹里的内容，文件夹在前、按名称排序。 */
  std::vector<HostItem> children;
};

/** 镜像目录里的文件存取。 */
class DiskImageStorage {
 public:
  virtual ~DiskImageStorage() = default;

  /** 路径是不是存在的文件夹。 */
  [[nodiscard]] virtual bool isDirectory(const std::string& path) const = 0;

  /**
   * 递归扫描镜像目录：只列出支持扩展名的镜像文件与含有镜像的文件夹（不跟随符号链接）。
   * @param limit 最多列出多少个镜像文件；超过时停止并把 limited 置为 true。
   * @param available 目录不存在或不能读取时置为 false，返回空。
   */
  [[nodiscard]] virtual std::vector<StoredImageInfo> scan(const std::string& root, std::size_t limit, bool& limited, bool& available) const = 0;

  /**
   * 读取整个镜像。
   * @throws shared::CoreError diskimages.invalidPath、imageNotFound、imageUnreadable（含超过大小上限）。
   */
  [[nodiscard]] virtual std::vector<std::uint8_t> read(const std::string& root, const std::string& relative) const = 0;

  /**
   * 把一段字节写进镜像文件的指定位置，文件的其余部分与大小不变。
   * @throws shared::CoreError diskimages.invalidPath、imageNotFound、saveFailed。
   */
  virtual void writeAt(const std::string& root, const std::string& relative, std::size_t offset, std::span<const std::uint8_t> bytes) = 0;

  /**
   * 在镜像目录的一个文件夹里新建镜像文件，文件名是“基本名.扩展名”，重名时依次加“ (2)”“ (3)”…；先写临时文件再改名。
   * @return 新文件的相对路径。
   * @throws shared::CoreError diskimages.invalidPath、saveFailed。
   */
  [[nodiscard]] virtual std::string create(const std::string& root, const std::string& relativeDirectory, const std::string& baseName, const std::string& extension, const std::vector<std::uint8_t>& bytes) = 0;

  /**
   * 镜像文件的完整路径。
   * @throws shared::CoreError diskimages.invalidPath、imageNotFound。
   */
  [[nodiscard]] virtual std::string absolutePath(const std::string& root, const std::string& relative) const = 0;

  /** 两个路径是不是同一个文件（大小写、分隔符、相对写法不同也算）。 */
  [[nodiscard]] virtual bool samePath(const std::string& a, const std::string& b) const = 0;

  /**
   * 读取拖入的文件或文件夹（文件夹连同里面的全部内容，不跟随符号链接）；总量、层数与个数有上限。
   * @throws shared::CoreError diskimages.invalidPath（找不到、读不了、超过上限）。
   */
  [[nodiscard]] virtual HostItem readHostItem(const std::string& path) const = 0;
};

}  // namespace ember::contracts::diskimages
