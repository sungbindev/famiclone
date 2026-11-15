/*
 * ------------------------------------------------------------------------
 * 名称：manifest.cpp
 * 说明：mapper 插件清单的解析与检查实现。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-14
 * 备注：先按结构读出各字段（不认识的字段、类型不对、长度越界直接拒绝），再做跨字段检查：标识唯一、窗口引用的存储区存在、
 *       窗口大小等于地址范围、同一总线上窗口互不重叠、寄存器范围互不重叠、指向 ROM 的窗口只读、设置项默认值合法。
 * ------------------------------------------------------------------------
 */

#include "infra/mappers/manifest.hpp"

#include <algorithm>
#include <cctype>
#include <set>

#include <nlohmann/json.hpp>

#include "contracts/mappers/mapper-error.hpp"
#include "shared/errors/core-error.hpp"
#include "shared/json/json-fields.hpp"

namespace ember::infra::mappers {

namespace {

namespace json_fields = shared::json_fields;
using nlohmann::json;

/** 各部分的数量上限，防止清单被用来耗尽资源。 */
constexpr std::size_t MAX_HOSTS = 8;
/** 外设目前只有 mouse，所以最多一项。 */
constexpr std::size_t MAX_DEVICES = 1;
constexpr std::size_t MAX_MATCH_RULES = 32;
constexpr std::size_t MAX_STORAGE = 16;
constexpr std::size_t MAX_WINDOWS = 64;
constexpr std::size_t MAX_REGISTERS = 64;
constexpr std::size_t MAX_STATE = 64;
constexpr std::size_t MAX_SETTINGS = 32;
constexpr std::size_t MAX_OPTIONS = 32;
/** ram 存储区的容量上限（16 MiB）。 */
constexpr std::int64_t MAX_RAM_SIZE = 16 * 1024 * 1024;
/** CPU 总线上属于卡带的地址从 $4020 开始。 */
constexpr std::uint32_t CPU_CARTRIDGE_FIRST = 0x4020;
constexpr std::uint32_t CPU_LAST = 0xFFFF;
/** PPU 侧的窗口与寄存器只能在图案表地址 $0000-$1FFF。 */
constexpr std::uint32_t PPU_PATTERN_LAST = 0x1FFF;

[[noreturn]] void fail(const std::string& message) { throw shared::SchemaError(message); }

const json& requireField(const json& object, const char* key, const std::string& path) {
  const json* field = json_fields::find(object, key);
  if (field == nullptr) fail(path + "." + key + " 缺少");
  return *field;
}

std::string readText(const json& object, const char* key, const std::string& path, std::size_t minLength, std::size_t maxLength) {
  return json_fields::readString(requireField(object, key, path), path + "." + key, minLength, maxLength, {});
}

std::optional<std::string> readOptionalText(const json& object, const char* key, const std::string& path, std::size_t minLength, std::size_t maxLength) {
  const json* field = json_fields::find(object, key);
  if (field == nullptr) return std::nullopt;
  return json_fields::readString(*field, path + "." + key, minLength, maxLength, {});
}

std::int64_t readInt(const json& object, const char* key, const std::string& path, std::int64_t minimum, std::int64_t maximum) {
  return json_fields::readInteger(requireField(object, key, path), path + "." + key, minimum, maximum);
}

std::optional<std::int64_t> readOptionalInt(const json& object, const char* key, const std::string& path, std::int64_t minimum, std::int64_t maximum) {
  const json* field = json_fields::find(object, key);
  if (field == nullptr) return std::nullopt;
  return json_fields::readInteger(*field, path + "." + key, minimum, maximum);
}

bool readFlag(const json& object, const char* key, const std::string& path) {
  const json* field = json_fields::find(object, key);
  return field != nullptr && json_fields::readBoolean(*field, path + "." + key);
}

/** 读取数组并检查项数。 */
const json& readList(const json& object, const char* key, const std::string& path, std::size_t minItems, std::size_t maxItems) {
  const json& list = requireField(object, key, path);
  if (!list.is_array()) fail(path + "." + key + " 应为数组");
  if (list.size() < minItems) fail(path + "." + key + " 至少 " + std::to_string(minItems) + " 项");
  if (list.size() > maxItems) fail(path + "." + key + " 最多 " + std::to_string(maxItems) + " 项");
  return list;
}

/** 数组可以不写，不写等于空。 */
const json* findList(const json& object, const char* key, const std::string& path, std::size_t maxItems) {
  if (json_fields::find(object, key) == nullptr) return nullptr;
  return &readList(object, key, path, 0, maxItems);
}

/** 标识：小写字母、数字与 -，首字符不是 -。 */
bool isLowerIdentifier(const std::string& text, std::size_t minLength, std::size_t maxLength) {
  if (text.size() < minLength || text.size() > maxLength || text.front() == '-') return false;
  return std::all_of(text.begin(), text.end(), [](char c) { return (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '-'; });
}

/** 状态名与设置键：camelCase。 */
bool isCamelName(const std::string& text) {
  if (text.empty() || text.size() > 32 || !(text.front() >= 'a' && text.front() <= 'z')) return false;
  return std::all_of(text.begin(), text.end(), [](char c) { return std::isalnum(static_cast<unsigned char>(c)) != 0; });
}

std::string readIdentifier(const json& object, const char* key, const std::string& path, std::size_t maxLength) {
  std::string value = readText(object, key, path, 1, maxLength);
  if (!isLowerIdentifier(value, 1, maxLength)) fail(path + "." + key + " 只能用小写字母、数字与 -，且不能以 - 开头");
  return value;
}

bool readBus(const json& object, const std::string& path) {
  const std::string bus = json_fields::readString(requireField(object, "bus", path), path + ".bus", 1, 8, {"cpu", "ppu"});
  return bus == "ppu";
}

/** 版本号：三段十进制整数，不带多余的前导零。 */
Version parseVersion(const std::string& text, const std::string& path) {
  std::uint32_t parts[3] = {0, 0, 0};
  std::size_t index = 0;
  std::size_t digits = 0;
  for (const char c : text) {
    if (c == '.') {
      if (digits == 0 || ++index > 2) fail(path + " 应为 主.次.修订，如 1.0.0");
      digits = 0;
      continue;
    }
    if (c < '0' || c > '9' || digits >= 9 || (digits == 1 && parts[index] == 0)) fail(path + " 应为 主.次.修订，如 1.0.0");
    parts[index] = parts[index] * 10 + static_cast<std::uint32_t>(c - '0');
    ++digits;
  }
  if (index != 2 || digits == 0) fail(path + " 应为 主.次.修订，如 1.0.0");
  return Version{parts[0], parts[1], parts[2]};
}

std::uint32_t parseHexAddress(std::string_view part, const std::string& path) {
  if (part.size() < 2 || part.size() > 5 || part.front() != '$') fail(path + " 的地址应写成 $XXXX（十六进制，1 到 4 位）");
  std::uint32_t value = 0;
  for (const char c : part.substr(1)) {
    std::uint32_t digit = 0;
    if (c >= '0' && c <= '9') digit = static_cast<std::uint32_t>(c - '0');
    else if (c >= 'a' && c <= 'f') digit = static_cast<std::uint32_t>(c - 'a' + 10);
    else if (c >= 'A' && c <= 'F') digit = static_cast<std::uint32_t>(c - 'A' + 10);
    else fail(path + " 的地址应写成 $XXXX（十六进制，1 到 4 位）");
    value = value * 16 + digit;
  }
  return value;
}

AddressRange parseRange(const std::string& text, const std::string& path) {
  AddressRange range;
  const std::size_t dash = text.find('-');
  if (dash == std::string::npos) {
    range.first = range.last = parseHexAddress(text, path);
  } else {
    range.first = parseHexAddress(std::string_view(text).substr(0, dash), path);
    range.last = parseHexAddress(std::string_view(text).substr(dash + 1), path);
  }
  if (range.first > range.last) fail(path + " 的起点大于终点");
  return range;
}

AddressRange readRange(const json& object, const char* key, const std::string& path, bool ppu) {
  AddressRange range = parseRange(json_fields::readString(requireField(object, key, path), path + "." + key, 1, 16, {}), path + "." + key);
  if (ppu) {
    if (range.last > PPU_PATTERN_LAST) fail(path + "." + key + " 超出范围：PPU 侧只能用图案表地址 $0000-$1FFF");
  } else if (range.first < CPU_CARTRIDGE_FIRST || range.last > CPU_LAST) {
    fail(path + "." + key + " 超出范围：CPU 侧属于卡带的地址是 $4020-$FFFF");
  }
  return range;
}

std::uint32_t parseCrc(const std::string& text, const std::string& path) {
  if (text.size() != 8) fail(path + " 应为 8 位十六进制");
  std::uint32_t value = 0;
  for (const char c : text) {
    if (std::isxdigit(static_cast<unsigned char>(c)) == 0) fail(path + " 应为 8 位十六进制");
    value = value * 16 + static_cast<std::uint32_t>(c <= '9' ? c - '0' : (std::tolower(static_cast<unsigned char>(c)) - 'a' + 10));
  }
  return value;
}

MatchRule readMatchRule(const json& item, const std::string& path) {
  json_fields::requireObject(item, path);
  json_fields::rejectUnknown(item, path, {"mapper", "variant", "prgSize", "chrSize", "crc32Prg", "crc32All"});
  MatchRule rule;
  rule.mapper = static_cast<std::uint32_t>(readInt(item, "mapper", path, 0, 4095));
  if (const auto value = readOptionalInt(item, "variant", path, 0, 255)) rule.variant = static_cast<std::uint32_t>(*value);
  if (const auto value = readOptionalInt(item, "prgSize", path, 0, MAX_RAM_SIZE * 256)) rule.prgSize = static_cast<std::uint64_t>(*value);
  if (const auto value = readOptionalInt(item, "chrSize", path, 0, MAX_RAM_SIZE * 256)) rule.chrSize = static_cast<std::uint64_t>(*value);
  if (const auto value = readOptionalText(item, "crc32Prg", path, 8, 8)) rule.crc32Prg = parseCrc(*value, path + ".crc32Prg");
  if (const auto value = readOptionalText(item, "crc32All", path, 8, 8)) rule.crc32All = parseCrc(*value, path + ".crc32All");
  return rule;
}

StorageDecl readStorage(const json& item, const std::string& path) {
  json_fields::requireObject(item, path);
  json_fields::rejectUnknown(item, path, {"id", "kind", "from", "size", "battery", "fallback"});
  StorageDecl decl;
  decl.id = readIdentifier(item, "id", path, 32);
  const std::string kind = json_fields::readString(requireField(item, "kind", path), path + ".kind", 1, 8, {"rom", "ram"});
  decl.ram = kind == "ram";
  decl.battery = readFlag(item, "battery", path);
  if (decl.ram) {
    if (json_fields::find(item, "from") != nullptr) fail(path + ".from 只用于 rom");
    if (json_fields::find(item, "fallback") != nullptr) fail(path + ".fallback 只用于 rom");
    decl.size = static_cast<std::uint32_t>(readInt(item, "size", path, 1, MAX_RAM_SIZE));
    return decl;
  }
  if (json_fields::find(item, "size") != nullptr) fail(path + ".size 只用于 ram");
  if (decl.battery) fail(path + ".battery 只用于 ram");
  const std::string from = json_fields::readString(requireField(item, "from", path), path + ".from", 1, 16, {"file.prg", "file.chr"});
  decl.from = from == "file.prg" ? StorageSource::FilePrg : StorageSource::FileChr;
  if (const json* fallback = json_fields::find(item, "fallback")) {
    const std::string fallbackPath = path + ".fallback";
    json_fields::requireObject(*fallback, fallbackPath);
    json_fields::rejectUnknown(*fallback, fallbackPath, {"kind", "size", "battery"});
    json_fields::readString(requireField(*fallback, "kind", fallbackPath), fallbackPath + ".kind", 1, 8, {"ram"});
    decl.fallback = FallbackRam{static_cast<std::uint32_t>(readInt(*fallback, "size", fallbackPath, 1, MAX_RAM_SIZE)), readFlag(*fallback, "battery", fallbackPath)};
  }
  return decl;
}

WindowDecl readWindow(const json& item, const std::string& path) {
  json_fields::requireObject(item, path);
  json_fields::rejectUnknown(item, path, {"id", "bus", "range", "storage", "size", "initial", "access"});
  WindowDecl decl;
  decl.id = readIdentifier(item, "id", path, 32);
  decl.ppu = readBus(item, path);
  decl.range = readRange(item, "range", path, decl.ppu);
  decl.storage = readIdentifier(item, "storage", path, 32);
  decl.size = static_cast<std::uint32_t>(readInt(item, "size", path, 1, 65536));
  if (decl.size != decl.range.length()) fail(path + ".size 必须等于地址范围的大小 " + std::to_string(decl.range.length()));
  if (const json* initial = json_fields::find(item, "initial")) {
    if (initial->is_string()) {
      const std::string text = json_fields::readString(*initial, path + ".initial", 1, 8, {"first", "last"});
      decl.initialKind = text == "last" ? InitialKind::Last : InitialKind::First;
    } else {
      decl.initialKind = InitialKind::Bank;
      decl.initialBank = static_cast<std::uint32_t>(json_fields::readInteger(*initial, path + ".initial", 0, 0xFFFFFF));
    }
  }
  const std::string access = json_fields::readString(requireField(item, "access", path), path + ".access", 1, 2, {"r", "rw"});
  decl.writable = access == "rw";
  return decl;
}

RegisterDecl readRegister(const json& item, const std::string& path) {
  json_fields::requireObject(item, path);
  json_fields::rejectUnknown(item, path, {"bus", "range", "read", "write"});
  RegisterDecl decl;
  decl.ppu = readBus(item, path);
  decl.range = readRange(item, "range", path, decl.ppu);
  decl.read = readFlag(item, "read", path);
  decl.write = readFlag(item, "write", path);
  if (!decl.read && !decl.write) fail(path + " 的 read 与 write 至少要有一个为 true");
  return decl;
}

StateDecl readState(const json& item, const std::string& path) {
  json_fields::requireObject(item, path);
  json_fields::rejectUnknown(item, path, {"name", "bits", "description"});
  StateDecl decl;
  decl.name = readText(item, "name", path, 1, 32);
  if (!isCamelName(decl.name)) fail(path + ".name 应为字母开头的 camelCase，只含字母与数字");
  decl.bits = static_cast<std::uint32_t>(readInt(item, "bits", path, 1, 32));
  decl.description = readText(item, "description", path, 1, 200);
  return decl;
}

SettingDecl readSetting(const json& item, const std::string& path) {
  json_fields::requireObject(item, path);
  json_fields::rejectUnknown(item, path, {"key", "label", "kind", "default", "description", "options"});
  SettingDecl decl;
  decl.key = readText(item, "key", path, 1, 32);
  if (!isCamelName(decl.key)) fail(path + ".key 应为字母开头的 camelCase，只含字母与数字");
  decl.label = readText(item, "label", path, 1, 40);
  const std::string kind = json_fields::readString(requireField(item, "kind", path), path + ".kind", 1, 8, {"text", "choice", "switch"});
  decl.description = readOptionalText(item, "description", path, 0, 200).value_or("");
  const json& defaultValue = requireField(item, "default", path);
  if (kind == "switch") {
    decl.kind = SettingKind::Switch;
    if (json_fields::find(item, "options") != nullptr) fail(path + ".options 只用于 choice");
    decl.switchDefault = json_fields::readBoolean(defaultValue, path + ".default");
    return decl;
  }
  decl.textDefault = json_fields::readString(defaultValue, path + ".default", 0, 200, {});
  if (kind == "text") {
    decl.kind = SettingKind::Text;
    if (json_fields::find(item, "options") != nullptr) fail(path + ".options 只用于 choice");
    return decl;
  }
  decl.kind = SettingKind::Choice;
  const json& options = readList(item, "options", path, 1, MAX_OPTIONS);
  for (std::size_t index = 0; index < options.size(); ++index) {
    const std::string optionPath = path + ".options[" + std::to_string(index) + "]";
    json_fields::requireObject(options[index], optionPath);
    json_fields::rejectUnknown(options[index], optionPath, {"value", "label"});
    decl.options.push_back(SettingOption{readText(options[index], "value", optionPath, 1, 64), readText(options[index], "label", optionPath, 1, 40)});
  }
  const bool found = std::any_of(decl.options.begin(), decl.options.end(), [&](const SettingOption& option) { return option.value == decl.textDefault; });
  if (!found) fail(path + ".default 必须是 options 里的一个取值");
  return decl;
}

NametableMode readNametable(const json& root, const std::string& path) {
  const json* field = json_fields::find(root, "nametable");
  if (field == nullptr) return NametableMode::Header;
  const std::string value = json_fields::readString(*field, path + ".nametable", 1, 16, {"header", "horizontal", "vertical", "single0", "single1", "four"});
  if (value == "horizontal") return NametableMode::Horizontal;
  if (value == "vertical") return NametableMode::Vertical;
  if (value == "single0") return NametableMode::Single0;
  if (value == "single1") return NametableMode::Single1;
  if (value == "four") return NametableMode::Four;
  return NametableMode::Header;
}

/** 跨字段检查。 */
void checkConsistency(const Manifest& manifest) {
  std::set<std::string> storageIds;
  for (const auto& storage : manifest.storage) {
    if (!storageIds.insert(storage.id).second) fail("storage 里的标识重复：" + storage.id);
  }
  std::set<std::string> windowIds;
  for (const auto& window : manifest.windows) {
    if (!windowIds.insert(window.id).second) fail("windows 里的标识重复：" + window.id);
    const auto storage = std::find_if(manifest.storage.begin(), manifest.storage.end(), [&](const StorageDecl& item) { return item.id == window.storage; });
    if (storage == manifest.storage.end()) fail("窗口 " + window.id + " 引用了不存在的存储区：" + window.storage);
    if (window.writable && !storage->ram && !storage->fallback) fail("窗口 " + window.id + " 指向 ROM，access 只能是 r");
  }
  for (std::size_t first = 0; first < manifest.windows.size(); ++first) {
    for (std::size_t second = first + 1; second < manifest.windows.size(); ++second) {
      const auto& a = manifest.windows[first];
      const auto& b = manifest.windows[second];
      if (a.ppu == b.ppu && a.range.overlaps(b.range)) fail("窗口 " + a.id + " 与 " + b.id + " 的地址范围重叠");
    }
  }
  for (std::size_t first = 0; first < manifest.registers.size(); ++first) {
    for (std::size_t second = first + 1; second < manifest.registers.size(); ++second) {
      const auto& a = manifest.registers[first];
      const auto& b = manifest.registers[second];
      if (a.ppu == b.ppu && a.range.overlaps(b.range)) fail("registers 的第 " + std::to_string(first) + " 项与第 " + std::to_string(second) + " 项的地址范围重叠");
    }
  }
  std::set<std::string> stateNames;
  for (const auto& state : manifest.state) {
    if (!stateNames.insert(state.name).second) fail("state 里的名称重复：" + state.name);
  }
  std::set<std::string> settingKeys;
  for (const auto& setting : manifest.settings) {
    if (!settingKeys.insert(setting.key).second) fail("settings 里的键重复：" + setting.key);
  }
  std::set<std::string> hosts;
  for (const auto& host : manifest.host) {
    if (!hosts.insert(host).second) fail("host 里的型号重复：" + host);
  }
}

Manifest build(const json& root) {
  const std::string path = "manifest";
  json_fields::requireObject(root, path);
  json_fields::rejectUnknown(root, path,
                             {"id", "name", "version", "apiVersion", "publisher", "description", "host", "devices", "match", "storage", "windows", "nametable", "registers", "state", "settings"});
  Manifest manifest;
  manifest.id = readText(root, "id", path, 3, 40);
  if (!isLowerIdentifier(manifest.id, 3, 40)) fail(path + ".id 只能用小写字母、数字与 -，3 到 40 个字符，且不能以 - 开头");
  manifest.name = readText(root, "name", path, 1, 40);
  manifest.version = readText(root, "version", path, 5, 32);
  manifest.versionNumber = parseVersion(manifest.version, path + ".version");
  const auto apiVersion = readInt(root, "apiVersion", path, 0, 1000000);
  if (apiVersion != static_cast<std::int64_t>(SUPPORTED_API_VERSION)) fail(path + ".apiVersion 为 " + std::to_string(apiVersion) + "，本程序只支持插件接口版本 " + std::to_string(SUPPORTED_API_VERSION));
  manifest.apiVersion = static_cast<std::uint32_t>(apiVersion);
  manifest.publisher = readText(root, "publisher", path, 1, 40);
  manifest.description = readOptionalText(root, "description", path, 0, 200).value_or("");

  const json& hosts = readList(root, "host", path, 1, MAX_HOSTS);
  for (std::size_t index = 0; index < hosts.size(); ++index) {
    const std::string hostPath = path + ".host[" + std::to_string(index) + "]";
    std::string host = json_fields::readString(hosts[index], hostPath, 1, 32, {});
    if (!isLowerIdentifier(host, 1, 32)) fail(hostPath + " 只能用小写字母、数字与 -");
    manifest.host.push_back(std::move(host));
  }
  if (const json* list = findList(root, "devices", path, MAX_DEVICES)) {
    for (std::size_t index = 0; index < list->size(); ++index) {
      std::string device = json_fields::readString((*list)[index], path + ".devices[" + std::to_string(index) + "]", 1, 16, {"mouse"});
      if (std::find(manifest.devices.begin(), manifest.devices.end(), device) != manifest.devices.end()) fail("devices 里的外设重复：" + device);
      manifest.devices.push_back(std::move(device));
    }
  }
  const json& matches = readList(root, "match", path, 1, MAX_MATCH_RULES);
  for (std::size_t index = 0; index < matches.size(); ++index) manifest.match.push_back(readMatchRule(matches[index], path + ".match[" + std::to_string(index) + "]"));

  if (const json* list = findList(root, "storage", path, MAX_STORAGE)) {
    for (std::size_t index = 0; index < list->size(); ++index) manifest.storage.push_back(readStorage((*list)[index], path + ".storage[" + std::to_string(index) + "]"));
  }
  if (const json* list = findList(root, "windows", path, MAX_WINDOWS)) {
    for (std::size_t index = 0; index < list->size(); ++index) manifest.windows.push_back(readWindow((*list)[index], path + ".windows[" + std::to_string(index) + "]"));
  }
  manifest.nametable = readNametable(root, path);
  if (const json* list = findList(root, "registers", path, MAX_REGISTERS)) {
    for (std::size_t index = 0; index < list->size(); ++index) manifest.registers.push_back(readRegister((*list)[index], path + ".registers[" + std::to_string(index) + "]"));
  }
  if (const json* list = findList(root, "state", path, MAX_STATE)) {
    for (std::size_t index = 0; index < list->size(); ++index) manifest.state.push_back(readState((*list)[index], path + ".state[" + std::to_string(index) + "]"));
  }
  if (const json* list = findList(root, "settings", path, MAX_SETTINGS)) {
    for (std::size_t index = 0; index < list->size(); ++index) manifest.settings.push_back(readSetting((*list)[index], path + ".settings[" + std::to_string(index) + "]"));
  }
  checkConsistency(manifest);
  return manifest;
}

}  // namespace

int MatchRule::conditionCount() const noexcept {
  return 1 + (variant ? 1 : 0) + (prgSize ? 1 : 0) + (chrSize ? 1 : 0) + (crc32Prg ? 1 : 0) + (crc32All ? 1 : 0);
}

bool MatchRule::matches(const contracts::mappers::CartridgeDescription& description) const noexcept {
  if (mapper != description.mapper) return false;
  if (variant && *variant != description.variant) return false;
  if (prgSize && *prgSize != description.prgSize) return false;
  if (chrSize && *chrSize != description.chrSize) return false;
  if (crc32Prg && *crc32Prg != description.crc32Prg) return false;
  if (crc32All && *crc32All != description.crc32All) return false;
  return true;
}

bool Manifest::needsReadHandler() const noexcept {
  return std::any_of(registers.begin(), registers.end(), [](const RegisterDecl& item) { return item.read; });
}

bool Manifest::needsWriteHandler() const noexcept {
  return std::any_of(registers.begin(), registers.end(), [](const RegisterDecl& item) { return item.write; });
}

Manifest parseManifest(std::string_view text) {
  const json root = json::parse(text.begin(), text.end(), nullptr, false);
  if (root.is_discarded()) throw shared::CoreError(contracts::mappers::ERROR_INVALID_PACKAGE, "manifest.json 不合格：不是合法的 JSON");
  try {
    return build(root);
  } catch (const shared::SchemaError& error) {
    throw shared::CoreError(contracts::mappers::ERROR_INVALID_PACKAGE, std::string("manifest.json 不合格：") + error.what());
  }
}

std::string formatRange(const AddressRange& range) {
  const auto hex = [](std::uint32_t value) {
    static constexpr char DIGITS[] = "0123456789ABCDEF";
    std::string text = "$";
    for (int shift = 12; shift >= 0; shift -= 4) text.push_back(DIGITS[(value >> shift) & 0xFU]);
    return text;
  };
  return range.first == range.last ? hex(range.first) : hex(range.first) + "-" + hex(range.last);
}

std::string_view nametableName(NametableMode mode) noexcept {
  switch (mode) {
    case NametableMode::Horizontal:
      return "horizontal";
    case NametableMode::Vertical:
      return "vertical";
    case NametableMode::Single0:
      return "single0";
    case NametableMode::Single1:
      return "single1";
    case NametableMode::Four:
      return "four";
    case NametableMode::Header:
      break;
  }
  return "header";
}

}  // namespace ember::infra::mappers
