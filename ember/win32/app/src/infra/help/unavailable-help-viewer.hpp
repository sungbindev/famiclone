/*
 * ------------------------------------------------------------------------
 * 名称：unavailable-help-viewer.hpp
 * 说明：“帮助文档不可用”的实现：打开帮助文档时返回固定的说明。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-19
 * 备注：原生外壳的帮助文档（本机只读服务加系统默认浏览器）在阶段 10 加入，加入后组装根换成真正的实现。
 * ------------------------------------------------------------------------
 */

#pragma once

#include "contracts/system/help-viewer.hpp"

namespace ember::win32::infra {

/** 帮助文档不可用。 */
class UnavailableHelpViewer : public contracts::HelpViewer {
 public:
  [[nodiscard]] std::string open(const std::optional<std::string>& /*page*/) override { return "原生外壳的帮助文档还没有提供"; }
};

}  // namespace ember::win32::infra
