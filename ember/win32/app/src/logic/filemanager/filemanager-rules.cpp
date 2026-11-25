/*
 * ------------------------------------------------------------------------
 * 名称：filemanager-rules.cpp
 * 说明：媒体库规则的实现。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-25
 * 备注：无。
 * ------------------------------------------------------------------------
 */

#include "logic/filemanager/filemanager-rules.hpp"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdio>

#include "shared/update/update-manifest.hpp"

namespace ember::win32::logic {

namespace {

constexpr double kKilobyte = 1024.0;
constexpr double kMegabyte = 1024.0 * 1024.0;
/** 新建文件夹的基础名：所有文件系统都认的短名称。 */
constexpr const char* kFolderBaseName = "NEWDIR";
/** 序号最大到多少（再大就放弃加序号）。 */
constexpr int kMaxFolderNumber = 999;

/** 保留最多 decimals 位小数并去掉末尾的 0。 */
std::string trimmed(double value, int decimals) {
  char buffer[64];
  std::snprintf(buffer, sizeof(buffer), "%.*f", decimals, value);
  std::string text = buffer;
  if (text.find('.') != std::string::npos) {
    while (!text.empty() && text.back() == '0') text.pop_back();
    if (!text.empty() && text.back() == '.') text.pop_back();
  }
  return text;
}

std::string upper(std::string text) {
  for (char& c : text) c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
  return text;
}

/** 读必填的字符串字段。 */
bool readString(const nlohmann::json& object, const char* name, std::string& out) {
  const auto found = object.find(name);
  if (found == object.end() || !found->is_string()) return false;
  out = found->get<std::string>();
  return true;
}

bool readBool(const nlohmann::json& object, const char* name, bool& out) {
  const auto found = object.find(name);
  if (found == object.end() || !found->is_boolean()) return false;
  out = found->get<bool>();
  return true;
}

bool readInteger(const nlohmann::json& object, const char* name, std::uint64_t& out) {
  const auto found = object.find(name);
  if (found == object.end() || !found->is_number_integer() || found->get<std::int64_t>() < 0) return false;
  out = found->get<std::uint64_t>();
  return true;
}

}  // namespace

std::string imageSizeText(std::uint64_t bytes) {
  const double value = static_cast<double>(bytes);
  if (value >= kMegabyte) return trimmed(value / kMegabyte, 2) + " MB";
  if (value >= kKilobyte) return trimmed(value / kKilobyte, 1) + " KB";
  return std::to_string(bytes) + " 字节";
}

std::string entrySizeText(std::uint64_t bytes, bool allocated) {
  std::string digits = std::to_string(bytes);
  std::string grouped;
  for (std::size_t i = 0; i < digits.size(); ++i) {
    if (i > 0 && (digits.size() - i) % 3 == 0) grouped.push_back(',');
    grouped.push_back(digits[i]);
  }
  return allocated ? "≈" + grouped : grouped;
}

std::optional<ParsedTree> parseTree(const nlohmann::json& result) {
  if (!result.is_object()) return std::nullopt;
  ParsedTree tree;
  if (const auto root = result.find("root"); root != result.end() && root->is_string()) tree.root = root->get<std::string>();
  if (!readBool(result, "available", tree.available)) return std::nullopt;
  const auto nodes = result.find("nodes");
  if (nodes == result.end() || !nodes->is_array()) return std::nullopt;
  for (const nlohmann::json& item : *nodes) {
    if (!item.is_object()) return std::nullopt;
    contracts::DiskTreeNode node;
    if (!readString(item, "path", node.path) || !readString(item, "name", node.name) || !readBool(item, "directory", node.directory) || !readString(item, "kind", node.kind) || !readInteger(item, "size", node.size)) {
      return std::nullopt;
    }
    tree.nodes.push_back(std::move(node));
  }
  return tree;
}

std::optional<ParsedList> parseList(const nlohmann::json& result) {
  if (!result.is_object()) return std::nullopt;
  ParsedList list;
  const auto system = result.find("fileSystem");
  if (system == result.end() || !system->is_object()) return std::nullopt;
  if (!readString(*system, "format", list.format) || !readString(*system, "formatName", list.formatName) || !readBool(*system, "writable", list.writable) || !readBool(*system, "subdirectories", list.subdirectories)) return std::nullopt;
  if (const auto drive = result.find("drive"); drive != result.end() && drive->is_number_integer()) list.drive = static_cast<int>(drive->get<std::int64_t>());
  if (const auto reason = result.find("readOnlyReason"); reason != result.end() && reason->is_string()) list.readOnlyReason = reason->get<std::string>();
  std::uint64_t depth = 0;
  if (!readInteger(result, "undoDepth", depth)) return std::nullopt;
  list.undoDepth = static_cast<int>(depth);
  const auto entries = result.find("entries");
  if (entries == result.end() || !entries->is_array()) return std::nullopt;
  for (const nlohmann::json& item : *entries) {
    if (!item.is_object()) return std::nullopt;
    contracts::DiskEntryRow row;
    std::uint64_t size = 0;
    bool allocated = false;
    if (!readString(item, "name", row.name) || !readBool(item, "directory", row.directory) || !readString(item, "type", row.type) || !readInteger(item, "size", size) || !readBool(item, "sizeIsAllocated", allocated) ||
        !readString(item, "modified", row.modified) || !readBool(item, "locked", row.locked)) {
      return std::nullopt;
    }
    if (!row.directory) row.size = entrySizeText(size, allocated);
    list.entries.push_back(std::move(row));
  }
  return list;
}

std::optional<std::vector<contracts::DiskBlankKind>> parseBlankKinds(const nlohmann::json& result) {
  if (!result.is_object()) return std::nullopt;
  const auto kinds = result.find("kinds");
  if (kinds == result.end() || !kinds->is_array()) return std::nullopt;
  std::vector<contracts::DiskBlankKind> parsed;
  for (const nlohmann::json& item : *kinds) {
    contracts::DiskBlankKind kind;
    if (!item.is_object() || !readString(item, "id", kind.id) || !readString(item, "title", kind.title)) return std::nullopt;
    parsed.push_back(std::move(kind));
  }
  return parsed;
}

std::optional<std::vector<contracts::DiskExportItem>> parseExport(const nlohmann::json& result) {
  if (!result.is_object()) return std::nullopt;
  const auto items = result.find("items");
  if (items == result.end() || !items->is_array()) return std::nullopt;
  std::vector<contracts::DiskExportItem> parsed;
  for (const nlohmann::json& item : *items) {
    if (!item.is_object()) return std::nullopt;
    contracts::DiskExportItem exported;
    std::string content;
    const auto path = item.find("path");
    if (path == item.end() || !path->is_array() || path->empty() || !readBool(item, "directory", exported.directory) || !readString(item, "content", content)) return std::nullopt;
    for (const nlohmann::json& part : *path) {
      if (!part.is_string()) return std::nullopt;
      if (!exported.path.empty()) exported.path.push_back('/');
      exported.path += part.get<std::string>();
    }
    if (!exported.directory) {
      auto decoded = shared::base64Decode(content);
      if (!decoded) return std::nullopt;
      exported.content = std::move(*decoded);
    }
    parsed.push_back(std::move(exported));
  }
  return parsed;
}

std::string diskErrorText(const std::string& code, const std::string& message) { return message.empty() ? code : message; }

std::string uniqueFolderName(const std::vector<contracts::DiskEntryRow>& entries) {
  const auto taken = [&entries](const std::string& name) {
    return std::any_of(entries.begin(), entries.end(), [&name](const contracts::DiskEntryRow& row) { return upper(row.name) == name; });
  };
  std::string name = kFolderBaseName;
  for (int number = 1; taken(name) && number <= kMaxFolderNumber; ++number) name = kFolderBaseName + std::to_string(number);
  return name;
}

}  // namespace ember::win32::logic
