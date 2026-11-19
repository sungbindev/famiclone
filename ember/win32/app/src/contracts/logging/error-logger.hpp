/*
 * ------------------------------------------------------------------------
 * 名称：error-logger.hpp
 * 说明：系统错误日志的抽象：外壳把需要排查的错误写进本地日志。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-19
 * 备注：写入不抛出异常，不记录密钥与鉴权信息。
 * ------------------------------------------------------------------------
 */

#pragma once

#include <optional>
#include <string>

#include <nlohmann/json.hpp>

namespace ember::win32::contracts {

/** 一条系统错误。 */
struct ErrorLogEntry {
  /** 所属模块，无法确定时为空。 */
  std::optional<std::string> module;
  /** 错误码。 */
  std::string code;
  /** 说明。 */
  std::string message;
  /** 排查需要的上下文。 */
  nlohmann::json context = nlohmann::json::object();
};

/** 系统错误日志。 */
class ErrorLogger {
 public:
  virtual ~ErrorLogger() = default;

  /** 写入一条错误；不抛出异常，可以在任意线程调用。 */
  virtual void write(const ErrorLogEntry& entry) noexcept = 0;
};

}  // namespace ember::win32::contracts
