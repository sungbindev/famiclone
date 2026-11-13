/*
 * ------------------------------------------------------------------------
 * 名称：local-output-service.hpp
 * 说明：输出设置服务的实现：音量与静音保存在内存里，变化时应用到声音输出。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-14
 * 备注：初始音量 100、未静音；构造时就把初始值应用到声音输出。
 * ------------------------------------------------------------------------
 */

#pragma once

#include <memory>
#include <mutex>

#include "contracts/output/output-service.hpp"
#include "platform/audio/audio-output.hpp"

namespace ember::logic::output {

/** 本机的输出设置服务。 */
class LocalOutputService final : public contracts::output::OutputService {
 public:
  /** 初始音量（百分比）。 */
  static constexpr std::int64_t DEFAULT_VOLUME = 100;

  /** @param audio 声音输出，设置变化时应用到它。 */
  explicit LocalOutputService(std::shared_ptr<platform::AudioOutput> audio);

  contracts::output::OutputSetVolumeResult setVolume(std::optional<std::int64_t> volume, std::optional<bool> muted) override;
  [[nodiscard]] contracts::output::OutputSetVolumeResult volume() const override;

 private:
  std::shared_ptr<platform::AudioOutput> audio_;
  mutable std::mutex mutex_;
  std::int64_t volume_{DEFAULT_VOLUME};
  bool muted_{false};
};

}  // namespace ember::logic::output
