/*
 * ------------------------------------------------------------------------
 * 名称：update-manifest.hpp
 * 说明：升级清单（latest.json）的规则：读取与检查清单的各项、清单里被签名的那段文字、版本号的比较、base64 解码。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-21
 * 备注：清单是服务器上的普通文件，一律不可信：版本号必须是“主.次.修订”三段数字、文件名必须是 Ember-<版本号>.exe 且与版本号一致、大小有上限、SHA-256 必须是 64 位十六进制、签名必须是合法的 base64；
 *       被签名的文字是“ember-update”、版本号、文件名、大小、SHA-256 五行（换行分隔、结尾没有换行），所以改动其中任何一项签名都对不上；发布脚本 tools/release/publish-update.mjs 用同样的规则生成。
 * ------------------------------------------------------------------------
 */

#pragma once

#include <array>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace ember::win32::shared {

/** 新版本程序文件的大小上限（字节）。 */
inline constexpr std::uint64_t kMaxUpdateFileBytes = 128ull * 1024 * 1024;
/** 清单文件的大小上限（字节）。 */
inline constexpr std::size_t kMaxManifestBytes = 16 * 1024;

/** 升级清单。 */
struct UpdateManifest {
  /** 新版本的版本号，主.次.修订。 */
  std::string version;
  /** 程序文件名，Ember-<版本号>.exe。 */
  std::string file;
  /** 程序文件的字节数。 */
  std::uint64_t size{0};
  /** 程序文件的 SHA-256，小写十六进制。 */
  std::string sha256;
  /** 对被签名文字的 RSA 签名（PKCS#1 v1.5，SHA-256）。 */
  std::vector<std::uint8_t> signature;
};

/** 读取并检查清单；任何一项不合格都没有值。 */
[[nodiscard]] std::optional<UpdateManifest> parseUpdateManifest(std::string_view text);

/** 清单里被签名的那段文字。 */
[[nodiscard]] std::string updateSignedMessage(const UpdateManifest& manifest);

/** 读出版本号的三段数字；格式不对时没有值。 */
[[nodiscard]] std::optional<std::array<std::uint32_t, 3>> parseVersion(std::string_view text);

/** candidate 是否比 current 新；任何一个不是合格的版本号时为 false。 */
[[nodiscard]] bool isNewerVersion(std::string_view candidate, std::string_view current);

/** base64 解码（标准字母表，必须带正确的补位）；不合格时没有值。 */
[[nodiscard]] std::optional<std::vector<std::uint8_t>> base64Decode(std::string_view text);

/** 小写十六进制文字。 */
[[nodiscard]] std::string toHex(const std::vector<std::uint8_t>& bytes);

}  // namespace ember::win32::shared
