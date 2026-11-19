/*
 * ------------------------------------------------------------------------
 * 名称：theme-sync.hpp
 * 说明：主题同步：核心就绪后读取首选项里的主题，之后每次变化都交给应用，界面的主题随之变化。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-19
 * 备注：主题设置项是 appearance.theme，取值 system（跟随系统）、dark、light，取值不认识时不改动；
 *       onFirst 在第一次读到并应用主题（或读取失败）时调用一次，主窗口显示前等它（带超时），启动时就不会先闪一下错误的颜色；核心重启后重新读取。
 * ------------------------------------------------------------------------
 */

#pragma once

#include <functional>
#include <string>

#include "contracts/core-gateway/core-gateway.hpp"

namespace ember::win32::entry {

/** 主题来源。 */
enum class ThemeSource { System, Dark, Light };

/** 主题同步。 */
class ThemeSync {
 public:
  /**
   * @param core 核心网关。
   * @param apply 应用主题来源。
   * @param onError 读取首选项失败时调用，不影响运行；可以为空。
   * @param onFirst 第一次读到并应用主题（或读取失败）时调用一次；可以为空。
   */
  ThemeSync(contracts::CoreGateway& core, std::function<void(ThemeSource)> apply, std::function<void(const std::string&)> onError, std::function<void()> onFirst);
  ~ThemeSync();
  ThemeSync(const ThemeSync&) = delete;
  ThemeSync& operator=(const ThemeSync&) = delete;

 private:
  void refresh();
  void settle();

  contracts::CoreGateway& core_;
  std::function<void(ThemeSource)> apply_;
  std::function<void(const std::string&)> onError_;
  std::function<void()> onFirst_;
  std::function<void()> stopEvents_;
  std::function<void()> stopStatus_;
  /** 回调里确认同步对象还在。 */
  std::shared_ptr<bool> alive_;
  bool settled_{false};
};

}  // namespace ember::win32::entry
