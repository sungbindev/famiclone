/*
 * ------------------------------------------------------------------------
 * 名称：rom-archive-source.hpp
 * 说明：ROM 包的来源：从固定地址下载 ROM 包并解出一个目录，供 machines.setRoms 按内容识别添加；用完后丢弃临时文件。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-28
 * 备注：地址由组装根决定，界面不能指定；下载在工作线程进行，结果回到界面线程；同一时刻只允许一次下载。
 * ------------------------------------------------------------------------
 */

#pragma once

#include <functional>
#include <string>

namespace ember::win32::contracts {

/** 一次下载解包的结果。 */
struct RomArchiveResult {
  bool ok{false};
  /** 失败的原因（中文）；成功时为空。 */
  std::string error;
  /** 解出的 ROM 文件所在目录（UTF-8）；失败时为空。 */
  std::string directory;
};

/** ROM 包的来源。 */
class RomArchiveSource {
 public:
  virtual ~RomArchiveSource() = default;

  /**
   * 下载 ROM 包并解出目录；结果在界面线程经 done 给出，可能晚于本函数返回。
   * 已有一次下载在进行时立即以失败回复。
   */
  virtual void fetch(std::function<void(const RomArchiveResult&)> done) = 0;

  /** 删除下载与解包的临时文件；结果处理完后调用。 */
  virtual void discard() = 0;
};

}  // namespace ember::win32::contracts