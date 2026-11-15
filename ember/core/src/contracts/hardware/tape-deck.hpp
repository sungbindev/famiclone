/*
 * ------------------------------------------------------------------------
 * 名称：tape-deck.hpp
 * 说明：硬件契约：磁带机。磁带的声音来源与录音去向、磁带机的控制接口（给介质模块用）、磁带机与主机之间的信号线。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-15
 * 备注：磁带是一个声音文件：单声道时整路是数据线（Family BASIC 的 SAVE 与 LOAD）；立体声时左声道是声音（经机器的扬声器播放），
 *       右声道是数据线（声像磁带，卡带上的电路取数据）；两种磁带走的是同一个磁带机与同一组信号线；
 *       解码在契约之外：来源对磁带机只表现为 96 kHz、16 位、左右交错的采样流，由读取它的基础设施负责把任意声音文件转成这种流；
 *       信号线的时间都是主时钟换算出来的模拟时间，与真实时间无关：暂停、断电时磁带不走。
 * ------------------------------------------------------------------------
 */

#pragma once

#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <string>

namespace ember::contracts::hardware {

/** 磁带的声音来源：96 kHz、16 位、左右交错的采样流。 */
class TapeSource {
 public:
  /** 来源给出的采样率（赫兹）。 */
  static constexpr unsigned SAMPLE_RATE = 96000;

  virtual ~TapeSource() = default;

  /**
   * 取下一批采样，可能阻塞到有数据为止。
   * @param interleaved 输出缓冲，左右声道交错，至少 frames * 2 个值。
   * @param frames 请求的帧数。
   * @return 实际给出的帧数；0 表示磁带放完。
   */
  virtual std::size_t read(std::int16_t* interleaved, std::size_t frames) = 0;

  /** 倒带：下次 read 从头给出。 */
  virtual void rewind() = 0;

  /** 总时长（秒）；不知道时为 0。 */
  [[nodiscard]] virtual double durationSeconds() const = 0;

  /** 是否是立体声磁带：是则左声道是声音、右声道是数据；否则整路是数据，两个声道相同，不出声。 */
  [[nodiscard]] virtual bool stereo() const = 0;
};

/** 录音的去向：单声道、16 位的采样流。 */
class TapeSink {
 public:
  /** 录音的采样率（赫兹）。 */
  static constexpr unsigned SAMPLE_RATE = 32000;

  virtual ~TapeSink() = default;

  /** 追加一批采样。 */
  virtual void write(const std::int16_t* samples, std::size_t count) = 0;

  /** 录完：补全文件并关闭。 */
  virtual void finish() = 0;
};

/** 磁带机的工作状态。 */
enum class TapeMode : std::uint8_t {
  /** 停止。 */
  Stopped,
  /** 播放。 */
  Playing,
  /** 录音。 */
  Recording
};

/** 磁带机的控制接口：介质模块经它载入、取出、播放、录音与停止。 */
class TapeControl {
 public:
  virtual ~TapeControl() = default;

  /** 里面是否有磁带。 */
  [[nodiscard]] virtual bool loaded() const = 0;

  /** 磁带的名字（文件名），没有磁带时为空。 */
  [[nodiscard]] virtual std::string name() const = 0;

  /** 工作状态。 */
  [[nodiscard]] virtual TapeMode mode() = 0;

  /** 当前位置（秒）；录音时是已录的时长。 */
  [[nodiscard]] virtual double positionSeconds() = 0;

  /** 总时长（秒）；不知道或没有磁带时为 0。 */
  [[nodiscard]] virtual double durationSeconds() const = 0;

  /** 当前的磁带是否是立体声（左声道是声音）。 */
  [[nodiscard]] virtual bool stereo() const = 0;

  /**
   * 放入一盘可以播放的磁带，位置回到开头；原来的磁带被取出。
   * @param source 声音来源。
   * @param name 显示用的名字。
   */
  virtual void load(std::unique_ptr<TapeSource> source, std::string name) = 0;

  /** 开始播放；没有磁带或已在播放、录音时没有效果。 */
  virtual void play() = 0;

  /**
   * 放入一盘空白磁带并开始录音，原来的磁带被取出。
   * @param sink 录音的去向。
   * @param name 显示用的名字。
   */
  virtual void record(std::unique_ptr<TapeSink> sink, std::string name) = 0;

  /** 停止播放或录音；录音停止时把录到的内容写完。已经停止时没有效果。 */
  virtual void stop() = 0;

  /** 取出磁带（先停止）。 */
  virtual void eject() = 0;
};

/** 数据线的比较器：把 16 位采样转成 1 位信号。采样不低于 high 时为 1，不高于 low 时为 0，之间保持原值（施密特触发）；invert 为真时输出取反。 */
struct TapeComparator {
  std::int16_t low{-3072};
  std::int16_t high{3072};
  bool invert{false};
};

/** 磁带机与主机之间的信号线。 */
class TapeLine {
 public:
  virtual ~TapeLine() = default;

  /** 磁带送给主机的 1 位信号（没有播放时保持最后的值）。 */
  [[nodiscard]] virtual bool level() = 0;

  /** 主机送给磁带机的 1 位信号；录音时记录下来。 */
  virtual void drive(bool high) = 0;

  /** 设置比较器；默认是 ±3072 的施密特触发、不取反。 */
  virtual void setComparator(const TapeComparator& comparator) = 0;

  /**
   * 设置边沿通知：播放时数据线每次翻转，在翻转的时刻回调；参数是翻转后的电平与距上一次翻转的采样数（96 kHz）。
   * 传空函数取消。
   */
  virtual void setEdgeListener(std::function<void(bool level, std::uint32_t samplesSincePrevious)> listener) = 0;
};

}  // namespace ember::contracts::hardware
