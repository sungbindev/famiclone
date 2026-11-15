/*
 * ------------------------------------------------------------------------
 * 名称：tape-deck.hpp
 * 说明：磁带机：播放时把磁带的采样流按主时钟换成数据线上的 1 位信号与声音，录音时把主机送来的 1 位信号按时间记成采样。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-15
 * 备注：播放的时间都是模拟时间：位置 = 播放起点的帧号 + 经过的主时钟周期按 96 kHz 换算出的帧数；读数据线、读位置、取声音时才把磁带处理到现在，
 *       所以没有人读时不占调度；设置了边沿通知时，向前扫描找到下一次翻转，在翻转的时刻登记一个调度器事件，事件里再回调；
 *       数据线取立体声磁带的右声道（单声道磁带的唯一声道），经施密特比较器转成 1 位信号；左声道（立体声磁带）降采样后混进机器的声音；
 *       录音记录的是 drive 调用的时刻：每次翻转前，把上一个电平按经过的时间补成 32 kHz 的方波采样（±20000），交给录音去向；
 *       磁带放完后自动停止，再按播放时回到开头；
 *       磁带机属于主机：换卡带、重新开机（powerOn 调用 stop）都会停止它；来源可能阻塞（外部程序解码的管道），读取发生在模拟线程。
 * ------------------------------------------------------------------------
 */

#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <deque>
#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "contracts/hardware/scheduler.hpp"
#include "contracts/hardware/tape-deck.hpp"

namespace ember::logic::hardware {

/** 磁带机。 */
class TapeDeck final : public contracts::hardware::TapeControl, public contracts::hardware::TapeLine, public contracts::hardware::EventListener {
 public:
  /** 录音时一个电平的采样幅度。 */
  static constexpr std::int16_t RECORD_AMPLITUDE = 20000;
  /** 一次从来源取的帧数。 */
  static constexpr std::size_t CHUNK_FRAMES = 4096;
  /** 向前扫描翻转的最大距离（帧，1 秒）；超过时在扫描终点再登记一次扫描。 */
  static constexpr std::uint64_t SCAN_FRAMES = contracts::hardware::TapeSource::SAMPLE_RATE;

  /**
   * @param scheduler 调度器：取当前时间，登记边沿事件。
   * @param masterHz 主时钟频率（赫兹）。
   */
  TapeDeck(contracts::hardware::Scheduler& scheduler, double masterHz);
  ~TapeDeck() override;

  TapeDeck(const TapeDeck&) = delete;
  TapeDeck& operator=(const TapeDeck&) = delete;

  // TapeControl
  [[nodiscard]] bool loaded() const override { return source_ != nullptr || recording(); }
  [[nodiscard]] std::string name() const override { return name_; }
  [[nodiscard]] contracts::hardware::TapeMode mode() override;
  [[nodiscard]] double positionSeconds() override;
  [[nodiscard]] double durationSeconds() const override { return source_ != nullptr ? source_->durationSeconds() : 0.0; }
  [[nodiscard]] bool stereo() const override { return source_ != nullptr && source_->stereo(); }
  void load(std::unique_ptr<contracts::hardware::TapeSource> source, std::string name) override;
  void play() override;
  void record(std::unique_ptr<contracts::hardware::TapeSink> sink, std::string name) override;
  void stop() override;
  void eject() override;

  // TapeLine
  [[nodiscard]] bool level() override;
  void drive(bool high) override;
  void setComparator(const contracts::hardware::TapeComparator& comparator) override;
  void setEdgeListener(std::function<void(bool level, std::uint32_t samplesSincePrevious)> listener) override;

  // EventListener
  void onEvent(contracts::hardware::MasterTime when, std::uint32_t tag) override;

  /** 设置声音的输出采样率（赫兹）；0 表示不产生声音。 */
  void setAudioRate(unsigned rate);

  /** 把自上次以来播放的声音（立体声磁带的左声道）加进 samples；多出的留到下次。 */
  void mixAudio(std::vector<float>& samples);

 private:
  using Frame = std::array<std::int16_t, 2>;

  /** 事件标记：数据线翻转、继续扫描、磁带放完。 */
  enum Tag : std::uint32_t { TagEdge = 0, TagRescan = 1, TagEnd = 2 };

  [[nodiscard]] bool recording() const { return mode_ == contracts::hardware::TapeMode::Recording; }

  /** 播放中当前时刻对应的帧号（正在放的那一帧）。 */
  [[nodiscard]] std::uint64_t frameAt(contracts::hardware::MasterTime now) const;
  /** 某一帧对应的主时钟时间（向上取整，保证到点时 frameAt 不小于它）。 */
  [[nodiscard]] contracts::hardware::MasterTime tickOf(std::uint64_t frame) const;
  /** 把播放处理到现在。 */
  void advanceTo(contracts::hardware::MasterTime now);
  /** 保证缓冲里有这一帧；来源已经放完时返回 false。 */
  bool ensureBuffered(std::uint64_t frame);
  /** 处理一帧：比较器与声音。 */
  void processFrame(const Frame& frame);
  /** 施密特比较器：给出新的内部状态。 */
  [[nodiscard]] bool schmitt(bool state, std::int16_t sample) const;
  /** 磁带放完：停止，位置停在结尾。 */
  void finishPlayback();
  /** 清掉缓冲，回到开头（来源倒带）。 */
  void rewindSource();
  /** 重新向前扫描下一次翻转并登记事件。 */
  void scheduleNextEdge();
  void cancelEvent();
  /** 录音：把上一个电平补到现在。 */
  void renderRecording(contracts::hardware::MasterTime now);
  void flushRecording();

  contracts::hardware::Scheduler& scheduler_;
  /** 每帧（96 kHz）占的主时钟周期数。 */
  double ticksPerFrame_;
  double masterHz_;

  std::unique_ptr<contracts::hardware::TapeSource> source_;
  std::unique_ptr<contracts::hardware::TapeSink> sink_;
  std::string name_;
  contracts::hardware::TapeMode mode_{contracts::hardware::TapeMode::Stopped};

  // 播放
  double anchorTick_{0};
  std::uint64_t anchorFrame_{0};
  /** 下一个要处理的帧号。 */
  std::uint64_t processed_{0};
  std::deque<Frame> buffer_;
  /** buffer_ 第一帧的帧号。 */
  std::uint64_t bufferBase_{0};
  bool endOfSource_{false};
  /** 来源放完时的总帧数。 */
  std::uint64_t endFrame_{0};
  contracts::hardware::TapeComparator comparator_;
  bool schmittState_{false};

  // 声音
  unsigned audioRate_{0};
  double framesPerAudioSample_{0};
  double audioPhase_{0};
  double audioSum_{0};
  double audioCount_{0};
  std::vector<float> audioPending_;

  // 边沿
  std::function<void(bool, std::uint32_t)> listener_;
  contracts::hardware::EventId eventId_{contracts::hardware::NO_EVENT};
  std::uint64_t scanFrame_{0};
  bool scanState_{false};
  std::uint64_t pendingEdgeFrame_{0};
  bool pendingEdgeLevel_{false};
  std::uint64_t lastEdgeFrame_{0};

  // 录音
  bool driven_{false};
  double recordStartTick_{0};
  std::uint64_t recordedSamples_{0};
  bool recordLevel_{false};
  std::vector<std::int16_t> recordBuffer_;
};

}  // namespace ember::logic::hardware
