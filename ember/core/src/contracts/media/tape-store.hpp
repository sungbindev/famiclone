/*
 * ------------------------------------------------------------------------
 * 名称：tape-store.hpp
 * 说明：磁带文件存取的契约：把声音文件打开成磁带的声音来源，为录音创建声音文件。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-15
 * 备注：解码与文件格式在实现里（infra/media）；路径只来自用户在系统窗口里的选择或调试服务的请求。
 * ------------------------------------------------------------------------
 */

#pragma once

#include <memory>
#include <string>

#include "contracts/hardware/tape-deck.hpp"

namespace ember::contracts::media {

/** 磁带文件的存取。 */
class TapeStore {
 public:
  virtual ~TapeStore() = default;

  /**
   * 打开声音文件作为磁带；文件内容的解码在返回的来源被读取时才进行。
   * @param path 声音文件的路径。
   * @throws shared::CoreError media.fileNotFound、media.fileUnreadable、media.tapeUnsupported（没有声音或无法解码）、media.decoderMissing（没有找到解码程序）。
   */
  virtual std::unique_ptr<hardware::TapeSource> open(const std::string& path) const = 0;

  /**
   * 创建（覆盖）录音文件：单声道 16 位的 WAV；来源的 finish 之后文件才完整。
   * @param path 录音文件的路径。
   * @throws shared::CoreError media.saveFailed。
   */
  virtual std::unique_ptr<hardware::TapeSink> create(const std::string& path) const = 0;
};

}  // namespace ember::contracts::media
