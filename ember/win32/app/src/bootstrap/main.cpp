/*
 * ------------------------------------------------------------------------
 * 名称：main.cpp
 * 说明：程序入口：把命令行转成 UTF-8 参数，交给组装根运行。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-19
 * 备注：DPI 感知、Windows 版本声明都在清单里（resources/ember-win.manifest）。
 * ------------------------------------------------------------------------
 */

#include <windows.h>

#include "bootstrap/application.hpp"
#include "platform/process/command-line.hpp"
#include "shared/launch/launch-args.hpp"

int WINAPI wWinMain(HINSTANCE /*instance*/, HINSTANCE /*previous*/, PWSTR /*commandLine*/, int /*show*/) {
  return ember::win32::bootstrap::runApplication(ember::win32::shared::parseLaunchArguments(ember::win32::platform::splitCommandLine(GetCommandLineW())));
}
