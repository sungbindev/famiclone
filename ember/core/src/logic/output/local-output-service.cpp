/*
 * ------------------------------------------------------------------------
 * 名称：local-output-service.cpp
 * 说明：输出设置服务的实现。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-14
 * ------------------------------------------------------------------------
 */

#include "logic/output/local-output-service.hpp"

#include <algorithm>

#include "shared/errors/core-error.hpp"

namespace ember::logic::output {

namespace {

constexpr std::int64_t MAX_VOLUME = 100;

}  // namespace

LocalOutputService::LocalOutputService(std::shared_ptr<platform::AudioOutput> audio) : audio_(std::move(audio)) {
  if (audio_ != nullptr) audio_->setVolume(static_cast<unsigned>(volume_), muted_);
}

contracts::output::OutputSetVolumeResult LocalOutputService::setVolume(std::optional<std::int64_t> volume, std::optional<bool> muted) {
  if (volume.has_value() && (*volume < 0 || *volume > MAX_VOLUME)) throw shared::SchemaError("音量必须在 0 到 100 之间");
  std::lock_guard lock(mutex_);
  if (volume.has_value()) volume_ = *volume;
  if (muted.has_value()) muted_ = *muted;
  if (audio_ != nullptr) audio_->setVolume(static_cast<unsigned>(volume_), muted_);
  return contracts::output::OutputSetVolumeResult{volume_, muted_};
}

contracts::output::OutputSetVolumeResult LocalOutputService::volume() const {
  std::lock_guard lock(mutex_);
  return contracts::output::OutputSetVolumeResult{volume_, muted_};
}

}  // namespace ember::logic::output
