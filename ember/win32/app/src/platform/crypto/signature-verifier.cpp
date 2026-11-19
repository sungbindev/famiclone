/*
 * ------------------------------------------------------------------------
 * 名称：signature-verifier.cpp
 * 说明：校验升级用的密码学的实现。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-21
 * 备注：无。
 * ------------------------------------------------------------------------
 */

#include "platform/crypto/signature-verifier.hpp"

#include <windows.h>
#include <bcrypt.h>

#include <algorithm>
#include <fstream>

#include "shared/update/update-manifest.hpp"

namespace ember::win32::platform {

namespace {

constexpr std::size_t kSha256Bytes = 32;
constexpr std::size_t kReadChunk = 1 << 20;

/** SHA-256 的流式计算。 */
class Sha256 {
 public:
  Sha256() {
    if (!BCRYPT_SUCCESS(BCryptOpenAlgorithmProvider(&algorithm_, BCRYPT_SHA256_ALGORITHM, nullptr, 0))) return;
    if (!BCRYPT_SUCCESS(BCryptCreateHash(algorithm_, &hash_, nullptr, 0, nullptr, 0, 0))) hash_ = nullptr;
  }
  ~Sha256() {
    if (hash_ != nullptr) BCryptDestroyHash(hash_);
    if (algorithm_ != nullptr) BCryptCloseAlgorithmProvider(algorithm_, 0);
  }
  Sha256(const Sha256&) = delete;
  Sha256& operator=(const Sha256&) = delete;

  [[nodiscard]] bool ok() const { return hash_ != nullptr; }
  bool update(const std::uint8_t* data, std::size_t size) {
    return BCRYPT_SUCCESS(BCryptHashData(hash_, const_cast<PUCHAR>(data), static_cast<ULONG>(size), 0));
  }
  /** 结束并取出摘要；失败时为空。 */
  std::vector<std::uint8_t> finish() {
    std::vector<std::uint8_t> digest(kSha256Bytes);
    if (!BCRYPT_SUCCESS(BCryptFinishHash(hash_, digest.data(), static_cast<ULONG>(digest.size()), 0))) return {};
    return digest;
  }

 private:
  BCRYPT_ALG_HANDLE algorithm_{nullptr};
  BCRYPT_HASH_HANDLE hash_{nullptr};
};

}  // namespace

std::string sha256Hex(const std::uint8_t* data, std::size_t size) {
  Sha256 hasher;
  if (!hasher.ok()) return {};
  // 一次最多交给系统 1 MB，避免长度超过 ULONG
  for (std::size_t offset = 0; offset < size; offset += kReadChunk) {
    if (!hasher.update(data + offset, std::min(kReadChunk, size - offset))) return {};
  }
  const auto digest = hasher.finish();
  return digest.empty() ? std::string{} : shared::toHex(digest);
}

std::optional<FileDigest> digestFile(const std::filesystem::path& file) {
  std::ifstream stream(file, std::ios::binary);
  if (!stream) return std::nullopt;
  Sha256 hasher;
  if (!hasher.ok()) return std::nullopt;
  std::vector<char> buffer(kReadChunk);
  std::uint64_t total = 0;
  while (stream) {
    stream.read(buffer.data(), static_cast<std::streamsize>(buffer.size()));
    const std::streamsize got = stream.gcount();
    if (got > 0) {
      if (!hasher.update(reinterpret_cast<const std::uint8_t*>(buffer.data()), static_cast<std::size_t>(got))) return std::nullopt;
      total += static_cast<std::uint64_t>(got);
    }
  }
  if (!stream.eof()) return std::nullopt;
  const auto digest = hasher.finish();
  if (digest.empty()) return std::nullopt;
  return FileDigest{shared::toHex(digest), total};
}

bool verifyRsaSha256(const std::vector<std::uint8_t>& modulus, const std::vector<std::uint8_t>& exponent, const std::string& message, const std::vector<std::uint8_t>& signature) {
  if (modulus.empty() || exponent.empty() || signature.size() != modulus.size()) return false;

  Sha256 hasher;
  if (!hasher.ok() || !hasher.update(reinterpret_cast<const std::uint8_t*>(message.data()), message.size())) return false;
  const auto digest = hasher.finish();
  if (digest.size() != kSha256Bytes) return false;

  // BCRYPT_RSAPUBLIC_BLOB：头、公钥指数、模数（都是大端）
  std::vector<std::uint8_t> blob(sizeof(BCRYPT_RSAKEY_BLOB) + exponent.size() + modulus.size());
  auto* header = reinterpret_cast<BCRYPT_RSAKEY_BLOB*>(blob.data());
  header->Magic = BCRYPT_RSAPUBLIC_MAGIC;
  header->BitLength = static_cast<ULONG>(modulus.size() * 8);
  header->cbPublicExp = static_cast<ULONG>(exponent.size());
  header->cbModulus = static_cast<ULONG>(modulus.size());
  header->cbPrime1 = 0;
  header->cbPrime2 = 0;
  std::copy(exponent.begin(), exponent.end(), blob.begin() + static_cast<std::ptrdiff_t>(sizeof(BCRYPT_RSAKEY_BLOB)));
  std::copy(modulus.begin(), modulus.end(), blob.begin() + static_cast<std::ptrdiff_t>(sizeof(BCRYPT_RSAKEY_BLOB) + exponent.size()));

  BCRYPT_ALG_HANDLE algorithm = nullptr;
  if (!BCRYPT_SUCCESS(BCryptOpenAlgorithmProvider(&algorithm, BCRYPT_RSA_ALGORITHM, nullptr, 0))) return false;
  BCRYPT_KEY_HANDLE key = nullptr;
  bool ok = false;
  if (BCRYPT_SUCCESS(BCryptImportKeyPair(algorithm, nullptr, BCRYPT_RSAPUBLIC_BLOB, &key, blob.data(), static_cast<ULONG>(blob.size()), 0))) {
    BCRYPT_PKCS1_PADDING_INFO padding{BCRYPT_SHA256_ALGORITHM};
    ok = BCRYPT_SUCCESS(BCryptVerifySignature(key, &padding, const_cast<PUCHAR>(digest.data()), static_cast<ULONG>(digest.size()), const_cast<PUCHAR>(signature.data()), static_cast<ULONG>(signature.size()), BCRYPT_PAD_PKCS1));
    BCryptDestroyKey(key);
  }
  BCryptCloseAlgorithmProvider(algorithm, 0);
  return ok;
}

}  // namespace ember::win32::platform
