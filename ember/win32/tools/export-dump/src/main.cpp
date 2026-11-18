/*
 * ------------------------------------------------------------------------
 * 名称：main.cpp
 * 说明：导出表转储工具 ember-export-dump.exe：读取一份 DLL 名称清单，把这些系统 DLL 的导出函数名写成 JSON，用作 Windows 7 SP1 的导出基线。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-19
 * 备注：在干净的 Windows 7 SP1 上运行一次，把生成的 JSON 交回仓库；用法：ember-export-dump.exe <DLL 清单.txt> <输出.json>。
 *       DLL 只以“不解析依赖”的方式映射进内存读取导出表，不会执行 DLL 的任何代码；系统里找不到的 DLL 记入 missing。
 * ------------------------------------------------------------------------
 */

#include <cstdio>
#include <fstream>
#include <string>
#include <vector>

#include <windows.h>

namespace {

/** RtlGetVersion 的函数类型。 */
using RtlGetVersionFunction = LONG(WINAPI*)(OSVERSIONINFOW*);

/** 在控制台显示 UTF-8 文字；重定向时写字节。 */
void printText(const std::string& text) {
  const HANDLE output = GetStdHandle(STD_OUTPUT_HANDLE);
  DWORD mode = 0;
  DWORD written = 0;
  if (GetConsoleMode(output, &mode) != 0) {
    const int size = MultiByteToWideChar(CP_UTF8, 0, text.data(), static_cast<int>(text.size()), nullptr, 0);
    std::wstring wide(static_cast<std::size_t>(size), L'\0');
    MultiByteToWideChar(CP_UTF8, 0, text.data(), static_cast<int>(text.size()), wide.data(), size);
    WriteConsoleW(output, wide.data(), static_cast<DWORD>(wide.size()), &written, nullptr);
    return;
  }
  WriteFile(output, text.data(), static_cast<DWORD>(text.size()), &written, nullptr);
}

/** 去掉字符串两端的空白。 */
std::string trim(const std::string& text) {
  const auto first = text.find_first_not_of(" \t\r\n");
  if (first == std::string::npos) return {};
  const auto last = text.find_last_not_of(" \t\r\n");
  return text.substr(first, last - first + 1);
}

/** 读取清单，一行一个 DLL 名称。 */
std::vector<std::string> readList(const char* path) {
  std::vector<std::string> names;
  std::ifstream input(path);
  std::string line;
  while (std::getline(input, line)) {
    const auto name = trim(line);
    if (!name.empty()) names.push_back(name);
  }
  return names;
}

/** 读取系统版本的文字。 */
std::string readOsVersion() {
  OSVERSIONINFOW version{};
  version.dwOSVersionInfoSize = sizeof(version);
  const HMODULE ntdll = GetModuleHandleW(L"ntdll.dll");
  const auto getVersion = ntdll == nullptr ? nullptr : reinterpret_cast<RtlGetVersionFunction>(reinterpret_cast<void*>(GetProcAddress(ntdll, "RtlGetVersion")));
  if (getVersion == nullptr || getVersion(&version) != 0) return "unknown";
  char text[64]{};
  std::snprintf(text, sizeof(text), "%lu.%lu.%lu", version.dwMajorVersion, version.dwMinorVersion, version.dwBuildNumber);
  std::wstring csd(version.szCSDVersion);
  const int size = WideCharToMultiByte(CP_UTF8, 0, csd.data(), static_cast<int>(csd.size()), nullptr, 0, nullptr, nullptr);
  std::string service(static_cast<std::size_t>(size), '\0');
  if (size > 0) WideCharToMultiByte(CP_UTF8, 0, csd.data(), static_cast<int>(csd.size()), service.data(), size, nullptr, nullptr);
  return std::string(text) + (service.empty() ? "" : " " + service);
}

/** 读取一个已映射 DLL 的全部导出函数名；格式不合法时返回 false。 */
bool readExports(HMODULE module, std::vector<std::string>& names) {
  const auto* base = reinterpret_cast<const unsigned char*>(module);
  const auto* dos = reinterpret_cast<const IMAGE_DOS_HEADER*>(base);
  if (dos->e_magic != IMAGE_DOS_SIGNATURE) return false;
  const auto* nt = reinterpret_cast<const IMAGE_NT_HEADERS64*>(base + dos->e_lfanew);
  if (nt->Signature != IMAGE_NT_SIGNATURE || nt->OptionalHeader.Magic != IMAGE_NT_OPTIONAL_HDR64_MAGIC) return false;
  const IMAGE_DATA_DIRECTORY& directory = nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT];
  if (directory.VirtualAddress == 0) return true;
  const auto* exports = reinterpret_cast<const IMAGE_EXPORT_DIRECTORY*>(base + directory.VirtualAddress);
  const auto* nameRvas = reinterpret_cast<const DWORD*>(base + exports->AddressOfNames);
  for (DWORD index = 0; index < exports->NumberOfNames; ++index) names.emplace_back(reinterpret_cast<const char*>(base + nameRvas[index]));
  return true;
}

}  // namespace

int main(int argc, char** argv) {
  if (argc != 3) {
    printText("用法：ember-export-dump.exe <DLL 清单.txt> <输出.json>\n");
    return 2;
  }
  const auto dlls = readList(argv[1]);
  if (dlls.empty()) {
    printText("DLL 清单是空的，或者读不到清单文件\n");
    return 2;
  }

  std::string json = "{\n  \"os\": \"" + readOsVersion() + "\",\n  \"exports\": {";
  std::string missing;
  bool firstDll = true;
  for (const auto& dll : dlls) {
    const HMODULE module = LoadLibraryExA(dll.c_str(), nullptr, DONT_RESOLVE_DLL_REFERENCES);
    std::vector<std::string> names;
    if (module == nullptr || !readExports(module, names)) {
      missing += (missing.empty() ? "" : ", ") + std::string("\"") + dll + "\"";
      if (module != nullptr) FreeLibrary(module);
      continue;
    }
    json += (firstDll ? "\n    \"" : ",\n    \"") + dll + "\": [";
    firstDll = false;
    for (std::size_t index = 0; index < names.size(); ++index) json += (index == 0 ? "\"" : ",\"") + names[index] + "\"";
    json += "]";
    FreeLibrary(module);
  }
  json += "\n  },\n  \"missing\": [" + missing + "]\n}\n";

  std::ofstream output(argv[2], std::ios::binary);
  output << json;
  printText("已写入 " + std::string(argv[2]) + "；系统里没有的 DLL：" + (missing.empty() ? std::string("无") : missing) + "\n");
  return 0;
}
