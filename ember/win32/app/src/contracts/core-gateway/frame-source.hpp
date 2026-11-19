/*
 * ------------------------------------------------------------------------
 * 名称：frame-source.hpp
 * 说明：画面帧的来源抽象：核心在模拟线程每完成一帧就把像素交出来，界面线程取最新一帧绘制，旧帧直接丢弃。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-19
 * 备注：规则与 Electron 外壳的背压一致：只有一个单槽缓冲，每帧恰好结算一次（被取走，或被更新的帧顶掉后记为丢弃），不积压；
 *       像素格式：每像素 4 字节，内存顺序 R、G、B、A，按行从上到下存放。
 * ------------------------------------------------------------------------
 */

#pragma once

#include <chrono>
#include <cstdint>
#include <functional>
#include <vector>

namespace ember::win32::contracts {

/** 一帧画面。 */
struct FrameImage {
  std::uint32_t width{0};
  std::uint32_t height{0};
  /** 帧号：自上电以来完成的帧数。 */
  std::uint64_t number{0};
  /** 核心送出这一帧的时刻（单调时钟），用来测量画面延迟。 */
  std::chrono::steady_clock::time_point publishedAt{};
  /** 像素，共 width × height 个。 */
  std::vector<std::uint32_t> pixels;
};

/** 帧的计数。 */
struct FrameStats {
  /** 核心送来的帧数。 */
  std::uint64_t published{0};
  /** 被界面取走绘制的帧数。 */
  std::uint64_t consumed{0};
  /** 还没被取走就被更新的帧顶掉的帧数。 */
  std::uint64_t dropped{0};
};

/** 画面帧的来源。 */
class FrameSource {
 public:
  virtual ~FrameSource() = default;

  /**
   * 取走最新一帧。
   * @param frame 接收这一帧；复用它的像素缓冲，避免每帧分配。
   * @returns 有新帧时为 true；上次取走以后没有新帧时为 false。
   */
  [[nodiscard]] virtual bool takeLatest(FrameImage& frame) = 0;

  /** 设置有新帧时的通知，在界面线程调用；槽里已有尚未取走的帧时不会重复通知。 */
  virtual void setOnFrame(std::function<void()> handler) = 0;

  /** 帧的计数。 */
  [[nodiscard]] virtual FrameStats stats() const = 0;
};

}  // namespace ember::win32::contracts
