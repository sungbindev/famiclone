/*
 * ------------------------------------------------------------------------
 * 名称：runtime-test-support.hpp
 * 说明：运行服务测试的公共辅助：线程安全的事件记录与帧记录、轮询等待、按 mapper 号构造 iNES 文件、构造装入请求、用 RAM 标记区分冷启动与热启动的小程序。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-14
 * 备注：事件与帧由模拟线程发布，测试线程读取，所以这里的记录都加了锁。
 * ------------------------------------------------------------------------
 */

#pragma once

#include <chrono>
#include <cstdint>
#include <functional>
#include <mutex>
#include <string>
#include <thread>
#include <utility>
#include <vector>

#include "../hardware/fc-test-support.hpp"
#include "contracts/runtime/runtime-service.hpp"
#include "platform/events/event-sink.hpp"
#include "platform/streams/frame-output.hpp"

namespace ember::test {

/** 线程安全的事件记录。 */
class SyncEventSink final : public platform::EventSink {
 public:
  void publish(std::string_view name, const nlohmann::json& data) override {
    std::lock_guard lock(mutex_);
    events_.emplace_back(std::string(name), data);
  }

  /** 某个事件已发布的次数。 */
  [[nodiscard]] int count(const std::string& name) const {
    std::lock_guard lock(mutex_);
    int total = 0;
    for (const auto& event : events_) total += event.first == name ? 1 : 0;
    return total;
  }

  /** 某个事件最近一次的数据；没有时为 null。 */
  [[nodiscard]] nlohmann::json last(const std::string& name) const {
    std::lock_guard lock(mutex_);
    for (auto it = events_.rbegin(); it != events_.rend(); ++it) {
      if (it->first == name) return it->second;
    }
    return nullptr;
  }

  /** 某个事件的全部数据，按发布顺序。 */
  [[nodiscard]] std::vector<nlohmann::json> all(const std::string& name) const {
    std::lock_guard lock(mutex_);
    std::vector<nlohmann::json> result;
    for (const auto& event : events_) {
      if (event.first == name) result.push_back(event.second);
    }
    return result;
  }

 private:
  mutable std::mutex mutex_;
  std::vector<std::pair<std::string, nlohmann::json>> events_;
};

/** 记录收到的帧：次数、尺寸、帧号是否连续、最近一帧的第一个像素。 */
class FrameCapture final : public platform::FrameOutput {
 public:
  void publish(const platform::FrameView& frame) noexcept override {
    std::lock_guard lock(mutex_);
    if (frame.pixels.size() != static_cast<std::size_t>(frame.width) * frame.height) sizeMismatch_ = true;
    if (count_ > 0 && frame.number != lastNumber_ + 1 && frame.number != 1) numberGap_ = true;
    width_ = frame.width;
    height_ = frame.height;
    lastNumber_ = frame.number;
    firstPixel_ = frame.pixels.empty() ? 0 : frame.pixels[0];
    ++count_;
  }

  [[nodiscard]] std::uint64_t count() const {
    std::lock_guard lock(mutex_);
    return count_;
  }
  [[nodiscard]] std::uint32_t width() const {
    std::lock_guard lock(mutex_);
    return width_;
  }
  [[nodiscard]] std::uint32_t height() const {
    std::lock_guard lock(mutex_);
    return height_;
  }
  [[nodiscard]] std::uint64_t lastNumber() const {
    std::lock_guard lock(mutex_);
    return lastNumber_;
  }
  [[nodiscard]] std::uint32_t firstPixel() const {
    std::lock_guard lock(mutex_);
    return firstPixel_;
  }
  [[nodiscard]] bool sizeMismatch() const {
    std::lock_guard lock(mutex_);
    return sizeMismatch_;
  }
  /** 帧号不是逐帧加 1（上电后从 1 重新开始除外）。 */
  [[nodiscard]] bool numberGap() const {
    std::lock_guard lock(mutex_);
    return numberGap_;
  }

 private:
  mutable std::mutex mutex_;
  std::uint64_t count_{0};
  std::uint32_t width_{0};
  std::uint32_t height_{0};
  std::uint64_t lastNumber_{0};
  std::uint32_t firstPixel_{0};
  bool sizeMismatch_{false};
  bool numberGap_{false};
};

/** 轮询等待条件成立，最长 10 秒；返回条件是否成立。 */
inline bool waitFor(const std::function<bool()>& condition) {
  const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(10);
  while (std::chrono::steady_clock::now() < deadline) {
    if (condition()) return true;
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
  }
  return condition();
}

/** 构造 iNES 文件：16K PRG（复位向量指向 $8000 的程序）、没有 CHR（用 CHR RAM）。 */
inline std::vector<std::uint8_t> makeInesFile(const std::vector<std::uint8_t>& program, unsigned mapper = 0) {
  std::vector<std::uint8_t> file = {'N', 'E', 'S', 0x1A, 1, 0, static_cast<std::uint8_t>((mapper & 0x0F) << 4), static_cast<std::uint8_t>(mapper & 0xF0), 0, 0, 0, 0, 0, 0, 0, 0};
  std::vector<std::uint8_t> prg(16384, 0xEA);
  std::copy(program.begin(), program.end(), prg.begin());
  const std::uint16_t vectors[3] = {0x8000, 0x8000, 0x8000};
  for (int i = 0; i < 3; ++i) {
    prg[0x3FFA + static_cast<std::size_t>(i) * 2] = static_cast<std::uint8_t>(vectors[i] & 0xFF);
    prg[0x3FFB + static_cast<std::size_t>(i) * 2] = static_cast<std::uint8_t>(vectors[i] >> 8);
  }
  file.insert(file.end(), prg.begin(), prg.end());
  return file;
}

/** 在 $0000 放一个标记：冷启动（内部 RAM 全零）把背景色设为 coldColor，标记已在（软复位保持了 RAM）时设为 warmColor，之后空转。 */
inline std::vector<std::uint8_t> makeBootMarkerProgram(std::uint8_t coldColor, std::uint8_t warmColor) {
  auto paint = [](Assembler& code, std::uint8_t color) {
    code.ppuWrite(0x3F00, color).ldaImm(0).staAbs(0x2006).staAbs(0x2006).jmpSelf();
  };
  // 主段：lda $00；bne 热启动。冷启动段紧随其后。
  constexpr std::uint16_t COLD_ORIGIN = 0x8005;
  Assembler cold(COLD_ORIGIN);
  cold.ldaImm(1).staAbs(0x0000);
  paint(cold, coldColor);
  const std::uint16_t warmOrigin = static_cast<std::uint16_t>(COLD_ORIGIN + cold.bytes().size());
  Assembler main(0x8000);
  main.ldaAbs(0x0000).bne(warmOrigin);
  Assembler warm(warmOrigin);
  paint(warm, warmColor);
  std::vector<std::uint8_t> program = main.bytes();
  program.insert(program.end(), cold.bytes().begin(), cold.bytes().end());
  program.insert(program.end(), warm.bytes().begin(), warm.bytes().end());
  return program;
}

/** 在 nrom 与测试用的 mapper 7 插件之外，再装一个会在写 $8000 时抛出异常的插件（mapper 7），给脚本出错的测试用。 */
inline std::vector<std::uint8_t> failingPluginPackage() {
  nlohmann::json manifest = nlohmann::json::parse(minimalManifest("test-failing", 7));
  manifest["registers"] = nlohmann::json::parse(R"([{ "bus": "cpu", "range": "$8000-$8000", "read": false, "write": true }])");
  return makeEmap({{"manifest.json", manifest.dump()}, {"main.js", "export default { write() { throw new Error('boom'); } };"}});
}

/** 一段只写一次 $8000 然后空转的程序，配合 failingPluginPackage 触发脚本异常。 */
inline std::vector<std::uint8_t> makeWriteRegisterProgram() {
  Assembler code(0x8000);
  code.ldaImm(1).staAbs(0x8000).jmpSelf();
  return code.bytes();
}

}  // namespace ember::test
