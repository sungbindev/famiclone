/*
 * ------------------------------------------------------------------------
 * 名称：preference-handlers.hpp
 * 说明：preferences 模块里外壳自己处理的请求：pickDirectory（弹出选择文件夹窗口，返回选定的路径，页面再用 preferences.set 保存）。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-19
 * 备注：用户取消选择时结果的 canceled 为 true，不是错误。
 * ------------------------------------------------------------------------
 */

#pragma once

#include "contracts/shell/file-dialog.hpp"
#include "entry/main-requests/main-request-handler.hpp"

namespace ember::win32::entry {

/** preferences 请求处理所需的依赖。 */
struct PreferenceDependencies {
  contracts::FileDialog& dialog;
};

/** 创建 preferences 模块的处理函数。 */
[[nodiscard]] MainRequestHandlers createPreferenceHandlers(PreferenceDependencies dependencies);

}  // namespace ember::win32::entry
