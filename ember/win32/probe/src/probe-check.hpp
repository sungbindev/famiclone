/*
 * ------------------------------------------------------------------------
 * 名称：probe-check.hpp
 * 说明：平台探针的检查项：每项检查系统上的一种能力，结果是通过、提示或失败。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-19
 * 备注：探针用于确认原生外壳依赖的系统能力在目标系统上可用，尤其是刚装好的 Windows 7 SP1；检查项只读系统状态，不改动系统。
 * ------------------------------------------------------------------------
 */

#pragma once

#include <string>
#include <vector>

#include <windows.h>

namespace ember::win32::probe {

/** 检查结果的级别。 */
enum class CheckLevel {
  /** 能力可用。 */
  Pass,
  /** 能力不可用或有限制，但不影响运行（如没有声音设备）。 */
  Info,
  /** 能力不可用，原生外壳无法运行。 */
  Fail
};

/** 一项检查的结果。 */
struct CheckResult {
  /** 检查项的名称。 */
  std::string name;
  /** 结果级别。 */
  CheckLevel level;
  /** 说明，写明得到的版本、设备或失败原因（UTF-8）。 */
  std::string detail;
};

/** 全部检查的结果。 */
using CheckResults = std::vector<CheckResult>;

/** 记录系统版本，并检查是 64 位进程。 */
void checkSystem(CheckResults& results);

/** 检查 Direct2D 1.0 的窗口渲染目标与 DirectWrite 1.0 及界面用到的字体。 */
void checkDirect2D(CheckResults& results, HWND window);

/** 检查 Direct3D 11 的硬件设备、DXGI 1.0 交换链与动态纹理上传。 */
void checkDirect3D(CheckResults& results, HWND window);

/** 检查 XInput 9.1.0 能否动态加载并读取手柄状态。 */
void checkXInput(CheckResults& results);

/** 检查 WinHTTP 能否创建会话，以及是否接受 TLS 1.2。 */
void checkWinHttp(CheckResults& results);

/** 检查系统声音设备能否经 miniaudio 打开。 */
void checkAudio(CheckResults& results);

/** 检查核心能否启动、响应请求并停止。 */
void checkCore(CheckResults& results);

/** 创建一个不显示的窗口，供图形检查使用。 */
HWND createProbeWindow(unsigned width, unsigned height);

/** 把宽字符串转成 UTF-8。 */
std::string toUtf8(const std::wstring& text);

/** 把 HRESULT 格式化成“0x8007007E”这样的文字。 */
std::string formatHresult(HRESULT value);

}  // namespace ember::win32::probe
