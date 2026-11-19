/*
 * ------------------------------------------------------------------------
 * 名称：single-instance.hpp
 * 说明：单实例：同一份用户数据同时只允许一个外壳进程运行，后启动的进程把窗口提前的请求交给已经在运行的进程后退出。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-19
 * 备注：做法与 Chromium 的 ProcessSingleton（Windows）完全相同，所以与 Electron 外壳互通，两个外壳不能同时运行：
 *       全局互斥量 Local\ChromeProcessSingletonStartup! 保护创建过程；用户数据目录下的 lockfile（独占打开、关闭时删除）保证同一份数据只有一个进程；
 *       只接收消息的窗口（类名 Chrome_MessageWindow，标题是用户数据目录的路径）接收后启动进程用 WM_COPYDATA 发来的 START 命令并回复 TRUE；
 *       必须在界面线程创建与使用，并且要有消息循环。
 * ------------------------------------------------------------------------
 */

#pragma once

#include <filesystem>
#include <functional>

namespace ember::win32::platform {

/** 取单实例的结果。 */
enum class SingleInstanceResult {
  /** 本进程是唯一的实例，继续启动。 */
  Acquired,
  /** 已有实例在运行，已经通知它把窗口提前，本进程应该退出。 */
  NotifiedExisting,
  /** 这份用户数据正在被别的进程使用，但通知不到它（它没有响应或锁文件建不出来），本进程应该提示后退出。 */
  InUse
};

/** 单实例。 */
class SingleInstance {
 public:
  /**
   * @param userDataDirectory 用户数据目录（与 Electron 外壳使用的路径写法相同）。
   * @param onSecondInstance 别的进程启动并通知本进程时调用（在界面线程）：把窗口提到前面。
   */
  SingleInstance(std::filesystem::path userDataDirectory, std::function<void()> onSecondInstance);
  ~SingleInstance();
  SingleInstance(const SingleInstance&) = delete;
  SingleInstance& operator=(const SingleInstance&) = delete;

  /** 取单实例：没有别的实例时创建锁与消息窗口；有时通知它。 */
  [[nodiscard]] SingleInstanceResult acquire();

 private:
  struct Impl;
  std::filesystem::path userDataDirectory_;
  std::function<void()> onSecondInstance_;
  void* lockFile_{nullptr};
  void* window_{nullptr};

  friend struct Impl;
};

}  // namespace ember::win32::platform
