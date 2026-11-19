/*
 * ------------------------------------------------------------------------
 * 名称：file-error-logger.cpp
 * 说明：外壳的系统错误日志的实现。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-19
 * 备注：无。
 * ------------------------------------------------------------------------
 */

#include "platform/logging/file-error-logger.hpp"

#include <windows.h>

#include <cstdio>
#include <exception>
#include <fstream>
#include <system_error>

namespace ember::win32::platform {

namespace {

std::string defaultLocalDate() {
  SYSTEMTIME time;
  GetLocalTime(&time);
  char text[16];
  std::snprintf(text, sizeof(text), "%04d-%02d-%02d", time.wYear, time.wMonth, time.wDay);
  return text;
}

std::string defaultIsoTime() {
  SYSTEMTIME time;
  GetSystemTime(&time);
  char text[32];
  std::snprintf(text, sizeof(text), "%04d-%02d-%02dT%02d:%02d:%02d.%03dZ", time.wYear, time.wMonth, time.wDay, time.wHour, time.wMinute, time.wSecond, time.wMilliseconds);
  return text;
}

}  // namespace

FileErrorLogger::FileErrorLogger(std::filesystem::path logsRoot) : FileErrorLogger(std::move(logsRoot), Clock{defaultLocalDate, defaultIsoTime}) {}

FileErrorLogger::FileErrorLogger(std::filesystem::path logsRoot, Clock clock) : logsRoot_(std::move(logsRoot)), clock_(std::move(clock)) {}

void FileErrorLogger::write(const ErrorRecord& record) noexcept {
  try {
    const nlohmann::json line{{"time", clock_.isoTime()},
                              {"process", "main"},
                              {"module", record.module ? nlohmann::json(*record.module) : nlohmann::json(nullptr)},
                              {"code", record.code},
                              {"message", record.message},
                              {"stack", nullptr},
                              {"context", record.context}};
    const std::string text = line.dump(-1, ' ', false, nlohmann::json::error_handler_t::replace) + "\n";
    std::lock_guard lock(mutex_);
    const std::filesystem::path directory = logsRoot_ / std::filesystem::path(clock_.localDate());
    std::error_code error;
    std::filesystem::create_directories(directory, error);
    std::ofstream file(directory / "errors.jsonl", std::ios::binary | std::ios::app);
    if (file) file.write(text.data(), static_cast<std::streamsize>(text.size()));
  } catch (const std::exception&) {
    // 写日志失败不影响业务，也不再写日志
  }
}

}  // namespace ember::win32::platform
