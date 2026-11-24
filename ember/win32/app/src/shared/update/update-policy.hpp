/*
 * ------------------------------------------------------------------------
 * 名称：update-policy.hpp
 * 说明：应用升级在本次运行里是否开启：发布构建、不是调试会话、程序所在目录能写，三项都满足才开启。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-21
 * 备注：调试会话指启动参数带 --debug-port 的实例（如 FC Dev 扩展的“调试运行”启动的）：它用临时的用户数据目录，运行中不能自行下载新版本并重启，所以升级状态固定是不支持。
 * ------------------------------------------------------------------------
 */

#pragma once

namespace ember::win32::shared {

/**
 * 本次运行是否开启应用升级。
 * @param releaseBuild 是否发布构建（编译时开启了升级）。
 * @param debugSession 是否调试会话（启动参数带 --debug-port）。
 * @param directoryWritable 程序所在目录能否创建文件（新程序要覆盖当前程序）。
 * @return 三项都满足时为真。
 */
[[nodiscard]] constexpr bool updatesSupported(bool releaseBuild, bool debugSession, bool directoryWritable) {
  return releaseBuild && !debugSession && directoryWritable;
}

}  // namespace ember::win32::shared
