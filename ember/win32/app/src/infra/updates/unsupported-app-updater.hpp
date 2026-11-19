/*
 * ------------------------------------------------------------------------
 * 名称：unsupported-app-updater.hpp
 * 说明：“不支持升级”的应用升级实现：状态固定是 unsupported，检查更新什么都不做，没有下载好的新版本可安装。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-19
 * 备注：原生外壳的升级引擎在阶段 11 加入，加入后组装根换成真正的实现；Electron 外壳里开发版、免安装目录的状态也是 unsupported，界面按它隐藏“更新”按钮。
 * ------------------------------------------------------------------------
 */

#pragma once

#include "contracts/system/app-updater.hpp"

namespace ember::win32::infra {

/** 不支持升级。 */
class UnsupportedAppUpdater : public contracts::AppUpdater {
 public:
  [[nodiscard]] contracts::UpdateStatus status() const override { return contracts::UpdateStatus{"unsupported", std::nullopt, std::nullopt}; }
  contracts::UpdateStatus check() override { return status(); }
  bool install() override { return false; }
};

}  // namespace ember::win32::infra
