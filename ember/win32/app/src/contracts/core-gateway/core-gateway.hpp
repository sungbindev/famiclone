/*
 * ------------------------------------------------------------------------
 * 名称：core-gateway.hpp
 * 说明：核心网关的抽象：外壳经它启动与停止核心、向核心发请求、订阅核心事件与状态变化、发送输入；回调都在界面线程。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-19
 * 备注：实现把核心在其他线程发出的结果与事件投递回界面线程；
 *       请求的参数与结果都是 JSON，失败时给出错误码与说明（核心的错误码、core.* 的通用错误码或外壳自己的错误码）。
 * ------------------------------------------------------------------------
 */

#pragma once

#include <functional>
#include <string>
#include <vector>

#include <nlohmann/json.hpp>

namespace ember::win32::contracts {

/** 请求的错误：错误码以“模块名.”开头，通用错误码以 core. 开头。 */
struct CoreError {
  std::string code;
  std::string message;
};

/** 请求的回复。 */
struct CoreReply {
  bool ok{false};
  /** ok 时的结果。 */
  nlohmann::json result;
  /** 失败时的错误。 */
  CoreError error;

  /** 成功的回复。 */
  [[nodiscard]] static CoreReply success(nlohmann::json result) { return CoreReply{true, std::move(result), {}}; }
  /** 失败的回复。 */
  [[nodiscard]] static CoreReply failure(std::string code, std::string message) { return CoreReply{false, nullptr, CoreError{std::move(code), std::move(message)}}; }
};

/** 请求的回复处理，在界面线程调用。 */
using CoreReplyHandler = std::function<void(CoreReply reply)>;

/** 核心的运行状态。 */
enum class CoreState {
  /** 还没有启动，或已停止。 */
  Stopped,
  /** 正在启动。 */
  Starting,
  /** 已就绪，可以发请求。 */
  Running,
  /** 启动失败。 */
  Failed
};

/** 核心的状态。 */
struct CoreStatus {
  CoreState state{CoreState::Stopped};
  /** 核心版本，就绪后有值。 */
  std::string coreVersion;
  /** 核心登记的全部请求名，就绪后有值。 */
  std::vector<std::string> requests;
  /** 启动失败的原因。 */
  std::string message;
};

/** 核心事件的处理：事件名与数据，在界面线程调用。 */
using CoreEventHandler = std::function<void(const std::string& name, const nlohmann::json& data)>;

/** 核心状态变化的处理，在界面线程调用。 */
using CoreStatusHandler = std::function<void(const CoreStatus& status)>;

/** 核心网关。 */
class CoreGateway {
 public:
  virtual ~CoreGateway() = default;

  /** 启动核心；成功或失败都经状态变化通知。 */
  virtual void start() = 0;

  /** 停止核心，等待进行中的请求结束；之后的请求立即以 core.unavailable 失败。 */
  virtual void stop() = 0;

  /** 当前状态。 */
  [[nodiscard]] virtual CoreStatus status() const = 0;

  /** 向核心发请求；回复经 done 交回界面线程，核心不可用时以 core.unavailable 失败。 */
  virtual void request(const std::string& name, nlohmann::json params, CoreReplyHandler done) = 0;

  /**
   * 把用户输入的原始事件交给核心，不等待执行；核心不可用时丢弃。
   * @param message 输入消息，如 {"type":"key","code":"KeyW","down":true}。
   */
  virtual void sendInput(const nlohmann::json& message) = 0;

  /** 订阅核心事件；返回取消订阅的函数。 */
  [[nodiscard]] virtual std::function<void()> onEvent(CoreEventHandler handler) = 0;

  /** 订阅核心状态变化；返回取消订阅的函数。 */
  [[nodiscard]] virtual std::function<void()> onStatusChange(CoreStatusHandler handler) = 0;
};

}  // namespace ember::win32::contracts
