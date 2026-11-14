/*
 * ------------------------------------------------------------------------
 * 名称：bios-storage.hpp
 * 说明：machines 契约：BIOS 文件的存取：读用户选定的源文件，保存、查找、读取与删除各机型已添加的 BIOS。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-15
 * 备注：BIOS 是用户自己的文件，来源路径由主进程根据用户的选择给出；已添加的 BIOS 保存在用户数据目录的 bios/<机型>/ 下，每个机型最多一个文件；
 *       业务代码按机型标识与文件名引用，不拼接磁盘路径；实现在 infra/machines。
 * ------------------------------------------------------------------------
 */

#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace ember::contracts::machines {

/** 已保存的 BIOS 的描述。 */
struct StoredBios {
  /** 文件名，不含路径。 */
  std::string fileName;
  /** 文件大小（字节）。 */
  std::int64_t size{0};
};

/** BIOS 文件的存取。 */
class BiosStorage {
 public:
  virtual ~BiosStorage() = default;

  /**
   * 读取用户选定的源文件。
   * @param path 文件路径（UTF-8）。
   * @throws shared::CoreError machines.fileNotFound：文件不存在；machines.fileUnreadable：不是普通文件或读取失败；machines.biosInvalid：文件大得不可能是 BIOS。
   */
  [[nodiscard]] virtual std::vector<std::uint8_t> readSource(const std::string& path) const = 0;

  /** 机型已保存的 BIOS；没有时为空。 */
  [[nodiscard]] virtual std::optional<StoredBios> find(const std::string& machine) const = 0;

  /**
   * 读出机型已保存的 BIOS 的内容。
   * @throws shared::CoreError machines.noBios：没有保存；machines.biosAccessFailed：读取失败。
   */
  [[nodiscard]] virtual std::vector<std::uint8_t> load(const std::string& machine) const = 0;

  /**
   * 保存机型的 BIOS，替换已有的：新文件先写好，成功之后才删旧文件，失败时原有的不变。
   * @param machine 机型标识。
   * @param fileName 文件名，不含路径。
   * @param bytes 文件内容。
   * @throws shared::CoreError machines.biosAccessFailed。
   */
  virtual void store(const std::string& machine, const std::string& fileName, const std::vector<std::uint8_t>& bytes) = 0;

  /**
   * 删除机型已保存的 BIOS；没有时没有效果。
   * @throws shared::CoreError machines.biosAccessFailed。
   */
  virtual void remove(const std::string& machine) = 0;
};

}  // namespace ember::contracts::machines
