/*
 * ------------------------------------------------------------------------
 * 名称：ui-thread.hpp
 * 说明：界面线程的任务投递：任意线程把函数投递到界面线程执行（核心的事件、请求结果、帧通知都经它回到界面线程），界面线程可以延时执行函数（文件防抖写盘等）。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-19
 * 备注：在界面线程创建；用一个只接收消息的隐藏窗口承载，任务经窗口消息在消息循环里执行，所以必须有消息循环（测试里用 pumpMessages）；
 *       析构时尚未执行的任务丢弃。
 * ------------------------------------------------------------------------
 */

#pragma once

#include <functional>
#include <map>
#include <mutex>
#include <vector>

namespace ember::win32::platform {

/** 界面线程的任务投递。 */
class UiThread {
 public:
  /** 在当前线程创建，当前线程成为界面线程。 */
  UiThread();
  ~UiThread();
  UiThread(const UiThread&) = delete;
  UiThread& operator=(const UiThread&) = delete;

  /** 把任务投递到界面线程，可以在任意线程调用。 */
  void post(std::function<void()> task);

  /**
   * 延时执行一次任务；只能在界面线程调用。
   * @param milliseconds 延时。
   * @returns 任务编号，供 cancelDelayed 使用。
   */
  unsigned postDelayed(unsigned milliseconds, std::function<void()> task);

  /** 取消还没执行的延时任务；编号不存在时无效；只能在界面线程调用。 */
  void cancelDelayed(unsigned id);

  /** 当前线程是不是界面线程。 */
  [[nodiscard]] bool isCurrent() const;

  /** 处理当前线程消息队列里已有的全部消息后返回（测试用，不等待新消息）。 */
  static void pumpMessages();

 private:
  struct Impl;
  void* window_{nullptr};
  unsigned long threadId_{0};
  std::mutex mutex_;
  std::vector<std::function<void()>> queue_;
  std::map<unsigned, std::function<void()>> delayed_;
  unsigned nextId_{1};

  friend struct Impl;
};

}  // namespace ember::win32::platform
