/*
 * ------------------------------------------------------------------------
 * 名称：third-party-notices.hpp
 * 说明：第三方依赖清单：“关于 Ember”对话框里显示的清单，由构建脚本从各项目的声明生成（generated/third-party-notices.generated.cpp）。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-21
 * 备注：生成物不提交，配置工程前由 win32-cmake.mjs 生成。
 * ------------------------------------------------------------------------
 */

#pragma once

#include <vector>

#include "contracts/system/about-model.hpp"

namespace ember::win32::bootstrap {

/** 第三方依赖清单，按名称排序。 */
[[nodiscard]] const std::vector<contracts::ThirdPartyNotice>& thirdPartyNotices();

}  // namespace ember::win32::bootstrap
