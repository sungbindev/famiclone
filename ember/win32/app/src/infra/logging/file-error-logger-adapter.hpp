/*
 * ------------------------------------------------------------------------
 * 名称：file-error-logger-adapter.hpp
 * 说明：系统错误日志的实现：把契约的 ErrorLogger 接到平台的文件错误日志上。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-19
 * 备注：平台层不引用契约，所以在这里做一次字段转换。
 * ------------------------------------------------------------------------
 */

#pragma once

#include "contracts/logging/error-logger.hpp"
#include "platform/logging/file-error-logger.hpp"

namespace ember::win32::infra {

/** 写到本地文件的系统错误日志。 */
class FileErrorLoggerAdapter : public contracts::ErrorLogger {
 public:
  explicit FileErrorLoggerAdapter(platform::FileErrorLogger& logger) : logger_(logger) {}

  void write(const contracts::ErrorLogEntry& entry) noexcept override { logger_.write(platform::ErrorRecord{entry.module, entry.code, entry.message, entry.context}); }

 private:
  platform::FileErrorLogger& logger_;
};

}  // namespace ember::win32::infra
