/*
 * ------------------------------------------------------------------------
 * 名称：latest-frame-slot.hpp
 * 说明：画面帧的单槽缓冲：核心在模拟线程发布帧，界面线程取最新一帧，旧帧直接丢弃。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-19
 * 备注：同时是核心的帧输出（publish 在模拟线程调用，复制走像素后立即返回，不阻塞）和界面的帧来源（FrameSource）；
 *       有新帧时经界面线程通知一次，槽里的帧被取走之前不重复通知；每帧恰好结算一次：被取走，或被更新的帧顶掉（记为丢弃）。
 * ------------------------------------------------------------------------
 */

#pragma once

#include <memory>

#include "contracts/core-gateway/frame-source.hpp"
#include "platform/threading/ui-thread.hpp"

namespace ember::win32::infra {

/** 画面帧的单槽缓冲。 */
class LatestFrameSlot : public contracts::FrameSource {
 public:
  explicit LatestFrameSlot(platform::UiThread& ui);

  /**
   * 发布一帧（在模拟线程调用）：复制像素放进槽里，槽里原有没取走的帧记为丢弃。
   * @param width 宽。
   * @param height 高。
   * @param number 帧号。
   * @param pixels 像素，共 width × height 个。
   */
  void publish(std::uint32_t width, std::uint32_t height, std::uint64_t number, const std::uint32_t* pixels) noexcept;

  [[nodiscard]] bool takeLatest(contracts::FrameImage& frame) override;
  void setOnFrame(std::function<void()> handler) override;
  [[nodiscard]] contracts::FrameStats stats() const override;

 private:
  struct State;
  platform::UiThread& ui_;
  std::shared_ptr<State> state_;
};

}  // namespace ember::win32::infra
