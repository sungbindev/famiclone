/*
 * ------------------------------------------------------------------------
 * 名称：json-file-store.cpp
 * 说明：本地 JSON 文件存储的实现。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-19
 * 备注：无。
 * ------------------------------------------------------------------------
 */

#include "platform/files/json-file-store.hpp"

#include <windows.h>

#include <fstream>
#include <iterator>
#include <system_error>

#include "platform/text/wide-text.hpp"

namespace ember::win32::platform {

JsonFileStore::JsonFileStore(std::filesystem::path path, UiThread& ui, ErrorHandler onError, unsigned debounceMilliseconds)
    : path_(std::move(path)), ui_(ui), onError_(std::move(onError)), debounceMilliseconds_(debounceMilliseconds) {}

JsonFileStore::~JsonFileStore() { flush(); }

void JsonFileStore::report(const std::string& message) const {
  if (onError_) onError_(message + "：" + pathToUtf8(path_));
}

std::optional<nlohmann::json> JsonFileStore::read() {
  std::ifstream file(path_, std::ios::binary);
  if (!file) {
    // 文件不存在是正常情况（第一次运行），不报告
    std::error_code ignored;
    if (std::filesystem::exists(path_, ignored)) report("读取文件失败");
    return std::nullopt;
  }
  const std::string text((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
  nlohmann::json value = nlohmann::json::parse(text, nullptr, false);
  if (value.is_discarded()) {
    report("文件内容不是合法的 JSON");
    return std::nullopt;
  }
  return value;
}

void JsonFileStore::write(nlohmann::json value) {
  pending_ = std::move(value);
  if (timer_ != 0) ui_.cancelDelayed(timer_);
  timer_ = ui_.postDelayed(debounceMilliseconds_, [this] {
    timer_ = 0;
    flush();
  });
}

void JsonFileStore::flush() {
  if (timer_ != 0) {
    ui_.cancelDelayed(timer_);
    timer_ = 0;
  }
  if (!pending_) return;
  const nlohmann::json value = std::move(*pending_);
  pending_.reset();

  std::error_code error;
  std::filesystem::create_directories(path_.parent_path(), error);
  std::filesystem::path temporary = path_;
  temporary += L".tmp";
  {
    std::ofstream file(temporary, std::ios::binary | std::ios::trunc);
    if (!file) {
      report("写入文件失败");
      return;
    }
    const std::string text = value.dump(2);
    file.write(text.data(), static_cast<std::streamsize>(text.size()));
    file.flush();
    if (!file) {
      report("写入文件失败");
      return;
    }
  }
  if (MoveFileExW(temporary.c_str(), path_.c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH) == FALSE) {
    report("替换文件失败");
    std::filesystem::remove(temporary, error);
  }
}

}  // namespace ember::win32::platform
