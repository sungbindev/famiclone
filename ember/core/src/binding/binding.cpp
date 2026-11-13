/*
 * ------------------------------------------------------------------------
 * 名称：binding.cpp
 * 说明：Node-API 绑定：导出 start、request、cancel、onEvent、stop，只做 JavaScript 值与核心消息的转换。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-13
 * 备注：请求在核心的线程池中执行，结果与事件经线程安全函数回到 JavaScript 线程；
 *       失败的请求以带 code 属性的 Error 拒绝；C++ 异常不越过本文件。一个进程只加载一个核心。
 * ------------------------------------------------------------------------
 */

#include <map>
#include <memory>
#include <string>
#include <utility>

#include <napi.h>
#include <nlohmann/json.hpp>

#include "bootstrap/core.hpp"
#include "shared/errors/core-error.hpp"

namespace {

using ember::bootstrap::Core;

/** 一次请求的结果。 */
struct Response {
  std::string id;
  bool ok = false;
  std::string payload;
};

/** 一个事件。 */
struct Event {
  std::string name;
  std::string data;
};

std::unique_ptr<Core> core;
Napi::ThreadSafeFunction responses;
Napi::ThreadSafeFunction events;
std::map<std::string, Napi::Promise::Deferred> pending;
bool stopping = false;

/** 带错误码的 JavaScript 错误。 */
Napi::Error codeError(Napi::Env env, const std::string& code, const std::string& message) {
  auto error = Napi::Error::New(env, message);
  error.Set("code", Napi::String::New(env, code));
  return error;
}

/** 读取字符串参数。 */
std::string stringArgument(const Napi::CallbackInfo& info, std::size_t index, const char* name) {
  if (info.Length() <= index || !info[index].IsString()) {
    throw codeError(info.Env(), std::string(ember::shared::ERROR_INVALID_PARAMS), std::string(name) + " 应为字符串");
  }
  return info[index].As<Napi::String>().Utf8Value();
}

/** 在 JavaScript 线程上完成请求的 Promise。 */
void settle(Napi::Env env, Napi::Function, Response* response) {
  std::unique_ptr<Response> owned(response);
  if (env == nullptr) return;
  const auto it = pending.find(owned->id);
  if (it == pending.end()) return;
  auto deferred = it->second;
  pending.erase(it);
  if (owned->ok) {
    deferred.Resolve(Napi::String::New(env, owned->payload));
    return;
  }
  const auto error = nlohmann::json::parse(owned->payload, nullptr, false);
  const std::string code = error.is_object() ? error.value("code", std::string(ember::shared::ERROR_INTERNAL)) : std::string(ember::shared::ERROR_INTERNAL);
  const std::string message = error.is_object() ? error.value("message", std::string("核心内部错误")) : std::string("核心内部错误");
  deferred.Reject(codeError(env, code, message).Value());
}

/** 在 JavaScript 线程上调用事件回调。 */
void emit(Napi::Env env, Napi::Function callback, Event* event) {
  std::unique_ptr<Event> owned(event);
  if (env == nullptr || callback.IsEmpty()) return;
  callback.Call({Napi::String::New(env, owned->name), Napi::String::New(env, owned->data)});
}

Napi::Value start(const Napi::CallbackInfo& info) {
  auto env = info.Env();
  if (core != nullptr) throw codeError(env, std::string(ember::shared::ERROR_UNAVAILABLE), "核心已启动过");
  const auto options = stringArgument(info, 0, "options");
  auto created = std::make_unique<Core>();
  if (events) {
    created->setEventHandler([](const std::string& name, const std::string& data) {
      events.BlockingCall(new Event{name, data}, emit);
    });
  }
  std::string started;
  try {
    started = created->start(options);
  } catch (const ember::shared::CoreError& error) {
    throw codeError(env, error.code(), error.what());
  } catch (const std::exception& error) {
    throw codeError(env, std::string(ember::shared::ERROR_INTERNAL), std::string("核心启动失败：") + error.what());
  }
  responses = Napi::ThreadSafeFunction::New(env, Napi::Function::New(env, [](const Napi::CallbackInfo&) {}), "ember-core-responses", 0, 1);
  responses.Unref(env);
  core = std::move(created);
  return Napi::String::New(env, started);
}

Napi::Value request(const Napi::CallbackInfo& info) {
  auto env = info.Env();
  const auto id = stringArgument(info, 0, "id");
  const auto name = stringArgument(info, 1, "name");
  const auto params = stringArgument(info, 2, "params");
  auto deferred = Napi::Promise::Deferred::New(env);
  if (core == nullptr || stopping) {
    deferred.Reject(codeError(env, std::string(ember::shared::ERROR_UNAVAILABLE), "核心未启动或正在停止").Value());
    return deferred.Promise();
  }
  if (pending.count(id) > 0) {
    deferred.Reject(codeError(env, std::string(ember::shared::ERROR_INVALID_PARAMS), "请求标识重复：" + id).Value());
    return deferred.Promise();
  }
  pending.emplace(id, deferred);
  core->request(id, name, params, [id](bool ok, std::string payload) {
    responses.BlockingCall(new Response{id, ok, std::move(payload)}, settle);
  });
  return deferred.Promise();
}

Napi::Value cancel(const Napi::CallbackInfo& info) {
  const auto id = stringArgument(info, 0, "id");
  if (core != nullptr) core->cancel(id);
  return info.Env().Undefined();
}

Napi::Value onEvent(const Napi::CallbackInfo& info) {
  auto env = info.Env();
  if (info.Length() < 1 || !info[0].IsFunction()) throw codeError(env, std::string(ember::shared::ERROR_INVALID_PARAMS), "callback 应为函数");
  if (core != nullptr) throw codeError(env, std::string(ember::shared::ERROR_UNAVAILABLE), "须在启动核心之前订阅事件");
  if (events) events.Release();
  events = Napi::ThreadSafeFunction::New(env, info[0].As<Napi::Function>(), "ember-core-events", 0, 1);
  events.Unref(env);
  return env.Undefined();
}

/** 在后台线程停止核心，结束后在 JavaScript 线程释放线程安全函数。 */
class StopWorker final : public Napi::AsyncWorker {
 public:
  StopWorker(Napi::Env env, Napi::Promise::Deferred deferred) : Napi::AsyncWorker(env), deferred_(std::move(deferred)) {}

  void Execute() override { core->stop(); }

  void OnOK() override {
    core.reset();
    if (responses) responses.Release();
    if (events) events.Release();
    deferred_.Resolve(Env().Undefined());
  }

 private:
  Napi::Promise::Deferred deferred_;
};

Napi::Value stop(const Napi::CallbackInfo& info) {
  auto env = info.Env();
  auto deferred = Napi::Promise::Deferred::New(env);
  if (core == nullptr || stopping) {
    deferred.Resolve(env.Undefined());
    return deferred.Promise();
  }
  stopping = true;
  (new StopWorker(env, deferred))->Queue();
  return deferred.Promise();
}

Napi::Object init(Napi::Env env, Napi::Object exports) {
  exports.Set("start", Napi::Function::New(env, start, "start"));
  exports.Set("request", Napi::Function::New(env, request, "request"));
  exports.Set("cancel", Napi::Function::New(env, cancel, "cancel"));
  exports.Set("onEvent", Napi::Function::New(env, onEvent, "onEvent"));
  exports.Set("stop", Napi::Function::New(env, stop, "stop"));
  return exports;
}

}  // namespace

NODE_API_MODULE(ember_core, init)
