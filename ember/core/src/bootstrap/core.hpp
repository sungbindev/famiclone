/*
 * ------------------------------------------------------------------------
 * 名称：core.hpp
 * 说明：核心入口：启动（创建日志、登记全部模块）、在线程池中处理请求、取消请求、转发事件、停止。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-13
 * 备注：参数与结果都是 JSON 文本；错误以 {"code","message"} 返回，异常不越过本类。binding 只经本类使用核心。
 * ------------------------------------------------------------------------
 */

#pragma once

#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

namespace ember::platform {
class CancellationToken;
class ErrorLog;
class RequestRouter;
class ThreadPool;
}  // namespace ember::platform

namespace ember::bootstrap {

class CoreEvents;

/** 核心入口。 */
class Core {
 public:
  /** 请求完成：ok 为 true 时 payload 是结果 JSON，否则是 {"code","message"}。可能在任意线程调用。 */
  using RequestDone = std::function<void(bool ok, std::string payload)>;
  /** 事件：事件名与数据 JSON。可能在任意线程调用。 */
  using EventHandler = std::function<void(const std::string& name, const std::string& dataJson)>;

  Core();
  ~Core();

  Core(const Core&) = delete;
  Core& operator=(const Core&) = delete;

  /**
   * 启动核心。
   * @param optionsJson {"userDataPath": 用户数据目录, "appVersion": 应用版本, "workerCount"?: 工作线程数}。
   * @returns {"coreVersion", "requests": [全部请求名]} 的 JSON 文本。
   * @throws shared::CoreError 参数不合格或已启动；std::exception 创建日志等失败。
   */
  std::string start(const std::string& optionsJson);

  /** 设置事件处理函数；应在 start 之前调用。 */
  void setEventHandler(EventHandler handler);

  /** 处理请求；结果经 done 返回，未启动或已停止时立即以 core.unavailable 返回。 */
  void request(const std::string& id, const std::string& name, const std::string& paramsJson, RequestDone done);

  /** 取消进行中的请求；请求已结束时忽略。 */
  void cancel(const std::string& id);

  /** 停止接受请求，等待进行中的请求结束，关闭日志。 */
  void stop();

 private:
  void run(const std::string& id, const std::string& name, const std::string& paramsJson, const platform::CancellationToken& token, const RequestDone& done);

  std::mutex mutex_;
  bool started_ = false;
  bool stopping_ = false;
  std::map<std::string, std::shared_ptr<platform::CancellationToken>> running_;
  std::shared_ptr<CoreEvents> events_;
  std::shared_ptr<platform::ErrorLog> errors_;
  std::unique_ptr<platform::RequestRouter> router_;
  std::unique_ptr<platform::ThreadPool> pool_;
  std::vector<std::function<void()>> stopHandlers_;
};

}  // namespace ember::bootstrap
