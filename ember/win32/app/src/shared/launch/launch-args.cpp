/*
 * ------------------------------------------------------------------------
 * 名称：launch-args.cpp
 * 说明：启动参数的解析。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-19
 * 备注：无。
 * ------------------------------------------------------------------------
 */

#include "shared/launch/launch-args.hpp"

#include <algorithm>
#include <cctype>
#include <string_view>

namespace ember::win32::shared {

namespace {

constexpr std::string_view kCartridgePrefix = "--cartridge=";
constexpr std::string_view kCartridgeExtension = ".nes";
constexpr std::string_view kDebugPortName = "--debug-port";
constexpr std::string_view kDebugTokenName = "--debug-token";
constexpr std::string_view kUserDataName = "--user-data-dir";
constexpr std::string_view kMoveUserDataName = "--move-user-data";
constexpr std::string_view kWaitPidName = "--wait-pid";
constexpr std::string_view kApplyUpdateName = "--apply-update";
constexpr std::size_t kMaxTokenLength = 128;
constexpr std::size_t kMaxPathLength = 4096;

/** 取“--名称=值”或“--名称 值”里的值；没有该参数时为空。 */
std::optional<std::string> argumentValue(const std::vector<std::string>& arguments, std::string_view name) {
  for (std::size_t index = 0; index < arguments.size(); ++index) {
    const std::string& argument = arguments[index];
    if (argument.size() > name.size() && argument.compare(0, name.size(), name) == 0 && argument[name.size()] == '=') return argument.substr(name.size() + 1);
    if (argument == name) return index + 1 < arguments.size() ? std::optional<std::string>(arguments[index + 1]) : std::nullopt;
  }
  return std::nullopt;
}

/** 文字以指定扩展名结尾（扩展名只含 ASCII，不分大小写）。 */
bool endsWithIgnoreCase(const std::string& text, std::string_view suffix) {
  if (text.size() < suffix.size()) return false;
  return std::equal(suffix.begin(), suffix.end(), text.end() - static_cast<std::ptrdiff_t>(suffix.size()), [](char expected, char actual) {
    return std::tolower(static_cast<unsigned char>(actual)) == expected;
  });
}

/** 合格的端口文字：1 到 5 位数字，值在 1 到 65535 之间。 */
std::optional<int> parsePort(const std::string& text) {
  if (text.empty() || text.size() > 5 || !std::all_of(text.begin(), text.end(), [](char c) { return c >= '0' && c <= '9'; })) return std::nullopt;
  const int port = std::stoi(text);
  return port >= 1 && port <= 65535 ? std::optional<int>(port) : std::nullopt;
}

/** 合格的进程号：1 到 10 位数字，值在 1 到 4294967295 之间。 */
std::optional<std::uint32_t> parseProcessId(const std::string& text) {
  if (text.empty() || text.size() > 10 || !std::all_of(text.begin(), text.end(), [](char c) { return c >= '0' && c <= '9'; })) return std::nullopt;
  const unsigned long long value = std::stoull(text);
  return value >= 1 && value <= 0xFFFFFFFFull ? std::optional<std::uint32_t>(static_cast<std::uint32_t>(value)) : std::nullopt;
}

/** 合格的令牌：1 到 128 个可见的 ASCII 字符。 */
bool isValidToken(const std::string& text) {
  return !text.empty() && text.size() <= kMaxTokenLength && std::all_of(text.begin(), text.end(), [](char c) { return c >= 0x21 && c <= 0x7e; });
}

/** 带值的参数名（“--名称 值”的写法里，值是下一个参数）。 */
bool takesValue(const std::string& name) {
  return name == kDebugPortName || name == kDebugTokenName || name == kUserDataName || name == kMoveUserDataName || name == kWaitPidName || name == kApplyUpdateName;
}

/** 直接写在命令行里的卡带路径（双击文件时系统这样传）：第一个不是选项也不是选项的值、扩展名是 .nes 的参数。 */
std::optional<std::string> positionalCartridge(const std::vector<std::string>& arguments) {
  for (std::size_t index = 0; index < arguments.size(); ++index) {
    const std::string& argument = arguments[index];
    if (argument.compare(0, 2, "--") == 0) {
      if (argument.find('=') == std::string::npos && takesValue(argument)) ++index;
      continue;
    }
    if (argument.size() <= kMaxPathLength && endsWithIgnoreCase(argument, kCartridgeExtension)) return argument;
  }
  return std::nullopt;
}

}  // namespace

LaunchArguments parseLaunchArguments(const std::vector<std::string>& arguments) {
  LaunchArguments result;
  for (const std::string& argument : arguments) {
    if (argument.compare(0, kCartridgePrefix.size(), kCartridgePrefix) != 0) continue;
    const std::string path = argument.substr(kCartridgePrefix.size());
    if (endsWithIgnoreCase(path, kCartridgeExtension)) result.cartridgePath = path;
    break;
  }
  if (!result.cartridgePath) result.cartridgePath = positionalCartridge(arguments);
  if (const auto text = argumentValue(arguments, kDebugPortName)) result.debugPort = parsePort(*text);
  if (const auto text = argumentValue(arguments, kDebugTokenName); text && isValidToken(*text)) result.debugToken = *text;
  if (const auto text = argumentValue(arguments, kUserDataName); text && !text->empty() && text->size() <= kMaxPathLength) result.userDataDirectory = *text;
  if (const auto text = argumentValue(arguments, kMoveUserDataName)) result.moveUserData = parseUserDataPlace(*text);
  if (const auto text = argumentValue(arguments, kWaitPidName)) result.waitProcessId = parseProcessId(*text);
  if (const auto text = argumentValue(arguments, kApplyUpdateName); text && !text->empty() && text->size() <= kMaxPathLength) result.applyUpdateTarget = *text;
  return result;
}

}  // namespace ember::win32::shared
