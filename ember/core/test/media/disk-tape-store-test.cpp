/*
 * ------------------------------------------------------------------------
 * 名称：disk-tape-store-test.cpp
 * 说明：磁带文件存取的测试：用录音文件生成真实的 WAV 磁带，检查解码出的采样流能定位到任意位置、定位后继续往后读、倒带后从头读、定位到结尾之外时读不到数据。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-21
 * 备注：磁带内容是 3 秒的单声道 32 kHz 录音，每 0.1 秒一段，第 n 段的采样值固定为 (n + 1) * 1000，解码后升到 96 kHz 立体声，段内（离边界 5 毫秒以上）的值固定（每个声道按 1/√2 缩小）。
 * ------------------------------------------------------------------------
 */

#include <gtest/gtest.h>

#include <cmath>
#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

#include "infra/media/disk-tape-store.hpp"
#include "shared/errors/core-error.hpp"

namespace {

namespace fs = std::filesystem;
namespace hw = ember::contracts::hardware;

constexpr unsigned SECTION_COUNT = 30;
constexpr unsigned SECTION_SAMPLES = hw::TapeSink::SAMPLE_RATE / 10;

/** 在临时目录里生成的磁带文件，测试结束时删除。 */
class TapeFile {
 public:
  TapeFile() {
    directory_ = fs::temp_directory_path() / "ember-disk-tape-store-test";
    fs::create_directories(directory_);
    const fs::path file = directory_ / "sections.wav";
    path_ = file.string();
    ember::infra::media::DiskTapeStore store(nullptr);
    const auto sink = store.create(path_);
    for (unsigned section = 0; section < SECTION_COUNT; ++section) {
      const std::vector<std::int16_t> samples(SECTION_SAMPLES, static_cast<std::int16_t>((section + 1) * 1000));
      sink->write(samples.data(), samples.size());
    }
    sink->finish();
  }
  ~TapeFile() {
    std::error_code error;
    fs::remove_all(directory_, error);
  }
  [[nodiscard]] const std::string& path() const { return path_; }

 private:
  fs::path directory_;
  std::string path_;
};

/** 读 count 帧，返回实际读到的左声道采样。 */
std::vector<std::int16_t> readLeft(hw::TapeSource& source, std::size_t count) {
  std::vector<std::int16_t> interleaved(count * 2);
  const std::size_t got = source.read(interleaved.data(), count);
  std::vector<std::int16_t> left(got);
  for (std::size_t index = 0; index < got; ++index) left[index] = interleaved[index * 2];
  return left;
}

/** 秒数换成帧号。 */
std::uint64_t frameOf(double seconds) { return static_cast<std::uint64_t>(seconds * hw::TapeSource::SAMPLE_RATE); }

/** 第 section 段（从 0 开始）解码后的采样值：单声道升成立体声时每个声道按 1/sqrt(2) 缩小。 */
double levelOf(unsigned section) { return (section + 1) * 1000.0 * std::sqrt(0.5); }

TEST(DiskTapeStoreSeek, SeekingLandsInTheRightSectionAndReadingContinuesFromThere) {
  const TapeFile tape;
  ember::infra::media::DiskTapeStore store(nullptr);
  const auto source = store.open(tape.path());
  ASSERT_NE(source, nullptr);
  EXPECT_NEAR(source->durationSeconds(), 3.0, 0.05);

  source->seek(frameOf(1.55));
  const auto first = readLeft(*source, 2000);
  ASSERT_EQ(first.size(), 2000U);
  EXPECT_NEAR(first.front(), levelOf(15), 60) << "1.55 秒在第 16 段（1.5 到 1.6 秒）";
  EXPECT_NEAR(first[1000], levelOf(15), 60);

  // 继续往后读：1.55 秒起 0.05 秒后进入第 17 段
  const auto next = readLeft(*source, 96000 / 10);
  ASSERT_EQ(next.size(), 9600U);
  EXPECT_NEAR(next.back(), levelOf(16), 60);
}

TEST(DiskTapeStoreSeek, SeekingBackwardsAndRewindingWork) {
  const TapeFile tape;
  ember::infra::media::DiskTapeStore store(nullptr);
  const auto source = store.open(tape.path());
  source->seek(frameOf(2.55));
  EXPECT_NEAR(readLeft(*source, 100).front(), levelOf(25), 60);
  source->seek(frameOf(0.25));
  EXPECT_NEAR(readLeft(*source, 100).front(), levelOf(2), 60) << "往前定位";
  source->seek(frameOf(0.0));
  EXPECT_NEAR(readLeft(*source, 100)[50], levelOf(0), 60) << "定位到 0 等于倒带";
  source->seek(frameOf(1.05));
  source->rewind();
  EXPECT_NEAR(readLeft(*source, 100)[50], levelOf(0), 60) << "倒带取消定位";
}

TEST(DiskTapeStoreSeek, SeekingPastTheEndReadsNothing) {
  const TapeFile tape;
  ember::infra::media::DiskTapeStore store(nullptr);
  const auto source = store.open(tape.path());
  source->seek(frameOf(10.0));
  EXPECT_TRUE(readLeft(*source, 100).empty());
}

TEST(DiskTapeStoreOverwrite, RecordingFromAPositionReplacesOnlyThatStretch) {
  const TapeFile tape;
  ember::infra::media::DiskTapeStore store(nullptr);
  {
    const auto sink = store.overwrite(tape.path(), 1.0);
    const std::vector<std::int16_t> samples(hw::TapeSink::SAMPLE_RATE / 2, 9000);  // 0.5 秒
    sink->write(samples.data(), samples.size());
    sink->finish();
  }
  EXPECT_FALSE(fs::exists(tape.path() + ".recording")) << "写完后临时文件已经换成了磁带文件";
  const auto source = store.open(tape.path());
  EXPECT_NEAR(source->durationSeconds(), 3.0, 0.05);
  source->seek(frameOf(0.55));
  EXPECT_NEAR(readLeft(*source, 100).front(), levelOf(5), 60) << "录音位置之前的内容保留";
  source->seek(frameOf(1.2));
  EXPECT_NEAR(readLeft(*source, 100).front(), 9000, 2) << "录音的一段";
  source->seek(frameOf(1.55));
  EXPECT_NEAR(readLeft(*source, 100).front(), levelOf(15), 60) << "录音结束处之后的内容保留";
  source->seek(frameOf(2.55));
  EXPECT_NEAR(readLeft(*source, 100).front(), levelOf(25), 60);
}

TEST(DiskTapeStoreOverwrite, RecordingPastTheEndExtendsTheTape) {
  const TapeFile tape;
  ember::infra::media::DiskTapeStore store(nullptr);
  {
    const auto sink = store.overwrite(tape.path(), 2.8);
    const std::vector<std::int16_t> samples(hw::TapeSink::SAMPLE_RATE / 2, 9000);
    sink->write(samples.data(), samples.size());
    sink->finish();
  }
  const auto source = store.open(tape.path());
  EXPECT_NEAR(source->durationSeconds(), 3.3, 0.05);
  source->seek(frameOf(2.75));
  EXPECT_NEAR(readLeft(*source, 100).front(), levelOf(27), 60);
  source->seek(frameOf(3.1));
  EXPECT_NEAR(readLeft(*source, 100).front(), 9000, 2);
}

TEST(DiskTapeStoreOverwrite, ADestroyedUnfinishedRecordingLeavesTheTapeAlone) {
  const TapeFile tape;
  ember::infra::media::DiskTapeStore store(nullptr);
  {
    const auto sink = store.overwrite(tape.path(), 1.0);
    const std::vector<std::int16_t> samples(1000, 9000);
    sink->write(samples.data(), samples.size());
  }
  EXPECT_FALSE(fs::exists(tape.path() + ".recording")) << "没有 finish 就销毁，临时文件被丢弃";
  const auto source = store.open(tape.path());
  source->seek(frameOf(1.02));
  EXPECT_NEAR(readLeft(*source, 100).front(), levelOf(10), 60) << "原文件不动";
}

TEST(DiskTapeStoreOverwrite, OnlyAWavFileCanBeOverwritten) {
  const TapeFile tape;
  ember::infra::media::DiskTapeStore store(nullptr);
  std::string code;
  try {
    static_cast<void>(store.overwrite((fs::path(tape.path()).parent_path() / "lesson.mp3").string(), 0.0));
  } catch (const ember::shared::CoreError& error) {
    code = error.code();
  }
  EXPECT_EQ(code, "media.tapeNotWritable");
}

}  // namespace
