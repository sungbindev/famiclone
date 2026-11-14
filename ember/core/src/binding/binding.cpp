/*
 * ------------------------------------------------------------------------
 * 名称：binding.cpp
 * 说明：Node-API 绑定：导出 start、request、cancel、onEvent、onFrame、sendInput、stop，只做 JavaScript 值与核心消息的转换。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-13
 * 备注：请求在核心的线程池中执行，结果与事件经线程安全函数回到 JavaScript 线程；
 *       失败的请求以带 code 属性的 Error 拒绝；C++ 异常不越过本文件。一个进程只加载一个核心。
 *       帧回调：在 JavaScript 线程上预建 3 个 ArrayBuffer（Electron 不允许外部 ArrayBuffer），模拟线程把帧直接拷贝到空闲的槽里（唯一的一次拷贝），
 *       再经线程安全函数通知 JavaScript 线程；回调返回后槽释放复用，没有空闲槽（JavaScript 处理不过来）时丢弃这一帧。
 *       回调拿到的 ArrayBuffer 只在回调期间有效，不能保存，也不能放进转移列表（转移会使槽失效）。
 * ------------------------------------------------------------------------
 */

#include <array>
#include <cstdint>
#include <cstring>
#include <map>
#include <memory>
#include <mutex>
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

/** 帧槽的个数：一个在 JavaScript 里处理，一个等待处理，一个让模拟线程写。 */
constexpr std::size_t FRAME_SLOT_COUNT = 3;
/** 槽的初始大小：经典 FC 的画面，256x240，每像素 4 字节。 */
constexpr std::size_t INITIAL_FRAME_BYTES = 256U * 240U * 4U;

/** 一个帧槽：由 JavaScript 线程创建的 ArrayBuffer，模拟线程直接写它的内存。 */
struct FrameSlot {
  Napi::Reference<Napi::ArrayBuffer> buffer;
  std::uint8_t* data = nullptr;
  /** 正在被模拟线程写、或已通知 JavaScript 但还没处理完。 */
  bool busy = false;
};

/** 帧槽的共享状态，互斥锁保护。 */
struct FrameSlots {
  std::mutex mutex;
  std::array<FrameSlot, FRAME_SLOT_COUNT> slots;
  /** 每个槽的字节数，0 表示还没有创建。 */
  std::size_t bytes = 0;
  /** 重建槽的次数；通知带着它，重建之前发出的通知作废。 */
  std::uint64_t generation = 0;
  /** 已经请求 JavaScript 线程重建槽（画面尺寸变了）。 */
  bool resizePending = false;
};

/** 给 JavaScript 线程的通知：一帧已写好，或者槽需要重建。 */
struct FrameNotice {
  bool resize = false;
  std::size_t slot = 0;
  std::uint64_t generation = 0;
  std::uint32_t width = 0;
  std::uint32_t height = 0;
  std::uint64_t number = 0;
  /** 重建时的新字节数。 */
  std::size_t bytes = 0;
};

std::unique_ptr<Core> core;
Napi::ThreadSafeFunction responses;
Napi::ThreadSafeFunction events;
Napi::ThreadSafeFunction frames;
FrameSlots frameSlots;
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

/** 在 JavaScript 线程上重建帧槽：每个槽一个新的 ArrayBuffer，旧的交给垃圾回收。 */
void rebuildFrameSlots(Napi::Env env, std::size_t bytes) {
  std::lock_guard lock(frameSlots.mutex);
  for (auto& slot : frameSlots.slots) {
    auto buffer = Napi::ArrayBuffer::New(env, bytes);
    slot.data = static_cast<std::uint8_t*>(buffer.Data());
    slot.buffer = Napi::Persistent(buffer);
    slot.busy = false;
  }
  frameSlots.bytes = bytes;
  frameSlots.resizePending = false;
  ++frameSlots.generation;
}

/** 释放帧槽里的 ArrayBuffer，只能在 JavaScript 线程上调用。 */
void releaseFrameSlots() {
  std::lock_guard lock(frameSlots.mutex);
  for (auto& slot : frameSlots.slots) {
    slot.buffer.Reset();
    slot.data = nullptr;
    slot.busy = false;
  }
  frameSlots.bytes = 0;
  ++frameSlots.generation;
}

/** 在 JavaScript 线程上处理通知：重建槽，或把写好的帧交给回调，回调返回后释放槽。 */
void deliverFrame(Napi::Env env, Napi::Function callback, FrameNotice* raw) {
  std::unique_ptr<FrameNotice> notice(raw);
  if (env == nullptr) return;
  if (notice->resize) {
    rebuildFrameSlots(env, notice->bytes);
    return;
  }
  Napi::ArrayBuffer buffer;
  {
    std::lock_guard lock(frameSlots.mutex);
    if (notice->generation != frameSlots.generation) return;
    buffer = frameSlots.slots[notice->slot].buffer.Value();
  }
  try {
    if (!callback.IsEmpty()) {
      callback.Call({buffer, Napi::Number::New(env, notice->width), Napi::Number::New(env, notice->height), Napi::Number::New(env, static_cast<double>(notice->number))});
    }
  } catch (const Napi::Error&) {
    // 回调抖出的错误不影响后面的帧。
  }
  std::lock_guard lock(frameSlots.mutex);
  if (notice->generation == frameSlots.generation) frameSlots.slots[notice->slot].busy = false;
}

/** 在模拟线程上调用：把一帧拷贝到空闲的槽，再通知 JavaScript 线程；不阻塞，没有空闲槽时丢弃。 */
void publishFrame(const ember::platform::FrameView& frame) {
  const std::size_t bytes = frame.pixels.size_bytes();
  auto notice = std::make_unique<FrameNotice>();
  std::size_t slotIndex = 0;
  {
    std::lock_guard lock(frameSlots.mutex);
    if (bytes != frameSlots.bytes) {
      // 画面尺寸变了：让 JavaScript 线程重建槽，这一帧丢弃。
      if (frameSlots.resizePending || bytes == 0) return;
      frameSlots.resizePending = true;
      notice->resize = true;
      notice->bytes = bytes;
    } else {
      while (slotIndex < FRAME_SLOT_COUNT && frameSlots.slots[slotIndex].busy) ++slotIndex;
      if (slotIndex == FRAME_SLOT_COUNT) return;
      auto& slot = frameSlots.slots[slotIndex];
      slot.busy = true;
      std::memcpy(slot.data, frame.pixels.data(), bytes);
      notice->slot = slotIndex;
      notice->generation = frameSlots.generation;
      notice->width = frame.width;
      notice->height = frame.height;
      notice->number = frame.number;
    }
  }
  const bool resize = notice->resize;
  const std::uint64_t generation = notice->generation;
  if (frames.NonBlockingCall(notice.get(), deliverFrame) == napi_ok) {
    static_cast<void>(notice.release());
    return;
  }
  // 线程安全函数已关闭（正在停止）：把槽还回去。
  std::lock_guard lock(frameSlots.mutex);
  if (resize) frameSlots.resizePending = false;
  else if (generation == frameSlots.generation) frameSlots.slots[slotIndex].busy = false;
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
  if (frames) created->setFrameHandler(publishFrame);
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

Napi::Value onFrame(const Napi::CallbackInfo& info) {
  auto env = info.Env();
  if (info.Length() < 1 || !info[0].IsFunction()) throw codeError(env, std::string(ember::shared::ERROR_INVALID_PARAMS), "callback 应为函数");
  if (core != nullptr) throw codeError(env, std::string(ember::shared::ERROR_UNAVAILABLE), "须在启动核心之前订阅画面");
  if (frames) {
    frames.Release();
    releaseFrameSlots();
  }
  frames = Napi::ThreadSafeFunction::New(env, info[0].As<Napi::Function>(), "ember-core-frames", 0, 1);
  frames.Unref(env);
  rebuildFrameSlots(env, INITIAL_FRAME_BYTES);
  return env.Undefined();
}

Napi::Value sendInput(const Napi::CallbackInfo& info) {
  auto env = info.Env();
  if (info.Length() < 1 || !info[0].IsObject()) throw codeError(env, std::string(ember::shared::ERROR_INVALID_PARAMS), "输入消息应为对象");
  if (core == nullptr || stopping) throw codeError(env, std::string(ember::shared::ERROR_UNAVAILABLE), "核心未启动或正在停止");
  // 消息的校验在核心里做，这里只把对象转成 JSON 文本。
  const Napi::Object json = env.Global().Get("JSON").As<Napi::Object>();
  const Napi::Value text = json.Get("stringify").As<Napi::Function>().Call(json, {info[0]});
  if (!text.IsString()) throw codeError(env, std::string(ember::shared::ERROR_INVALID_PARAMS), "输入消息无法转成 JSON");
  try {
    core->sendInput(text.As<Napi::String>().Utf8Value());
  } catch (const ember::shared::CoreError& error) {
    throw codeError(env, error.code(), error.what());
  } catch (const std::exception& error) {
    throw codeError(env, std::string(ember::shared::ERROR_INTERNAL), error.what());
  }
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
    if (frames) {
      frames.Release();
      releaseFrameSlots();
    }
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
  exports.Set("onFrame", Napi::Function::New(env, onFrame, "onFrame"));
  exports.Set("sendInput", Napi::Function::New(env, sendInput, "sendInput"));
  exports.Set("stop", Napi::Function::New(env, stop, "stop"));
  return exports;
}

}  // namespace

NODE_API_MODULE(ember_core, init)
