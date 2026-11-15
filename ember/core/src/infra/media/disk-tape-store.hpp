/*
 * ------------------------------------------------------------------------
 * 名称：disk-tape-store.hpp
 * 说明：磁带文件存取的磁盘实现：用随应用附带的 ffmpeg 把声音文件解码成 96 kHz 立体声的采样流，录音写成 32 kHz 单声道的 WAV 文件。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-15
 * 备注：打开时先运行一次 ffmpeg 读出时长与声道数（不解码）；解码在来源第一次被读取时才启动子进程，管道满了子进程自己停下，所以长磁带不占内存；
 *       来源的 rewind 结束子进程，下次读取重新启动；解码中途出错时当作磁带放完并写系统错误日志；
 *       ffmpeg 的输出格式取 wav（程序里没有编入原始采样的封装器），读取时解析头部再把 data 块当 16 位立体声采样流。
 * ------------------------------------------------------------------------
 */

#pragma once

#include <filesystem>
#include <memory>
#include <string>

#include "contracts/media/tape-store.hpp"
#include "platform/logging/error-log.hpp"

namespace ember::infra::media {

/** 磁盘上的磁带文件存取。 */
class DiskTapeStore final : public contracts::media::TapeStore {
 public:
  /**
   * @param ffmpegPath ffmpeg 程序的完整路径；为空或文件不存在时打开磁带报“没有找到解码程序”。
   * @param errors 系统错误日志，解码中途出错时写入。
   */
  DiskTapeStore(std::filesystem::path ffmpegPath, std::shared_ptr<platform::ErrorLog> errors);

  [[nodiscard]] std::unique_ptr<contracts::hardware::TapeSource> open(const std::string& path) const override;
  [[nodiscard]] std::unique_ptr<contracts::hardware::TapeSink> create(const std::string& path) const override;

 private:
  std::filesystem::path ffmpegPath_;
  std::shared_ptr<platform::ErrorLog> errors_;
};

}  // namespace ember::infra::media
