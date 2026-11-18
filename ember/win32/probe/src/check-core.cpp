/*
 * ------------------------------------------------------------------------
 * 名称：check-core.cpp
 * 说明：核心检查：在进程内启动核心，请求一次运行状态，再停止核心。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-19
 * 备注：核心用临时目录做用户数据目录，检查结束后删除；请求的结果在核心的线程池里返回，这里最多等 10 秒。
 * ------------------------------------------------------------------------
 */

#include "probe-check.hpp"

#include <chrono>
#include <exception>
#include <filesystem>
#include <future>
#include <string>

#include <nlohmann/json.hpp>

#include "bootstrap/core.hpp"

namespace ember::win32::probe {

namespace {

/** 等待请求结果的最长时间。 */
constexpr std::chrono::seconds REQUEST_TIMEOUT{10};

/** 创建临时的用户数据目录。 */
std::filesystem::path makeUserDataDirectory() {
  std::error_code error;
  const auto directory = std::filesystem::temp_directory_path(error) / ("ember-probe-" + std::to_string(GetCurrentProcessId()));
  std::filesystem::create_directories(directory, error);
  return directory;
}

}  // namespace

void checkCore(CheckResults& results) {
  const auto userData = makeUserDataDirectory();
  {
    ember::bootstrap::Core core;
    core.setEventHandler([](const std::string&, const std::string&) {});
    std::string startResult;
    try {
      const auto path = userData.u8string();
      const auto options = nlohmann::json{{"userDataPath", std::string(path.begin(), path.end())}, {"appVersion", "probe"}};
      startResult = core.start(options.dump());
    } catch (const std::exception& error) {
      results.push_back({"核心启动", CheckLevel::Fail, std::string("启动失败：") + error.what()});
      std::error_code ignored;
      std::filesystem::remove_all(userData, ignored);
      return;
    }
    const auto started = nlohmann::json::parse(startResult, nullptr, false);
    const std::string version = started.is_object() && started.contains("coreVersion") ? started["coreVersion"].get<std::string>() : "未知";
    const std::size_t requestCount = started.is_object() && started.contains("requests") ? started["requests"].size() : 0;
    results.push_back({"核心启动", CheckLevel::Pass, "核心版本 " + version + "，登记了 " + std::to_string(requestCount) + " 个请求"});

    std::promise<std::pair<bool, std::string>> done;
    auto reply = done.get_future();
    core.request("probe-1", "runtime.status", "{}", [&done](bool ok, std::string payload) { done.set_value({ok, std::move(payload)}); });
    if (reply.wait_for(REQUEST_TIMEOUT) != std::future_status::ready) {
      results.push_back({"核心请求", CheckLevel::Fail, "10 秒内没有收到 runtime.status 的结果"});
    } else {
      const auto [ok, payload] = reply.get();
      results.push_back({"核心请求", ok ? CheckLevel::Pass : CheckLevel::Fail, ok ? "runtime.status 返回 " + payload : "runtime.status 失败：" + payload});
    }
    core.stop();
    results.push_back({"核心停止", CheckLevel::Pass, "已停止"});
  }
  std::error_code ignored;
  std::filesystem::remove_all(userData, ignored);
}

}  // namespace ember::win32::probe
