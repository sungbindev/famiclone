/*
 * ------------------------------------------------------------------------
 * 名称：dac-retimer-test.cpp
 * 说明：DAC 写入重新定时器的测试：零星写入直接输出、连续采样流被识别后等间隔输出且不丢不重、中断造成的停顿被缓冲吸收、采样流断开后回到直接输出。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-16
 * ------------------------------------------------------------------------
 */

#include <gtest/gtest.h>

#include <algorithm>
#include <cstdint>
#include <vector>

#include "logic/hardware/psg-um6576/dac-retimer.hpp"

namespace {

using ember::logic::hardware::DacRetimer;

constexpr double CPU_HZ = 1'773'447.0;
/** 采样间隔（CPU 周期，约 46.8 微秒）。 */
constexpr std::int64_t INTERVAL = 83;

struct Applied {
  std::int64_t cycle;
  int level;
};

/** 逐周期运行：到点的写入交给重新定时器，每个周期取一次输出。 */
std::vector<Applied> run(DacRetimer& retimer, const std::vector<std::pair<std::int64_t, int>>& writes, std::int64_t end) {
  std::vector<Applied> applied;
  std::size_t next = 0;
  for (std::int64_t cycle = 0; cycle < end; ++cycle) {
    while (next < writes.size() && writes[next].first == cycle) {
      retimer.push(cycle, static_cast<std::uint8_t>(writes[next].second));
      ++next;
    }
    std::uint8_t level = 0;
    if (retimer.poll(cycle, level)) applied.push_back({cycle, level});
  }
  return applied;
}

/** 每 60 个采样有一次中断造成的停顿：第 1 个采样晚 5 个间隔才到，之后的采样以最短 43 个周期（中断处理程序的时长）的间隔连着补写，直到追上原来的节拍。 */
std::vector<std::pair<std::int64_t, int>> streamWithStalls(int count) {
  std::vector<std::pair<std::int64_t, int>> writes;
  std::int64_t previous = 0;
  for (int index = 0; index < count; ++index) {
    const std::int64_t scheduled = static_cast<std::int64_t>(index) * INTERVAL;
    const std::int64_t arrival = std::max(index % 60 == 0 ? scheduled + 5 * INTERVAL : scheduled, previous + 43);
    previous = arrival;
    writes.emplace_back(arrival + 1000, index % 256);
  }
  return writes;
}

TEST(DacRetimer, SparseWritesAreAppliedAtOnceAtTheirOwnCycle) {
  DacRetimer retimer(CPU_HZ);
  const auto applied = run(retimer, {{0, 10}, {5000, 20}, {12000, 30}}, 13000);
  ASSERT_EQ(applied.size(), 3U);
  EXPECT_EQ(applied[0].cycle, 0);
  EXPECT_EQ(applied[1].cycle, 5000);
  EXPECT_EQ(applied[2].cycle, 12000);
  EXPECT_EQ(applied[2].level, 30);
}

TEST(DacRetimer, ASteadyStreamIsPlayedInOrderWithoutLossOrDuplication) {
  DacRetimer retimer(CPU_HZ);
  const auto writes = streamWithStalls(600);
  const auto applied = run(retimer, writes, writes.back().first + 5000);
  ASSERT_GT(applied.size(), 560U);
  for (std::size_t index = 1; index < applied.size(); ++index) {
    EXPECT_EQ((applied[index].level - applied[index - 1].level + 256) % 256, 1) << "第 " << index << " 个";
  }
}

TEST(DacRetimer, StallsCausedByInterruptsAreAbsorbedByTheBuffer) {
  DacRetimer retimer(CPU_HZ);
  const auto writes = streamWithStalls(900);
  const auto applied = run(retimer, writes, writes.back().first + 5000);
  // 认出采样流之后（前 300 个之外），输出间隔都接近采样间隔；直接按写入时刻输出时，停顿处的间隔约 6 倍。
  std::int64_t shortest = INTERVAL * 100;
  std::int64_t longest = 0;
  for (std::size_t index = 300; index + 1 < applied.size() - 20; ++index) {
    const std::int64_t gap = applied[index + 1].cycle - applied[index].cycle;
    shortest = std::min(shortest, gap);
    longest = std::max(longest, gap);
  }
  EXPECT_GE(shortest, INTERVAL * 7 / 10);
  EXPECT_LE(longest, INTERVAL * 13 / 10);
}

TEST(DacRetimer, TheStreamEndingFallsBackToDirectOutput) {
  DacRetimer retimer(CPU_HZ);
  auto writes = streamWithStalls(400);
  const std::int64_t lastCycle = writes.back().first;
  writes.emplace_back(lastCycle + 20000, 77);
  const auto applied = run(retimer, writes, lastCycle + 21000);
  ASSERT_FALSE(applied.empty());
  EXPECT_EQ(applied.back().level, 77);
  EXPECT_EQ(applied.back().cycle, lastCycle + 20000) << "采样流断开后的写入立即生效";
}

}  // namespace
