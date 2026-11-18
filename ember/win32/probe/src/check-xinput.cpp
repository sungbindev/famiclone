/*
 * ------------------------------------------------------------------------
 * 名称：check-xinput.cpp
 * 说明：XInput 检查：动态加载 Windows 7 自带的 xinput9_1_0.dll，读取四个手柄位的状态。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-19
 * 备注：不使用需要另装 DirectX 运行库的 xinput1_3.dll；没有接手柄时系统返回“设备未连接”，这说明库本身可用。
 * ------------------------------------------------------------------------
 */

#include "probe-check.hpp"

#include <cstdio>

namespace ember::win32::probe {

namespace {

/** XInputGetState 的函数类型；状态结构固定为 16 字节，这里只关心返回值。 */
using XInputGetStateFunction = DWORD(WINAPI*)(DWORD, void*);

/** 手柄位的个数。 */
constexpr DWORD CONTROLLER_COUNT = 4;

}  // namespace

void checkXInput(CheckResults& results) {
  const HMODULE library = LoadLibraryW(L"xinput9_1_0.dll");
  if (library == nullptr) {
    results.push_back({"XInput 9.1.0", CheckLevel::Fail, "加载 xinput9_1_0.dll 失败"});
    return;
  }
  const auto getState = reinterpret_cast<XInputGetStateFunction>(reinterpret_cast<void*>(GetProcAddress(library, "XInputGetState")));
  if (getState == nullptr) {
    results.push_back({"XInput 9.1.0", CheckLevel::Fail, "找不到 XInputGetState"});
    FreeLibrary(library);
    return;
  }
  unsigned connected = 0;
  bool failed = false;
  for (DWORD index = 0; index < CONTROLLER_COUNT; ++index) {
    alignas(8) unsigned char state[16]{};
    const DWORD status = getState(index, state);
    if (status == ERROR_SUCCESS) ++connected;
    else if (status != ERROR_DEVICE_NOT_CONNECTED) failed = true;
  }
  char text[64]{};
  std::snprintf(text, sizeof(text), "已连接 %u 个手柄", connected);
  results.push_back({"XInput 9.1.0", failed ? CheckLevel::Fail : CheckLevel::Pass, failed ? "读取状态返回了意外的错误" : text});
  FreeLibrary(library);
}

}  // namespace ember::win32::probe
