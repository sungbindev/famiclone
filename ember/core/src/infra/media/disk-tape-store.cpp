/*
 * ------------------------------------------------------------------------
 * 名称：disk-tape-store.cpp
 * 说明：磁带文件存取的磁盘实现：ffmpeg 解码的声音来源与 WAV 录音文件。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-15
 * ------------------------------------------------------------------------
 */

#include "infra/media/disk-tape-store.hpp"

#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <string_view>
#include <utility>
#include <vector>

#include "contracts/media/media-error.hpp"
#include "platform/process/stream-process.hpp"
#include "shared/errors/core-error.hpp"

namespace ember::infra::media {

namespace api = contracts::media;
namespace hw = contracts::hardware;
namespace fs = std::filesystem;

namespace {

fs::path toPath(const std::string& text) { return fs::path(std::u8string(text.begin(), text.end())); }

/** 路径转 UTF-8 文本。 */
std::string toUtf8(const fs::path& path) {
  const auto text = path.u8string();
  return std::string(text.begin(), text.end());
}

std::uint32_t readLe32(const std::uint8_t* data) { return static_cast<std::uint32_t>(data[0]) | (static_cast<std::uint32_t>(data[1]) << 8) | (static_cast<std::uint32_t>(data[2]) << 16) | (static_cast<std::uint32_t>(data[3]) << 24); }
std::uint16_t readLe16(const std::uint8_t* data) { return static_cast<std::uint16_t>(data[0] | (data[1] << 8)); }

void writeLe32(std::uint8_t* target, std::uint32_t value) {
  for (unsigned index = 0; index < 4; ++index) target[index] = static_cast<std::uint8_t>(value >> (index * 8));
}

/** 从文本里读 "Duration: 时:分:秒.百分秒" 的秒数；没有或写着 N/A 时为 0。 */
double parseDuration(const std::string& text) {
  const auto position = text.find("Duration:");
  if (position == std::string::npos) return 0.0;
  const char* cursor = text.c_str() + position + 9;
  char* end = nullptr;
  double total = 0.0;
  // 时:分:秒.小数，三段都要有。
  for (unsigned part = 0; part < 3; ++part) {
    while (*cursor == ' ') ++cursor;
    const double value = std::strtod(cursor, &end);
    if (end == cursor) return 0.0;
    total = total * 60.0 + value;
    cursor = end;
    if (part < 2) {
      if (*cursor != ':') return 0.0;
      ++cursor;
    }
  }
  return total;
}

/** 探测结果。 */
struct Probe {
  double duration{0};
  bool stereo{true};
};

/** 运行一次 ffmpeg（不解码）读出时长与声道；没有声音轨时抛 media.tapeUnsupported。 */
Probe probe(const fs::path& ffmpeg, const std::string& path) {
  std::string errorText;
  try {
    auto process = platform::startStreamProcess(ffmpeg, {"-hide_banner", "-nostdin", "-i", path});
    std::uint8_t buffer[256];
    while (process->read(buffer, sizeof(buffer)) > 0) {
    }
    errorText = process->errorOutput();
  } catch (const platform::ProcessError&) {
    throw shared::CoreError(api::ERROR_DECODER_MISSING, "无法启动解码程序 ffmpeg");
  }
  const auto audio = errorText.find("Audio:");
  if (audio == std::string::npos) throw shared::CoreError(api::ERROR_TAPE_UNSUPPORTED, "文件里没有声音，或格式无法解码：" + path);
  const auto lineEnd = errorText.find('\n', audio);
  const std::string line = errorText.substr(audio, lineEnd == std::string::npos ? std::string::npos : lineEnd - audio);
  Probe result;
  result.duration = parseDuration(errorText);
  result.stereo = line.find(", mono") == std::string::npos;
  return result;
}

/** ffmpeg 解码出来的声音来源。 */
class FfmpegTapeSource final : public hw::TapeSource {
 public:
  FfmpegTapeSource(fs::path ffmpeg, std::string file, Probe info, std::shared_ptr<platform::ErrorLog> errors)
      : ffmpeg_(std::move(ffmpeg)), file_(std::move(file)), info_(info), errors_(std::move(errors)) {}

  std::size_t read(std::int16_t* interleaved, std::size_t frames) override {
    if (failed_) return 0;
    if (process_ == nullptr && !start()) return 0;
    auto* bytes = reinterpret_cast<std::uint8_t*>(interleaved);
    const std::size_t wanted = frames * BYTES_PER_FRAME;
    std::size_t got = 0;
    while (got < wanted) {
      const std::size_t count = process_->read(bytes + got, wanted - got);
      if (count == 0) break;
      got += count;
    }
    return got / BYTES_PER_FRAME;
  }

  void rewind() override {
    process_.reset();
    failed_ = false;
  }

  [[nodiscard]] double durationSeconds() const override { return info_.duration; }
  [[nodiscard]] bool stereo() const override { return info_.stereo; }

 private:
  /** 每帧的字节数：左右各 16 位。 */
  static constexpr std::size_t BYTES_PER_FRAME = 4;

  bool start() {
    try {
      process_ = platform::startStreamProcess(ffmpeg_, {"-v", "error", "-nostdin", "-i", file_, "-vn", "-map", "0:a:0", "-map_metadata", "-1", "-fflags", "+bitexact", "-flags:a", "+bitexact", "-acodec",
                                                         "pcm_s16le", "-ac", "2", "-ar", "96000", "-f", "wav", "pipe:1"});
      if (readHeader()) return true;
      fail("解码输出的头部不是 96 kHz、16 位、立体声的 WAV");
    } catch (const platform::ProcessError& error) {
      fail(error.what());
    }
    return false;
  }

  /** 读满 count 字节；中途结束返回 false。 */
  bool readExact(std::uint8_t* target, std::size_t count) {
    std::size_t got = 0;
    while (got < count) {
      const std::size_t part = process_->read(target + got, count - got);
      if (part == 0) return false;
      got += part;
    }
    return true;
  }

  bool skip(std::uint64_t count) {
    std::uint8_t scratch[4096];
    while (count > 0) {
      const std::size_t part = static_cast<std::size_t>(std::min<std::uint64_t>(count, sizeof(scratch)));
      if (!readExact(scratch, part)) return false;
      count -= part;
    }
    return true;
  }

  /** 解析 WAV 头部，停在 data 块的第一个采样前。 */
  bool readHeader() {
    std::uint8_t riff[12];
    if (!readExact(riff, sizeof(riff)) || std::memcmp(riff, "RIFF", 4) != 0 || std::memcmp(riff + 8, "WAVE", 4) != 0) return false;
    bool formatOk = false;
    for (;;) {
      std::uint8_t chunk[8];
      if (!readExact(chunk, sizeof(chunk))) return false;
      const std::uint32_t size = readLe32(chunk + 4);
      if (std::memcmp(chunk, "fmt ", 4) == 0) {
        std::vector<std::uint8_t> format(std::min<std::uint32_t>(size, 64));
        if (format.size() < 16 || !readExact(format.data(), format.size())) return false;
        const std::uint16_t tag = readLe16(format.data());
        formatOk = (tag == 1 || tag == 0xFFFE) && readLe16(format.data() + 2) == 2 && readLe32(format.data() + 4) == hw::TapeSource::SAMPLE_RATE && readLe16(format.data() + 14) == 16;
        if (!skip(static_cast<std::uint64_t>(size - format.size()) + (size & 1U))) return false;
      } else if (std::memcmp(chunk, "data", 4) == 0) {
        return formatOk;
      } else if (!skip(static_cast<std::uint64_t>(size) + (size & 1U))) {
        return false;
      }
    }
  }

  void fail(const std::string& reason) {
    failed_ = true;
    process_.reset();
    if (errors_ != nullptr) errors_->write({"media", "media.tapeDecodeFailed", "磁带解码失败：" + reason, nlohmann::json{{"file", file_}}});
  }

  fs::path ffmpeg_;
  std::string file_;
  Probe info_;
  std::shared_ptr<platform::ErrorLog> errors_;
  std::unique_ptr<platform::StreamProcess> process_;
  bool failed_{false};
};

/** 录音的 WAV 文件：单声道、16 位、32 kHz。 */
class WavTapeSink final : public hw::TapeSink {
 public:
  WavTapeSink(const fs::path& file, const std::string& display) : stream_(file, std::ios::binary | std::ios::trunc), display_(display) {
    if (!stream_) throw shared::CoreError(api::ERROR_SAVE_FAILED, "无法创建录音文件：" + display);
    std::uint8_t header[HEADER_SIZE] = {};
    std::memcpy(header, "RIFF", 4);
    std::memcpy(header + 8, "WAVEfmt ", 8);
    writeLe32(header + 16, 16);
    header[20] = 1;  // PCM
    header[22] = 1;  // 单声道
    writeLe32(header + 24, SAMPLE_RATE);
    writeLe32(header + 28, SAMPLE_RATE * 2);
    header[32] = 2;   // 每帧 2 字节
    header[34] = 16;  // 位数
    std::memcpy(header + 36, "data", 4);
    stream_.write(reinterpret_cast<const char*>(header), HEADER_SIZE);
    if (!stream_) throw shared::CoreError(api::ERROR_SAVE_FAILED, "无法写入录音文件：" + display);
  }

  ~WavTapeSink() override {
    try {
      finish();
    } catch (...) {
      // 析构时无法再报告，文件头保留占位的大小。
    }
  }

  void write(const std::int16_t* samples, std::size_t count) override {
    if (finished_) return;
    std::vector<std::uint8_t> bytes(count * 2);
    for (std::size_t index = 0; index < count; ++index) {
      const auto value = static_cast<std::uint16_t>(samples[index]);
      bytes[index * 2] = static_cast<std::uint8_t>(value & 0xFF);
      bytes[index * 2 + 1] = static_cast<std::uint8_t>(value >> 8);
    }
    stream_.write(reinterpret_cast<const char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
    dataBytes_ += bytes.size();
  }

  void finish() override {
    if (finished_) return;
    finished_ = true;
    std::uint8_t size[4];
    writeLe32(size, static_cast<std::uint32_t>(36 + dataBytes_));
    stream_.seekp(4);
    stream_.write(reinterpret_cast<const char*>(size), 4);
    writeLe32(size, static_cast<std::uint32_t>(dataBytes_));
    stream_.seekp(40);
    stream_.write(reinterpret_cast<const char*>(size), 4);
    stream_.flush();
    const bool ok = static_cast<bool>(stream_);
    stream_.close();
    if (!ok) throw shared::CoreError(api::ERROR_SAVE_FAILED, "录音文件写入失败：" + display_);
  }

 private:
  static constexpr std::size_t HEADER_SIZE = 44;

  std::ofstream stream_;
  std::string display_;
  std::uint64_t dataBytes_{0};
  bool finished_{false};
};

}  // namespace

DiskTapeStore::DiskTapeStore(fs::path ffmpegPath, std::shared_ptr<platform::ErrorLog> errors) : ffmpegPath_(std::move(ffmpegPath)), errors_(std::move(errors)) {}

std::unique_ptr<hw::TapeSource> DiskTapeStore::open(const std::string& path) const {
  const fs::path file = toPath(path);
  std::error_code error;
  const auto status = fs::status(file, error);
  if (error || !fs::exists(status)) throw shared::CoreError(api::ERROR_FILE_NOT_FOUND, "找不到文件：" + path);
  if (!fs::is_regular_file(status)) throw shared::CoreError(api::ERROR_FILE_UNREADABLE, "不是普通文件：" + path);
  if (ffmpegPath_.empty() || !fs::exists(ffmpegPath_, error)) throw shared::CoreError(api::ERROR_DECODER_MISSING, "没有找到解码用的 ffmpeg 程序，无法载入磁带");
  const Probe info = probe(ffmpegPath_, toUtf8(file));
  return std::make_unique<FfmpegTapeSource>(ffmpegPath_, toUtf8(file), info, errors_);
}

std::unique_ptr<hw::TapeSink> DiskTapeStore::create(const std::string& path) const { return std::make_unique<WavTapeSink>(toPath(path), path); }

}  // namespace ember::infra::media
