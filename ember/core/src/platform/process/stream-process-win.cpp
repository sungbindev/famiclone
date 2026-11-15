/*
 * ------------------------------------------------------------------------
 * 名称：stream-process-win.cpp
 * 说明：流式读取子进程输出的 Windows 实现：CreateProcessW，参数按 Windows 的命令行规则逐个加引号。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-15
 * 备注：子进程只继承标准输入（空设备）、标准输出与标准错误三个句柄（PROC_THREAD_ATTRIBUTE_HANDLE_LIST），不显示控制台窗口；
 *       标准错误由后台线程读完，避免它的管道写满后子进程卡住。
 * ------------------------------------------------------------------------
 */

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>

#include <algorithm>
#include <memory>
#include <mutex>
#include <thread>

#include "platform/process/stream-process.hpp"

namespace ember::platform {

namespace {

/** 标准错误最多保留的字节数。 */
constexpr std::size_t ERROR_OUTPUT_LIMIT = std::size_t{64} * 1024;

/** 自动关闭的句柄。 */
class Handle {
 public:
  Handle() = default;
  explicit Handle(HANDLE handle) : handle_(handle) {}
  ~Handle() { reset(); }
  Handle(const Handle&) = delete;
  Handle& operator=(const Handle&) = delete;
  Handle(Handle&& other) noexcept : handle_(other.handle_) { other.handle_ = nullptr; }

  [[nodiscard]] HANDLE get() const { return handle_; }
  HANDLE* out() { return &handle_; }

  void reset() {
    if (handle_ != nullptr && handle_ != INVALID_HANDLE_VALUE) CloseHandle(handle_);
    handle_ = nullptr;
  }

 private:
  HANDLE handle_ = nullptr;
};

/** UTF-8 转 UTF-16。 */
std::wstring widen(const std::string& text) {
  if (text.empty()) return {};
  const int size = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, text.data(), static_cast<int>(text.size()), nullptr, 0);
  if (size <= 0) throw ProcessError("参数不是合法的 UTF-8");
  std::wstring result(static_cast<std::size_t>(size), L'\0');
  MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, text.data(), static_cast<int>(text.size()), result.data(), size);
  return result;
}

/** 按 CommandLineToArgvW 的规则给一个参数加引号。 */
void appendQuoted(std::wstring& line, const std::wstring& argument) {
  if (!line.empty()) line += L' ';
  if (!argument.empty() && argument.find_first_of(L" \t\n\v\"") == std::wstring::npos) {
    line += argument;
    return;
  }
  line += L'"';
  for (auto it = argument.begin();; ++it) {
    std::size_t backslashes = 0;
    while (it != argument.end() && *it == L'\\') {
      ++it;
      ++backslashes;
    }
    if (it == argument.end()) {
      line.append(backslashes * 2, L'\\');
      break;
    }
    if (*it == L'"') {
      line.append(backslashes * 2 + 1, L'\\');
      line += L'"';
    } else {
      line.append(backslashes, L'\\');
      line += *it;
    }
  }
  line += L'"';
}

class WindowsStreamProcess final : public StreamProcess {
 public:
  WindowsStreamProcess(Handle process, Handle output, Handle error) : process_(std::move(process)), output_(std::move(output)), error_(std::move(error)) {
    errorReader_ = std::thread([this] {
      char buffer[4096];
      DWORD read = 0;
      while (ReadFile(error_.get(), buffer, sizeof(buffer), &read, nullptr) && read > 0) {
        std::lock_guard lock(errorMutex_);
        errorText_.append(buffer, read);
        if (errorText_.size() > ERROR_OUTPUT_LIMIT) errorText_.erase(0, errorText_.size() - ERROR_OUTPUT_LIMIT);
      }
    });
  }

  ~WindowsStreamProcess() override {
    terminate();
    if (errorReader_.joinable()) errorReader_.join();
  }

  std::size_t read(std::uint8_t* buffer, std::size_t size) override {
    DWORD read = 0;
    const DWORD request = static_cast<DWORD>(std::min<std::size_t>(size, 1U << 20));
    if (!ReadFile(output_.get(), buffer, request, &read, nullptr)) return 0;
    return read;
  }

  void terminate() override {
    if (process_.get() != nullptr) TerminateProcess(process_.get(), 1);
  }

  [[nodiscard]] std::string errorOutput() override {
    std::lock_guard lock(errorMutex_);
    return errorText_;
  }

 private:
  Handle process_;
  Handle output_;
  Handle error_;
  std::thread errorReader_;
  std::mutex errorMutex_;
  std::string errorText_;
};

/** 新建可继承的管道；本进程一端由调用方取消继承。 */
void createPipe(Handle& read, Handle& write) {
  SECURITY_ATTRIBUTES attributes{sizeof(SECURITY_ATTRIBUTES), nullptr, TRUE};
  if (!CreatePipe(read.out(), write.out(), &attributes, 0)) throw ProcessError("无法创建管道，错误码 " + std::to_string(GetLastError()));
}

}  // namespace

std::unique_ptr<StreamProcess> startStreamProcess(const std::filesystem::path& program, const std::vector<std::string>& arguments) {
  std::wstring commandLine;
  appendQuoted(commandLine, program.wstring());
  for (const auto& argument : arguments) appendQuoted(commandLine, widen(argument));

  Handle outputRead, outputWrite, errorRead, errorWrite;
  createPipe(outputRead, outputWrite);
  createPipe(errorRead, errorWrite);
  SetHandleInformation(outputRead.get(), HANDLE_FLAG_INHERIT, 0);
  SetHandleInformation(errorRead.get(), HANDLE_FLAG_INHERIT, 0);
  // 标准输入接空设备。
  SECURITY_ATTRIBUTES inheritable{sizeof(SECURITY_ATTRIBUTES), nullptr, TRUE};
  Handle input(CreateFileW(L"NUL", GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, &inheritable, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr));
  if (input.get() == INVALID_HANDLE_VALUE) throw ProcessError("无法打开空设备，错误码 " + std::to_string(GetLastError()));

  SIZE_T attributeSize = 0;
  InitializeProcThreadAttributeList(nullptr, 1, 0, &attributeSize);
  auto attributeBuffer = std::make_unique<std::uint8_t[]>(attributeSize);
  auto* attributeList = reinterpret_cast<LPPROC_THREAD_ATTRIBUTE_LIST>(attributeBuffer.get());
  if (!InitializeProcThreadAttributeList(attributeList, 1, 0, &attributeSize)) throw ProcessError("无法初始化进程属性");
  struct AttributeGuard {
    LPPROC_THREAD_ATTRIBUTE_LIST list;
    ~AttributeGuard() { DeleteProcThreadAttributeList(list); }
  } attributeGuard{attributeList};
  HANDLE inherited[] = {input.get(), outputWrite.get(), errorWrite.get()};
  if (!UpdateProcThreadAttribute(attributeList, 0, PROC_THREAD_ATTRIBUTE_HANDLE_LIST, inherited, sizeof(inherited), nullptr, nullptr)) {
    throw ProcessError("无法设置继承的句柄");
  }

  STARTUPINFOEXW startup{};
  startup.StartupInfo.cb = sizeof(startup);
  startup.StartupInfo.dwFlags = STARTF_USESTDHANDLES;
  startup.StartupInfo.hStdInput = input.get();
  startup.StartupInfo.hStdOutput = outputWrite.get();
  startup.StartupInfo.hStdError = errorWrite.get();
  startup.lpAttributeList = attributeList;

  PROCESS_INFORMATION info{};
  const std::wstring programPath = program.wstring();
  if (!CreateProcessW(programPath.c_str(), commandLine.data(), nullptr, nullptr, TRUE, CREATE_NO_WINDOW | EXTENDED_STARTUPINFO_PRESENT | CREATE_UNICODE_ENVIRONMENT, nullptr, nullptr,
                      &startup.StartupInfo, &info)) {
    throw ProcessError("无法启动程序 " + program.filename().string() + "，错误码 " + std::to_string(GetLastError()));
  }
  Handle process(info.hProcess);
  Handle thread(info.hThread);
  // 子进程的一端在本进程中关闭，子进程退出后读取才会结束。
  outputWrite.reset();
  errorWrite.reset();
  return std::make_unique<WindowsStreamProcess>(std::move(process), std::move(outputRead), std::move(errorRead));
}

}  // namespace ember::platform
