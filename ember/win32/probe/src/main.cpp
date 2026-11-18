/*
 * ------------------------------------------------------------------------
 * 名称：main.cpp
 * 说明：平台探针 ember-probe.exe 的入口：依次运行各项检查，在控制台显示结果并写入同目录的 ember-probe-report.txt。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-19
 * 备注：退出码是失败项的个数，全部通过或只有提示时为 0；报告用 UTF-8 保存。
 * ------------------------------------------------------------------------
 */

#include <cstdio>
#include <filesystem>
#include <fstream>
#include <string>

#include "probe-check.hpp"

namespace {

using ember::win32::probe::CheckLevel;
using ember::win32::probe::CheckResults;

/** 窗口与画面的宽高（像素），与模拟器画面一致。 */
constexpr unsigned PROBE_WIDTH = 256;
constexpr unsigned PROBE_HEIGHT = 240;

/** 级别的标记文字。 */
const char* levelMark(CheckLevel level) {
  switch (level) {
    case CheckLevel::Pass: return "通过";
    case CheckLevel::Info: return "提示";
    case CheckLevel::Fail: return "失败";
  }
  return "";
}

/** 生成整份报告的文字。 */
std::string formatReport(const CheckResults& results) {
  std::string report = "Ember 平台探针\n";
  for (const auto& item : results) report += std::string("[") + levelMark(item.level) + "] " + item.name + "：" + item.detail + "\n";
  return report;
}

/** 在控制台显示文字；控制台用宽字符输出，重定向到文件时写 UTF-8 字节。 */
void printText(const std::string& text) {
  const HANDLE output = GetStdHandle(STD_OUTPUT_HANDLE);
  DWORD mode = 0;
  if (GetConsoleMode(output, &mode) != 0) {
    const int size = MultiByteToWideChar(CP_UTF8, 0, text.data(), static_cast<int>(text.size()), nullptr, 0);
    std::wstring wide(static_cast<std::size_t>(size), L'\0');
    MultiByteToWideChar(CP_UTF8, 0, text.data(), static_cast<int>(text.size()), wide.data(), size);
    DWORD written = 0;
    WriteConsoleW(output, wide.data(), static_cast<DWORD>(wide.size()), &written, nullptr);
    return;
  }
  DWORD written = 0;
  WriteFile(output, text.data(), static_cast<DWORD>(text.size()), &written, nullptr);
}

/** 报告文件的路径：与可执行文件同目录。 */
std::filesystem::path reportPath() {
  wchar_t buffer[MAX_PATH]{};
  GetModuleFileNameW(nullptr, buffer, MAX_PATH);
  return std::filesystem::path(buffer).parent_path() / "ember-probe-report.txt";
}

}  // namespace

int main() {
  using namespace ember::win32::probe;
  CheckResults results;
  checkSystem(results);
  const HWND window = createProbeWindow(PROBE_WIDTH, PROBE_HEIGHT);
  if (window == nullptr) {
    results.push_back({"探针窗口", CheckLevel::Fail, "创建窗口失败"});
  } else {
    checkDirect2D(results, window);
    checkDirect3D(results, window);
    DestroyWindow(window);
  }
  checkXInput(results);
  checkWinHttp(results);
  checkAudio(results);
  checkCore(results);

  const std::string report = formatReport(results);
  printText(report);
  std::ofstream file(reportPath(), std::ios::binary);
  file << report;

  int failures = 0;
  for (const auto& item : results) {
    if (item.level == CheckLevel::Fail) ++failures;
  }
  return failures;
}
