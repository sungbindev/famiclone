/*
 * ------------------------------------------------------------------------
 * 名称：dos-catalog.hpp
 * 说明：DOS 3.3 风格目录的共用常量与工具：Apple DOS 3.3 与金字塔 DOS 的目录项结构相同
 *       （35 字节：磁道、扇区、类型、30 字节名称、2 字节占用扇区数），类型名与导出扩展名也相同。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-27
 * 备注：目录项里的名称字节最高位为 1，显示时去掉；T/S 引用是逻辑扇区号，换算成镜像里的位置由各文件系统自己做。
 * ------------------------------------------------------------------------
 */

#pragma once

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <string>

namespace ember::logic::filemanager::dos_catalog {

/** 卷目录表所在的磁道。 */
inline constexpr unsigned VTOC_TRACK = 17;
/** 一个目录项的字节数。 */
inline constexpr std::size_t CATALOG_ENTRY_SIZE = 35;
/** 目录扇区里第一个目录项的偏移。 */
inline constexpr std::size_t CATALOG_FIRST_ENTRY = 0x0B;
/** 每个目录扇区的目录项个数。 */
inline constexpr std::size_t CATALOG_ENTRIES_PER_SECTOR = 7;
/** 目录项里名称的偏移与长度。 */
inline constexpr std::size_t NAME_OFFSET = 3;
inline constexpr std::size_t NAME_LENGTH = 30;
/** 一个 T/S 列表扇区里的磁道扇区对个数与第一对的偏移。 */
inline constexpr std::size_t PAIRS_PER_LIST = 122;
inline constexpr std::size_t LIST_FIRST_PAIR = 0x0C;
/** 目录项第一个字节是它：已删除。 */
inline constexpr std::uint8_t ENTRY_DELETED = 0xFF;
/** 类型字节里的锁定标志。 */
inline constexpr std::uint8_t LOCKED_FLAG = 0x80;
/** 目录链与列表链的最大长度，防止损坏的盘里出现循环。 */
inline constexpr std::size_t MAX_CHAIN = 1000;

/** 文件类型的显示名称。 */
[[nodiscard]] inline std::string typeName(std::uint8_t type) {
  switch (type & 0x7FU) {
    case 0x00: return "文本";
    case 0x01: return "整数 BASIC";
    case 0x02: return "Applesoft";
    case 0x04: return "二进制";
    case 0x08: return "S 型";
    case 0x10: return "可重定位";
    case 0x20: return "A 型";
    case 0x40: return "B 型";
    default: {
      char text[8];
      std::snprintf(text, sizeof(text), "$%02X", type & 0x7FU);
      return text;
    }
  }
}

/** 导出时文件名的扩展名。 */
[[nodiscard]] inline const char* typeExtension(std::uint8_t type) {
  switch (type & 0x7FU) {
    case 0x00: return ".TXT";
    case 0x01: return ".INT";
    case 0x02: return ".BAS";
    case 0x04: return ".BIN";
    default: return ".DAT";
  }
}

/** 目录项里的名称，去掉高位与末尾的空格。 */
[[nodiscard]] inline std::string entryName(const std::uint8_t* entry) {
  std::string name;
  for (std::size_t i = 0; i < NAME_LENGTH; ++i) {
    const auto c = static_cast<char>(entry[NAME_OFFSET + i] & 0x7FU);
    name.push_back(c >= 0x20 && c < 0x7F ? c : '?');
  }
  while (!name.empty() && name.back() == ' ') name.pop_back();
  return name;
}

}  // namespace ember::logic::filemanager::dos_catalog
