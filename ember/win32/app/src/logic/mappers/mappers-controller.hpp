/*
 * ------------------------------------------------------------------------
 * 名称：mappers-controller.hpp
 * 说明：mapper 插件模块的控制器：核心就绪后读取全部插件，订阅 mappers.changed 保持列表最新，核心重启后重新读取；选中一个插件时读取它的清单详情；执行搜索与来源筛选、安装、卸载（先询问）、启用与停用。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-21
 * 备注：启用与停用以核心返回的摘要为准，失败时列表保持原样并用通知给出原因；插件被卸载后选中项自动清除；规则见 docs/05-界面.md 第 4.4 节。
 * ------------------------------------------------------------------------
 */

#pragma once

#include <cstdint>
#include <functional>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "contracts/core-gateway/core-gateway.hpp"
#include "contracts/mappers/mappers-model.hpp"
#include "contracts/shell/confirmation.hpp"
#include "contracts/shell/request-broker.hpp"
#include "contracts/shell/shell-registry.hpp"
#include "logic/mappers/mappers-rules.hpp"

namespace ember::win32::logic {

/** mapper 插件模块的控制器。 */
class MappersController final : public contracts::MappersModel {
 public:
  /**
   * @param broker 请求中转：核心的请求与外壳自己处理的请求（选择插件包）都经它。
   * @param core 核心网关：订阅核心事件与核心运行状态。
   * @param confirmation 卸载前的确认询问。
   */
  MappersController(contracts::RequestBroker& broker, contracts::CoreGateway& core, contracts::Confirmation& confirmation);
  ~MappersController() override;
  MappersController(const MappersController&) = delete;
  MappersController& operator=(const MappersController&) = delete;

  /**
   * 接上外壳：订阅核心事件与核心运行状态，并读取全部插件。
   * @returns 断开的函数。
   */
  std::function<void()> attach(contracts::ShellApi& shell);

  // ---- MappersModel ----
  [[nodiscard]] contracts::MappersView view() const override;
  void setSearch(const std::string& text) override;
  void setSource(const std::string& source) override;
  void toggleSelect(const std::string& id) override;
  void install() override;
  void uninstall(const std::string& id) override;
  void setEnabled(const std::string& id, bool enabled) override;
  [[nodiscard]] std::function<void()> subscribe(std::function<void()> handler) override;

 private:
  /** 一次详情读取的结果。 */
  struct DetailResult {
    std::string id;
    std::optional<MapperDetail> detail;
    std::string message;
  };

  void load();
  void loadDetail();
  void handleEvent(const std::string& name);
  void handleCoreStatus(const contracts::CoreStatus& status);
  void report(const std::string& text);
  void notifySuccess(const std::string& text);
  void changed();
  /** 请求：回复只在仍接着外壳时处理。 */
  void ask(const std::string& name, nlohmann::json params, std::function<void(const contracts::CoreReply&)> done);
  [[nodiscard]] const MapperSummary* find(const std::string& id) const;
  /** 用新的摘要替换列表里同标识的一项，没有时加进去并按标识排序。 */
  void put(const MapperSummary& plugin);

  contracts::RequestBroker& broker_;
  contracts::CoreGateway& core_;
  contracts::Confirmation& confirmation_;
  contracts::ShellApi* shell_{nullptr};
  std::vector<std::function<void()>> stops_;
  /** 控制器存在期间为 true：回复到达时控制器已经销毁就不处理。 */
  std::shared_ptr<bool> alive_{std::make_shared<bool>(true)};
  /** 每次接上外壳加一：上一次接上时发出的请求，回复到了也不处理。 */
  std::uint64_t generation_{0};
  /** 最近一次读取列表与详情的编号：迟到的旧结果不覆盖新结果。 */
  std::uint64_t loadRequest_{0};
  std::uint64_t detailRequest_{0};

  contracts::MappersLoad load_{contracts::MappersLoad::Loading};
  std::string message_;
  std::vector<MapperSummary> plugins_;
  std::string search_;
  std::string source_{contracts::kMapperSourceAll};
  std::string chosenId_;
  std::optional<DetailResult> detail_;
  std::map<int, std::function<void()>> listeners_;
  int nextListener_{1};
};

}  // namespace ember::win32::logic
