/*
 * ------------------------------------------------------------------------
 * 名称：latest-frame-slot.cpp
 * 说明：画面帧单槽缓冲的实现。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-19
 * 备注：无。
 * ------------------------------------------------------------------------
 */

#include "infra/core-client/latest-frame-slot.hpp"

#include <algorithm>
#include <mutex>
#include <utility>

namespace ember::win32::infra {

struct LatestFrameSlot::State {
  std::mutex mutex;
  contracts::FrameImage slot;
  bool hasFrame{false};
  bool notifyPending{false};
  contracts::FrameStats stats;
  /** 只在界面线程读写。 */
  std::function<void()> onFrame;
};

LatestFrameSlot::LatestFrameSlot(platform::UiThread& ui) : ui_(ui), state_(std::make_shared<State>()) {}

void LatestFrameSlot::publish(std::uint32_t width, std::uint32_t height, std::uint64_t number, const std::uint32_t* pixels) noexcept {
  try {
    bool notify = false;
    {
      std::lock_guard lock(state_->mutex);
      ++state_->stats.published;
      if (state_->hasFrame) ++state_->stats.dropped;
      const std::size_t count = static_cast<std::size_t>(width) * height;
      state_->slot.width = width;
      state_->slot.height = height;
      state_->slot.number = number;
      state_->slot.publishedAt = std::chrono::steady_clock::now();
      state_->slot.pixels.assign(pixels, pixels + count);
      state_->hasFrame = true;
      if (!state_->notifyPending) {
        state_->notifyPending = true;
        notify = true;
      }
    }
    if (!notify) return;
    ui_.post([weak = std::weak_ptr<State>(state_)] {
      const auto state = weak.lock();
      if (!state) return;
      {
        std::lock_guard lock(state->mutex);
        state->notifyPending = false;
      }
      if (state->onFrame) state->onFrame();
    });
  } catch (...) {
    // 内存不足等情况下丢弃这一帧，不能让异常越过模拟线程
  }
}

bool LatestFrameSlot::takeLatest(contracts::FrameImage& frame) {
  std::lock_guard lock(state_->mutex);
  if (!state_->hasFrame) return false;
  std::swap(frame, state_->slot);
  state_->hasFrame = false;
  ++state_->stats.consumed;
  return true;
}

void LatestFrameSlot::setOnFrame(std::function<void()> handler) { state_->onFrame = std::move(handler); }

contracts::FrameStats LatestFrameSlot::stats() const {
  std::lock_guard lock(state_->mutex);
  return state_->stats;
}

}  // namespace ember::win32::infra
