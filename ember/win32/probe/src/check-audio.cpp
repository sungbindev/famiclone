/*
 * ------------------------------------------------------------------------
 * 名称：check-audio.cpp
 * 说明：声音检查：用核心里的系统声音输出（miniaudio）打开默认播放设备。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-19
 * 备注：没有声音设备时核心会降级为空输出，不影响运行，所以失败只算提示。
 * ------------------------------------------------------------------------
 */

#include "probe-check.hpp"

#include "platform/audio/miniaudio-output.hpp"

namespace ember::win32::probe {

namespace {

/** 探针打开设备用的采样率（赫兹）。 */
constexpr unsigned SAMPLE_RATE = 48000;

}  // namespace

void checkAudio(CheckResults& results) {
  std::string failure;
  const auto output = ember::platform::MiniaudioOutput::open(SAMPLE_RATE, failure);
  if (output == nullptr) {
    results.push_back({"系统声音输出", CheckLevel::Info, "打不开默认播放设备，核心会改用空输出：" + failure});
    return;
  }
  results.push_back({"系统声音输出", CheckLevel::Pass, "默认播放设备已打开"});
}

}  // namespace ember::win32::probe
