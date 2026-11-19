/*
 * ------------------------------------------------------------------------
 * 名称：help-request.cpp
 * 说明：帮助文档站点请求规则的实现。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-21
 * 备注：无。
 * ------------------------------------------------------------------------
 */

#include "shared/help/help-request.hpp"

#include <map>

namespace ember::win32::shared {

namespace {

/** 页面里的内联脚本与样式由站点生成器输出（暗色主题的初始化等），所以放行 unsafe-inline；其余一律只允许本站。 */
constexpr const char* kContentSecurityPolicy =
    "default-src 'self'; script-src 'self' 'unsafe-inline'; style-src 'self' 'unsafe-inline'; img-src 'self' data:; font-src 'self' data:; connect-src 'self'; object-src 'none'; base-uri 'self'; frame-ancestors 'none'";

int hexValue(char c) {
  if (c >= '0' && c <= '9') return c - '0';
  if (c >= 'a' && c <= 'f') return c - 'a' + 10;
  if (c >= 'A' && c <= 'F') return c - 'A' + 10;
  return -1;
}

/** 百分号解码；格式不对时没有值。 */
std::optional<std::string> percentDecode(std::string_view text) {
  std::string result;
  for (std::size_t i = 0; i < text.size(); ++i) {
    if (text[i] != '%') {
      result += text[i];
      continue;
    }
    if (i + 2 >= text.size()) return std::nullopt;
    const int high = hexValue(text[i + 1]);
    const int low = hexValue(text[i + 2]);
    if (high < 0 || low < 0) return std::nullopt;
    result += static_cast<char>(high * 16 + low);
    i += 2;
  }
  return result;
}

/** 解码后是不是合法的 UTF-8（站点文件名都是合法的 UTF-8，不合法的一律不认）。 */
bool isValidUtf8(const std::string& text) {
  std::size_t i = 0;
  while (i < text.size()) {
    const unsigned char c = static_cast<unsigned char>(text[i]);
    std::size_t length = 0;
    if (c < 0x80) length = 1;
    else if (c >= 0xC2 && c <= 0xDF) length = 2;
    else if (c >= 0xE0 && c <= 0xEF) length = 3;
    else if (c >= 0xF0 && c <= 0xF4) length = 4;
    else return false;
    if (i + length > text.size()) return false;
    for (std::size_t k = 1; k < length; ++k) {
      if ((static_cast<unsigned char>(text[i + k]) & 0xC0) != 0x80) return false;
    }
    i += length;
  }
  return true;
}

}  // namespace

std::optional<std::string> helpPagePath(std::string_view page) {
  if (page == "changelog") return std::string("manual/changelog.html");
  return std::nullopt;
}

std::optional<std::string> normalizeHelpPath(std::string_view target) {
  if (target.empty() || target[0] != '/') return std::nullopt;
  // 去掉查询与片段
  const std::size_t cut = target.find_first_of("?#");
  const std::string_view path = cut == std::string_view::npos ? target : target.substr(0, cut);
  // 编码的斜杠与反斜杠不接受
  for (std::size_t i = 0; i + 2 < path.size(); ++i) {
    if (path[i] == '%' && (path.compare(i + 1, 2, "2f") == 0 || path.compare(i + 1, 2, "2F") == 0 || path.compare(i + 1, 2, "5c") == 0 || path.compare(i + 1, 2, "5C") == 0)) return std::nullopt;
  }
  const auto decoded = percentDecode(path);
  if (!decoded || !isValidUtf8(*decoded)) return std::nullopt;
  for (const char c : *decoded) {
    if (c == '\0' || c == '\\' || c == ':') return std::nullopt;
  }
  const bool directory = decoded->back() == '/';
  std::vector<std::string> segments;
  std::size_t start = 0;
  while (start <= decoded->size()) {
    const std::size_t end = decoded->find('/', start);
    const std::string segment = decoded->substr(start, end == std::string::npos ? std::string::npos : end - start);
    if (segment == "." || segment == "..") return std::nullopt;
    if (!segment.empty()) segments.push_back(segment);
    if (end == std::string::npos) break;
    start = end + 1;
  }
  if (directory) segments.push_back("index.html");
  std::string result;
  for (std::size_t i = 0; i < segments.size(); ++i) result += (i == 0 ? "" : "/") + segments[i];
  return result;
}

std::string helpContentType(std::string_view path) {
  static const std::map<std::string, std::string, std::less<>> kTypes{
      {".html", "text/html; charset=utf-8"}, {".js", "text/javascript; charset=utf-8"}, {".mjs", "text/javascript; charset=utf-8"}, {".css", "text/css; charset=utf-8"}, {".json", "application/json; charset=utf-8"},
      {".txt", "text/plain; charset=utf-8"}, {".svg", "image/svg+xml"}, {".png", "image/png"}, {".ico", "image/x-icon"}, {".woff", "font/woff"}, {".woff2", "font/woff2"}};
  const std::size_t dot = path.find_last_of('.');
  const std::size_t slash = path.find_last_of('/');
  if (dot == std::string_view::npos || (slash != std::string_view::npos && dot < slash)) return "application/octet-stream";
  std::string extension(path.substr(dot));
  for (char& c : extension) {
    if (c >= 'A' && c <= 'Z') c = static_cast<char>(c - 'A' + 'a');
  }
  const auto found = kTypes.find(extension);
  return found == kTypes.end() ? "application/octet-stream" : found->second;
}

std::vector<std::pair<std::string, std::string>> helpSecurityHeaders() { return {{"X-Content-Type-Options", "nosniff"}, {"Content-Security-Policy", kContentSecurityPolicy}}; }

}  // namespace ember::win32::shared
