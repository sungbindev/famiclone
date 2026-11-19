/*
 * ------------------------------------------------------------------------
 * 名称：file-error-logger.hpp
 * 说明：外壳的系统错误日志：追加到 <日志目录>/<本地日期>/errors.jsonl，一行一条。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-19
 * 备注：字段与 Electron 外壳的 src/platform/logging/file-error-logger.ts、核心的 file-error-log.cpp 一致（time、process、module、code、message、stack、context）；
 *       任意线程可调用，按调用顺序写入；写入失败时丢弃该条，不再写日志，不抛异常；外壳的 process 字段固定为 main。
 * ------------------------------------------------------------------------
 */

#pragma once

#include <filesystem>
#include <functional>
#include <mutex>
#include <optional>
#include <string>

#include <nlohmann/json.hpp>

namespace ember::win32::platform {

/** 一条系统错误。 */
struct ErrorRecord {
  /** 所属模块，无法确定时为空。 */
  std::optional<std::string> module;
  std::string code;
  std::string message;
  /** 排查需要的上下文，没有时为空。 */
  nlohmann::json context = nlohmann::json::object();
};

/** 文件错误日志。 */
class FileErrorLogger {
 public:
  /** 取当前时间（用于测试替换）：本地时间的年月日与 UTC 的 ISO 时间文字。 */
  struct Clock {
    std::function<std::string()> localDate;
    std::function<std::string()> isoTime;
  };

  /** @param logsRoot 日志根目录（<用户数据目录>\logs）。 */
  explicit FileErrorLogger(std::filesystem::path logsRoot);
  FileErrorLogger(std::filesystem::path logsRoot, Clock clock);

  /** 写入一条错误；不抛出异常。 */
  void write(const ErrorRecord& record) noexcept;

 private:
  std::filesystem::path logsRoot_;
  Clock clock_;
  std::mutex mutex_;
};

}  // namespace ember::win32::platform
