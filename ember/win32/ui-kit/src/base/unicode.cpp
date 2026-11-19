/*
 * ------------------------------------------------------------------------
 * 名称：unicode.cpp
 * 说明：UTF-8 与 UTF-16 互转的实现。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-19
 * 备注：UTF-8 解码拒绝过长编码、代理码点与超出 U+10FFFF 的值。
 * ------------------------------------------------------------------------
 */

#include "base/unicode.hpp"

namespace rujian::ui {

namespace {

constexpr char32_t kReplacement = 0xFFFD;

bool isHighSurrogate(char16_t unit) { return unit >= 0xD800 && unit <= 0xDBFF; }
bool isLowSurrogate(char16_t unit) { return unit >= 0xDC00 && unit <= 0xDFFF; }

void append(std::u16string& out, char32_t codePoint) {
  if (codePoint >= 0x10000) {
    const char32_t value = codePoint - 0x10000;
    out.push_back(static_cast<char16_t>(0xD800 + (value >> 10)));
    out.push_back(static_cast<char16_t>(0xDC00 + (value & 0x3FF)));
  } else {
    out.push_back(static_cast<char16_t>(codePoint));
  }
}

}  // namespace

std::u16string toUtf16(std::string_view utf8) {
  std::u16string out;
  out.reserve(utf8.size());
  std::size_t i = 0;
  while (i < utf8.size()) {
    const auto lead = static_cast<unsigned char>(utf8[i]);
    int extra = 0;
    char32_t codePoint = 0;
    char32_t minimum = 0;
    if (lead < 0x80) {
      codePoint = lead;
    } else if ((lead & 0xE0) == 0xC0) {
      extra = 1;
      codePoint = lead & 0x1F;
      minimum = 0x80;
    } else if ((lead & 0xF0) == 0xE0) {
      extra = 2;
      codePoint = lead & 0x0F;
      minimum = 0x800;
    } else if ((lead & 0xF8) == 0xF0) {
      extra = 3;
      codePoint = lead & 0x07;
      minimum = 0x10000;
    } else {
      append(out, kReplacement);
      ++i;
      continue;
    }
    // 后续字节必须都在字符串里
    bool valid = i + static_cast<std::size_t>(extra) < utf8.size();
    std::size_t used = 1;
    if (valid) {
      for (int k = 1; k <= extra; ++k) {
        const auto next = static_cast<unsigned char>(utf8[i + static_cast<std::size_t>(k)]);
        if ((next & 0xC0) != 0x80) {
          valid = false;
          break;
        }
        codePoint = (codePoint << 6) | (next & 0x3F);
        used = static_cast<std::size_t>(k) + 1;
      }
    }
    if (valid && (codePoint < minimum || codePoint > 0x10FFFF || (codePoint >= 0xD800 && codePoint <= 0xDFFF))) valid = false;
    if (!valid) {
      append(out, kReplacement);
      ++i;
      continue;
    }
    append(out, codePoint);
    i += used;
  }
  return out;
}

std::string toUtf8(std::u16string_view utf16) {
  std::string out;
  out.reserve(utf16.size() * 3);
  for (std::size_t i = 0; i < utf16.size();) {
    char32_t codePoint = utf16[i];
    std::size_t length = 1;
    if (isHighSurrogate(utf16[i]) && i + 1 < utf16.size() && isLowSurrogate(utf16[i + 1])) {
      codePoint = 0x10000 + ((static_cast<char32_t>(utf16[i]) - 0xD800) << 10) + (static_cast<char32_t>(utf16[i + 1]) - 0xDC00);
      length = 2;
    } else if (isHighSurrogate(utf16[i]) || isLowSurrogate(utf16[i])) {
      codePoint = kReplacement;
    }
    i += length;
    if (codePoint < 0x80) {
      out.push_back(static_cast<char>(codePoint));
    } else if (codePoint < 0x800) {
      out.push_back(static_cast<char>(0xC0 | (codePoint >> 6)));
      out.push_back(static_cast<char>(0x80 | (codePoint & 0x3F)));
    } else if (codePoint < 0x10000) {
      out.push_back(static_cast<char>(0xE0 | (codePoint >> 12)));
      out.push_back(static_cast<char>(0x80 | ((codePoint >> 6) & 0x3F)));
      out.push_back(static_cast<char>(0x80 | (codePoint & 0x3F)));
    } else {
      out.push_back(static_cast<char>(0xF0 | (codePoint >> 18)));
      out.push_back(static_cast<char>(0x80 | ((codePoint >> 12) & 0x3F)));
      out.push_back(static_cast<char>(0x80 | ((codePoint >> 6) & 0x3F)));
      out.push_back(static_cast<char>(0x80 | (codePoint & 0x3F)));
    }
  }
  return out;
}

std::u16string toUtf16(char32_t codePoint) {
  std::u16string out;
  const bool surrogate = codePoint >= 0xD800 && codePoint <= 0xDFFF;
  append(out, (codePoint > 0x10FFFF || surrogate) ? kReplacement : codePoint);
  return out;
}

char32_t codePointAt(std::u16string_view text, std::size_t index) {
  if (index >= text.size()) return 0;
  if (isHighSurrogate(text[index]) && index + 1 < text.size() && isLowSurrogate(text[index + 1])) {
    return 0x10000 + ((static_cast<char32_t>(text[index]) - 0xD800) << 10) + (static_cast<char32_t>(text[index + 1]) - 0xDC00);
  }
  return text[index];
}

std::size_t codePointLength(std::u16string_view text, std::size_t index) {
  return (index + 1 < text.size() && isHighSurrogate(text[index]) && isLowSurrogate(text[index + 1])) ? 2 : 1;
}

}  // namespace rujian::ui
