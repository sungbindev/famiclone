/*
 * ------------------------------------------------------------------------
 * 名称：signature-verifier.hpp
 * 说明：校验升级用的密码学：文件与内存数据的 SHA-256，用写在程序里的 RSA 公钥校验签名（PKCS#1 v1.5，SHA-256）。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-21
 * 备注：用 Windows 自带的 CNG（bcrypt.dll，Windows 7 SP1 可用），不引入第三方库；公钥以大端的模数与指数字节给出。
 * ------------------------------------------------------------------------
 */

#pragma once

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>
#include <vector>

namespace ember::win32::platform {

/** 内存数据的 SHA-256（小写十六进制）；失败时为空文字。 */
[[nodiscard]] std::string sha256Hex(const std::uint8_t* data, std::size_t size);

/**
 * 文件的 SHA-256（小写十六进制）与字节数；打不开或读取失败时没有值。
 * @param file 文件路径。
 */
struct FileDigest {
  std::string sha256;
  std::uint64_t size{0};
};
[[nodiscard]] std::optional<FileDigest> digestFile(const std::filesystem::path& file);

/**
 * 用 RSA 公钥校验对 message 的签名（PKCS#1 v1.5，SHA-256）。
 * @param modulus 公钥模数（大端字节）。
 * @param exponent 公钥指数（大端字节）。
 * @param message 被签名的文字。
 * @param signature 签名（长度等于模数的字节数）。
 * @returns 签名正确时为 true；任何错误（密钥格式不对、签名长度不对、校验失败）都是 false。
 */
[[nodiscard]] bool verifyRsaSha256(const std::vector<std::uint8_t>& modulus, const std::vector<std::uint8_t>& exponent, const std::string& message, const std::vector<std::uint8_t>& signature);

}  // namespace ember::win32::platform
