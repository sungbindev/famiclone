/*
 * ------------------------------------------------------------------------
 * 名称：update-manifest.cpp
 * 说明：升级清单规则的实现。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-21
 * 备注：无。
 * ------------------------------------------------------------------------
 */

#include "shared/update/update-manifest.hpp"

#include <algorithm>

#include <nlohmann/json.hpp>

namespace ember::win32::shared {

namespace {

constexpr std::string_view kMessageHeader = "ember-update";
constexpr std::string_view kFilePrefix = "Ember-";
constexpr std::string_view kFileSuffix = ".exe";
constexpr std::size_t kMaxSignatureBytes = 1024;

bool isDigits(std::string_view text) { return !text.empty() && text.size() <= 9 && std::all_of(text.begin(), text.end(), [](char c) { return c >= '0' && c <= '9'; }); }

bool isLowerHex64(const std::string& text) {
  return text.size() == 64 && std::all_of(text.begin(), text.end(), [](char c) { return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f'); });
}

}  // namespace

std::optional<std::array<std::uint32_t, 3>> parseVersion(std::string_view text) {
  std::array<std::uint32_t, 3> parts{};
  std::size_t start = 0;
  for (std::size_t index = 0; index < 3; ++index) {
    const std::size_t end = index == 2 ? text.size() : text.find('.', start);
    if (end == std::string_view::npos || end < start) return std::nullopt;
    const std::string_view part = text.substr(start, end - start);
    if (!isDigits(part) || (part.size() > 1 && part[0] == '0')) return std::nullopt;
    parts[index] = static_cast<std::uint32_t>(std::stoul(std::string(part)));
    start = end + 1;
  }
  return parts;
}

bool isNewerVersion(std::string_view candidate, std::string_view current) {
  const auto a = parseVersion(candidate);
  const auto b = parseVersion(current);
  return a && b && *a > *b;
}

std::optional<std::vector<std::uint8_t>> base64Decode(std::string_view text) {
  if (text.empty() || text.size() % 4 != 0) return std::nullopt;
  std::vector<std::uint8_t> result;
  result.reserve(text.size() / 4 * 3);
  const auto value = [](char c) -> int {
    if (c >= 'A' && c <= 'Z') return c - 'A';
    if (c >= 'a' && c <= 'z') return c - 'a' + 26;
    if (c >= '0' && c <= '9') return c - '0' + 52;
    if (c == '+') return 62;
    if (c == '/') return 63;
    return -1;
  };
  for (std::size_t i = 0; i < text.size(); i += 4) {
    const bool last = i + 4 == text.size();
    int padding = 0;
    int chunk[4];
    for (int k = 0; k < 4; ++k) {
      const char c = text[i + static_cast<std::size_t>(k)];
      if (c == '=') {
        // 补位只能出现在最后一组的末尾
        if (!last || k < 2) return std::nullopt;
        ++padding;
        chunk[k] = 0;
      } else {
        if (padding > 0) return std::nullopt;
        chunk[k] = value(c);
        if (chunk[k] < 0) return std::nullopt;
      }
    }
    const std::uint32_t triple = (static_cast<std::uint32_t>(chunk[0]) << 18) | (static_cast<std::uint32_t>(chunk[1]) << 12) | (static_cast<std::uint32_t>(chunk[2]) << 6) | static_cast<std::uint32_t>(chunk[3]);
    result.push_back(static_cast<std::uint8_t>(triple >> 16));
    if (padding < 2) result.push_back(static_cast<std::uint8_t>(triple >> 8));
    if (padding < 1) result.push_back(static_cast<std::uint8_t>(triple));
  }
  return result;
}

std::string toHex(const std::vector<std::uint8_t>& bytes) {
  static constexpr char kDigits[] = "0123456789abcdef";
  std::string result;
  result.reserve(bytes.size() * 2);
  for (const std::uint8_t byte : bytes) {
    result += kDigits[byte >> 4];
    result += kDigits[byte & 0x0F];
  }
  return result;
}

std::optional<UpdateManifest> parseUpdateManifest(std::string_view text) {
  if (text.empty() || text.size() > kMaxManifestBytes) return std::nullopt;
  const nlohmann::json json = nlohmann::json::parse(text.begin(), text.end(), nullptr, false);
  if (!json.is_object()) return std::nullopt;
  const auto field = [&json](const char* name) -> const nlohmann::json* {
    const auto found = json.find(name);
    return found == json.end() ? nullptr : &*found;
  };
  const auto* version = field("version");
  const auto* file = field("file");
  const auto* size = field("size");
  const auto* sha256 = field("sha256");
  const auto* signature = field("signature");
  if (version == nullptr || !version->is_string() || file == nullptr || !file->is_string() || size == nullptr || !size->is_number_unsigned() || sha256 == nullptr || !sha256->is_string() || signature == nullptr ||
      !signature->is_string()) {
    return std::nullopt;
  }
  UpdateManifest manifest;
  manifest.version = version->get<std::string>();
  manifest.file = file->get<std::string>();
  manifest.size = size->get<std::uint64_t>();
  manifest.sha256 = sha256->get<std::string>();
  if (!parseVersion(manifest.version)) return std::nullopt;
  if (manifest.file != std::string(kFilePrefix) + manifest.version + std::string(kFileSuffix)) return std::nullopt;
  if (manifest.size == 0 || manifest.size > kMaxUpdateFileBytes) return std::nullopt;
  if (!isLowerHex64(manifest.sha256)) return std::nullopt;
  auto decoded = base64Decode(signature->get<std::string>());
  if (!decoded || decoded->empty() || decoded->size() > kMaxSignatureBytes) return std::nullopt;
  manifest.signature = std::move(*decoded);
  return manifest;
}

std::string updateSignedMessage(const UpdateManifest& manifest) {
  return std::string(kMessageHeader) + "\n" + manifest.version + "\n" + manifest.file + "\n" + std::to_string(manifest.size) + "\n" + manifest.sha256;
}

}  // namespace ember::win32::shared
