/*
 * ------------------------------------------------------------------------
 * 名称：thread-priority-test.cpp
 * 说明：线程优先级的测试：对象存活期间当前线程的优先级被提高，销毁后恢复。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-16
 * 备注：只在 Windows 上检查优先级；其他平台对象什么也不做。
 * ------------------------------------------------------------------------
 */

#include <gtest/gtest.h>

#include <thread>

#include "platform/time/thread-priority.hpp"

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#endif

namespace {

using ember::platform::ThreadPriorityBoost;

TEST(ThreadPriorityBoost, RaisesTheCurrentThreadWhileAliveAndRestoresIt) {
  // 在新线程里测，避免改动测试主线程的优先级。
  std::thread([] {
#ifdef _WIN32
    const int before = GetThreadPriority(GetCurrentThread());
    {
      const ThreadPriorityBoost boost;
      EXPECT_EQ(GetThreadPriority(GetCurrentThread()), THREAD_PRIORITY_HIGHEST);
    }
    EXPECT_EQ(GetThreadPriority(GetCurrentThread()), before);
#else
    const ThreadPriorityBoost boost;
    SUCCEED();
#endif
  }).join();
}

}  // namespace
