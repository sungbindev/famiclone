/*
 * ------------------------------------------------------------------------
 * 名称：manifest.hpp
 * 说明：mapper 插件清单（manifest.json）的数据结构、解析与检查。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-14
 * 备注：字段与规则见 docs/03-mapper插件规范.md 第 4 节；不认识的字段、缺字段、越界、窗口重叠等都拒绝，错误码为 mappers.invalidPackage，说明指出出错的字段。
 *       设置项本阶段只解析与检查，运行时的行为属于后续阶段。
 * ------------------------------------------------------------------------
 */

#pragma once

#include <compare>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "contracts/mappers/cartridge-image.hpp"

namespace ember::infra::mappers {

/** 插件接口版本，当前为 1。 */
inline constexpr std::uint32_t SUPPORTED_API_VERSION = 1;

/** 版本号：主.次.修订。 */
struct Version {
  std::uint32_t major{0};
  std::uint32_t minor{0};
  std::uint32_t patch{0};

  friend auto operator<=>(const Version&, const Version&) = default;
};

/** 总线上的地址范围，两端都包含。 */
struct AddressRange {
  std::uint32_t first{0};
  std::uint32_t last{0};

  [[nodiscard]] std::uint32_t length() const noexcept { return last - first + 1; }
  [[nodiscard]] bool overlaps(const AddressRange& other) const noexcept { return first <= other.last && other.first <= last; }
};

/** 一项匹配条件。 */
struct MatchRule {
  std::uint32_t mapper{0};
  std::optional<std::uint32_t> variant;
  std::optional<std::uint64_t> prgSize;
  std::optional<std::uint64_t> chrSize;
  std::optional<std::uint32_t> crc32Prg;
  std::optional<std::uint32_t> crc32All;

  /** 写出的条件数，mapper 号也算一项。 */
  [[nodiscard]] int conditionCount() const noexcept;

  /** 描述是否满足全部写出的条件。 */
  [[nodiscard]] bool matches(const contracts::mappers::CartridgeDescription& description) const noexcept;
};

/** rom 的数据来源。 */
enum class StorageSource { None, FilePrg, FileChr };

/** 来源为空时改用的 RAM。 */
struct FallbackRam {
  std::uint32_t size{0};
  bool battery{false};
};

/** 存储区声明。 */
struct StorageDecl {
  std::string id;
  bool ram{false};
  StorageSource from{StorageSource::None};
  std::uint32_t size{0};
  bool battery{false};
  std::optional<FallbackRam> fallback;
};

/** 窗口复位后指向的 bank 的写法。 */
enum class InitialKind { Bank, First, Last };

/** 窗口声明。 */
struct WindowDecl {
  std::string id;
  bool ppu{false};
  AddressRange range;
  std::string storage;
  std::uint32_t size{0};
  InitialKind initialKind{InitialKind::First};
  std::uint32_t initialBank{0};
  bool writable{false};
};

/** 名称表的初始连法。 */
enum class NametableMode { Header, Horizontal, Vertical, Single0, Single1, Four };

/** 寄存器范围声明：落在范围内的读写回调脚本。 */
struct RegisterDecl {
  bool ppu{false};
  AddressRange range;
  bool read{false};
  bool write{false};
};

/** 脚本状态声明。 */
struct StateDecl {
  std::string name;
  std::uint32_t bits{0};
  std::string description;
};

/** 设置项种类。 */
enum class SettingKind { Text, Choice, Switch };

/** choice 设置项的选项。 */
struct SettingOption {
  std::string value;
  std::string label;
};

/** 设置项声明（后续阶段才有运行时行为）。 */
struct SettingDecl {
  std::string key;
  std::string label;
  SettingKind kind{SettingKind::Text};
  /** text 与 choice 的默认值。 */
  std::string textDefault;
  /** switch 的默认值。 */
  bool switchDefault{false};
  std::string description;
  std::vector<SettingOption> options;
};

/** 插件清单。 */
struct Manifest {
  std::string id;
  std::string name;
  std::string version;
  Version versionNumber;
  std::string publisher;
  std::string description;
  std::uint32_t apiVersion{0};
  std::vector<std::string> host;
  std::vector<MatchRule> match;
  std::vector<StorageDecl> storage;
  std::vector<WindowDecl> windows;
  NametableMode nametable{NametableMode::Header};
  std::vector<RegisterDecl> registers;
  std::vector<StateDecl> state;
  std::vector<SettingDecl> settings;

  /** 声明了读寄存器。 */
  [[nodiscard]] bool needsReadHandler() const noexcept;
  /** 声明了写寄存器。 */
  [[nodiscard]] bool needsWriteHandler() const noexcept;
};

/**
 * 解析并检查清单。
 * @param text manifest.json 的内容。
 * @throws shared::CoreError mappers.invalidPackage，说明里指出出错的字段。
 */
[[nodiscard]] Manifest parseManifest(std::string_view text);

/** 地址范围的文字，如 $8000-$9FFF；单个地址写成 $4100。 */
[[nodiscard]] std::string formatRange(const AddressRange& range);

/** 名称表连法在清单里的名称：header、horizontal、vertical、single0、single1、four。 */
[[nodiscard]] std::string_view nametableName(NametableMode mode) noexcept;

}  // namespace ember::infra::mappers
