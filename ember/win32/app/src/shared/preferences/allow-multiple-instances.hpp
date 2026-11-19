/*
 * ------------------------------------------------------------------------
 * 名称：allow-multiple-instances.hpp
 * 说明：从首选项文件的内容里取“允许同时运行多个实例”：启动时核心还没起来，读不了核心里的设置，所以直接读首选项文件里的这一项。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-19
 * 备注：规则与 Electron 外壳的 src/platform/launch/allow-multiple-instances.ts 相同：设置项 app.allowMultipleInstances，文件格式版本 1；
 *       文件不存在、格式不对或取值不是布尔值时都按默认（不允许）处理。
 * ------------------------------------------------------------------------
 */

#pragma once

#include <nlohmann/json.hpp>

namespace ember::win32::shared {

/**
 * 读取是否允许同时运行多个实例。
 * @param preferences 首选项文件的内容（config/preferences.json）。
 * @returns 设置为开启时为 true；其他情况为 false。
 */
[[nodiscard]] bool readAllowMultipleInstances(const nlohmann::json& preferences);

}  // namespace ember::win32::shared
