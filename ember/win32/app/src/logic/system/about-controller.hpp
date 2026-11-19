/*
 * ------------------------------------------------------------------------
 * 名称：about-controller.hpp
 * 说明：“关于 Ember”对话框的控制器：给出对话框要显示的固定文字、编译时间与第三方依赖清单，经核心的 system.version 读取应用版本与核心版本（核心就绪后读取，核心重启后重新读取），打开项目地址。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-21
 * 备注：版本读不到时显示“读取失败”，不用通知；打开项目地址失败用外壳通知给出原因；规则见 docs/05-界面.md 第 8 节。
 * ------------------------------------------------------------------------
 */

#pragma once

#include <cstdint>
#include <functional>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include "contracts/core-gateway/core-gateway.hpp"
#include "contracts/shell/request-broker.hpp"
#include "contracts/shell/shell-registry.hpp"
#include "contracts/system/about-model.hpp"
#include "contracts/system/external-links.hpp"

namespace ember::win32::logic {

/** 关于对话框的控制器。 */
class AboutController final : public contracts::AboutModel {
 public:
  /**
   * @param broker 请求中转：读取版本的 system.version 经它。
   * @param core 核心网关：订阅核心运行状态。
   * @param links 打开项目地址。
   * @param notices 第三方依赖清单（构建脚本生成）。
   * @param buildTime 编译时间（构建时确定）。
   */
  AboutController(contracts::RequestBroker& broker, contracts::CoreGateway& core, contracts::ExternalLinks& links, std::vector<contracts::ThirdPartyNotice> notices, std::string buildTime);
  ~AboutController() override;
  AboutController(const AboutController&) = delete;
  AboutController& operator=(const AboutController&) = delete;

  /**
   * 接上外壳：订阅核心运行状态并读取版本。
   * @returns 断开的函数。
   */
  std::function<void()> attach(contracts::ShellApi& shell);

  // ---- AboutModel ----
  [[nodiscard]] contracts::AboutView view() const override;
  void openProject() override;
  [[nodiscard]] std::function<void()> subscribe(std::function<void()> handler) override;

 private:
  void load();
  void changed();

  contracts::RequestBroker& broker_;
  contracts::CoreGateway& core_;
  contracts::ExternalLinks& links_;
  std::vector<contracts::ThirdPartyNotice> notices_;
  std::string buildTime_;
  contracts::ShellApi* shell_{nullptr};
  std::function<void()> stopStatus_;
  std::shared_ptr<bool> alive_{std::make_shared<bool>(true)};
  std::uint64_t generation_{0};
  std::uint64_t loadRequest_{0};
  contracts::AboutVersionLoad load_{contracts::AboutVersionLoad::Loading};
  std::string appVersion_;
  std::string coreVersion_;
  std::map<int, std::function<void()>> listeners_;
  int nextListener_{1};
};

}  // namespace ember::win32::logic
