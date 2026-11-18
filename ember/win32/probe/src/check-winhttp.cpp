/*
 * ------------------------------------------------------------------------
 * 名称：check-winhttp.cpp
 * 说明：WinHTTP 检查：创建会话，并试着启用 TLS 1.2。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-19
 * 备注：刚装好的 Windows 7 SP1 默认不启用 TLS 1.2（需要补丁 KB3140245），这时只给出提示，升级下载会改用有签名校验的备用通道。
 * ------------------------------------------------------------------------
 */

#include "probe-check.hpp"

#include <winhttp.h>

namespace ember::win32::probe {

namespace {

/** TLS 1.2 的协议标志；旧版 SDK 头文件里没有这个常量，用数值。 */
constexpr DWORD TLS_1_2_FLAG = 0x00000800;

}  // namespace

void checkWinHttp(CheckResults& results) {
  const HINTERNET session = WinHttpOpen(L"EmberProbe/1.0", WINHTTP_ACCESS_TYPE_NO_PROXY, WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
  if (session == nullptr) {
    results.push_back({"WinHTTP", CheckLevel::Fail, "创建会话失败，错误 " + std::to_string(GetLastError())});
    return;
  }
  results.push_back({"WinHTTP", CheckLevel::Pass, "可以创建会话"});
  DWORD protocols = TLS_1_2_FLAG;
  const BOOL accepted = WinHttpSetOption(session, WINHTTP_OPTION_SECURE_PROTOCOLS, &protocols, sizeof(protocols));
  results.push_back({"WinHTTP TLS 1.2", accepted != FALSE ? CheckLevel::Pass : CheckLevel::Info, accepted != FALSE ? "系统接受启用 TLS 1.2" : "系统不支持启用 TLS 1.2，升级下载需要备用通道"});
  WinHttpCloseHandle(session);
}

}  // namespace ember::win32::probe
