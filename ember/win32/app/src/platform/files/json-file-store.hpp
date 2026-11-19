/*
 * ------------------------------------------------------------------------
 * 名称：json-file-store.hpp
 * 说明：本地 JSON 文件的读写：同步读取（文件不存在、读不了或不是合法 JSON 时得到空值，不抛异常），防抖写入，退出前可立即写盘。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-19
 * 备注：做法同 Electron 外壳的 src/platform/shell-state/json-file-store.ts：写入先写同目录的临时文件再改名，写到一半断电不会留下半个文件；
 *       写失败只经 onError 报告，不影响界面；防抖用界面线程的延时任务，所以只能在界面线程使用；对象销毁时把没写的内容写盘。
 * ------------------------------------------------------------------------
 */

#pragma once

#include <filesystem>
#include <functional>
#include <optional>
#include <string>

#include <nlohmann/json.hpp>

#include "platform/threading/ui-thread.hpp"

namespace ember::win32::platform {

/** 本地 JSON 文件存储。 */
class JsonFileStore {
 public:
  /** 读写出错时的报告（说明文字，UTF-8）。 */
  using ErrorHandler = std::function<void(const std::string& message)>;

  /**
   * @param path 文件的完整路径。
   * @param ui 界面线程，防抖写盘用它的延时任务。
   * @param onError 读写出错时的报告，可以为空。
   * @param debounceMilliseconds 写入的防抖时间：连续修改合并成一次写盘。
   */
  JsonFileStore(std::filesystem::path path, UiThread& ui, ErrorHandler onError = {}, unsigned debounceMilliseconds = 500);
  ~JsonFileStore();
  JsonFileStore(const JsonFileStore&) = delete;
  JsonFileStore& operator=(const JsonFileStore&) = delete;

  /** 读取文件内容；文件不存在、读不了或内容不是合法 JSON 时为空。 */
  [[nodiscard]] std::optional<nlohmann::json> read();

  /** 安排写入；防抖时间内再次调用时只写最后一次的内容。 */
  void write(nlohmann::json value);

  /** 立即写入尚未写盘的内容；没有时什么都不做。 */
  void flush();

 private:
  void report(const std::string& message) const;

  std::filesystem::path path_;
  UiThread& ui_;
  ErrorHandler onError_;
  unsigned debounceMilliseconds_;
  std::optional<nlohmann::json> pending_;
  unsigned timer_{0};
};

}  // namespace ember::win32::platform
