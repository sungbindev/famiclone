/*
 * ------------------------------------------------------------------------
 * 名称：output.cpp
 * 说明：输出模块的登记：打开系统声音输出（失败时降级为空输出并记录错误日志），创建输出设置服务并登记 output.* 请求，把声音输出提供给运行模块。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-14
 * 备注：运行模块取用声音输出，须排在本模块之后登记；核心停止时系统设备随声音输出一起释放。
 * ------------------------------------------------------------------------
 */

#include "bootstrap/modules/output.hpp"

#include <exception>
#include <memory>
#include <string>

#include "bootstrap/machines/machine-catalog.hpp"
#include "contracts/output/output-service.hpp"
#include "logic/output/local-output-service.hpp"
#include "platform/audio/miniaudio-output.hpp"
#include "platform/audio/null-audio-output.hpp"
#include "requests/output/output-requests.hpp"

namespace ember::bootstrap {

std::shared_ptr<platform::AudioOutput> openAudioOutputOrFallback(const AudioOpener& opener, unsigned sampleRate, platform::ErrorLog& errors) {
  std::string failure;
  std::shared_ptr<platform::AudioOutput> device;
  try {
    device = opener(sampleRate, failure);
  } catch (const std::exception& error) {
    failure = error.what();
  }
  if (device != nullptr) return device;
  errors.write({"output", "output.deviceUnavailable", "无法打开系统声音设备，已降级为无声输出", {{"reason", failure}}});
  return std::make_shared<platform::NullAudioOutput>(sampleRate);
}

void registerOutput(ModuleRegistry& registry) {
  const AudioOpener opener = [](unsigned sampleRate, std::string& failure) -> std::shared_ptr<platform::AudioOutput> { return platform::MiniaudioOutput::open(sampleRate, failure); };
  auto audio = openAudioOutputOrFallback(opener, MachineCatalog::AUDIO_SAMPLE_RATE, *registry.errors);
  auto service = std::make_shared<logic::output::LocalOutputService>(audio);
  registry.services.provide<platform::AudioOutput>(audio);
  registry.services.provide<contracts::output::OutputService>(service);
  requests::output::addOutputRequests(registry.requests, service);
}

}  // namespace ember::bootstrap
