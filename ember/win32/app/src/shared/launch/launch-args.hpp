/*
 * ------------------------------------------------------------------------
 * 名称：launch-args.hpp
 * 说明：命令行启动参数：卡带路径（--cartridge=路径）、调试服务的端口与令牌（--debug-port、--debug-token）、用户数据目录（--user-data-dir）。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-19
 * 备注：规则与 Electron 外壳的 src/platform/launch/launch-args.ts 相同：卡带只接受 .nes 文件；端口是 1 到 65535 的整数；令牌是 1 到 128 个可见的 ASCII 字符；
 *       端口与令牌既可以写成“--名称=值”，也可以写成“--名称 值”；不合格的参数一律忽略；参数都是 UTF-8 文本，不含程序名。
 * ------------------------------------------------------------------------
 */

#pragma once

#include <optional>
#include <string>
#include <vector>

namespace ember::win32::shared {

/** 从命令行取出的启动参数。 */
struct LaunchArguments {
  /** 要打开的卡带路径。 */
  std::optional<std::string> cartridgePath;
  /** 调试服务的端口：给出时本次运行强制开启调试服务。 */
  std::optional<int> debugPort;
  /** 调试服务的令牌。 */
  std::optional<std::string> debugToken;
  /** 用户数据目录：不给时用默认位置。 */
  std::optional<std::string> userDataDirectory;
};

/**
 * 解析启动参数。
 * @param arguments 命令行参数（不含程序名），UTF-8。
 * @returns 合格的参数；没有或不合格的项为空。
 */
[[nodiscard]] LaunchArguments parseLaunchArguments(const std::vector<std::string>& arguments);

}  // namespace ember::win32::shared
