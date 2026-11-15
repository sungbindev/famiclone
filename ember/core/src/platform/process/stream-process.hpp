/*
 * ------------------------------------------------------------------------
 * 名称：stream-process.hpp
 * 说明：启动外部程序并一边运行一边读它的标准输出：参数按数组传入，不经命令行外壳；可随时结束程序。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-15
 * 备注：按平台分开实现（stream-process-win.cpp、stream-process-mac.cpp）；只在模块的 infra 中使用，如调用随应用附带的 ffmpeg 解码磁带；
 *       标准输入不接任何内容（空设备），标准错误在后台读取，只保留末尾 64 KB；
 *       读标准输出会阻塞，管道满了子进程自己就停下来等，所以读得慢不会丢数据。
 * ------------------------------------------------------------------------
 */

#pragma once

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

namespace ember::platform {

/** 程序无法启动。 */
class ProcessError : public std::runtime_error {
 public:
  using std::runtime_error::runtime_error;
};

/** 一个正在运行的子进程。 */
class StreamProcess {
 public:
  virtual ~StreamProcess() = default;

  /**
   * 读标准输出，阻塞到有数据、程序结束或被 terminate 结束。
   * @return 读到的字节数；程序已结束且数据读完时返回 0。
   */
  virtual std::size_t read(std::uint8_t* buffer, std::size_t size) = 0;

  /** 结束程序；已经结束时没有效果。正在阻塞的 read 随之返回。 */
  virtual void terminate() = 0;

  /** 标准错误的内容（末尾 64 KB），供出错时给说明。 */
  [[nodiscard]] virtual std::string errorOutput() = 0;
};

/**
 * 启动外部程序。
 * @param program 程序的完整路径。
 * @param arguments 参数（UTF-8），不含程序本身。
 * @throws ProcessError 无法启动。
 */
std::unique_ptr<StreamProcess> startStreamProcess(const std::filesystem::path& program, const std::vector<std::string>& arguments);

}  // namespace ember::platform
