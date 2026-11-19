/*
 * ------------------------------------------------------------------------
 * 名称：help-viewer.hpp
 * 说明：帮助文档查看的抽象：打开随应用带的帮助文档（首页或固定的几页之一）。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-19
 * 备注：对应 Electron 外壳的 src/platform/help 的帮助文档窗口；文档的位置由外壳决定，界面只能从固定的几页里选；
 *       原生外壳的帮助文档（本机只读服务加系统默认浏览器）在阶段 10 加入，在此之前由“帮助文档不可用”的实现提供。
 * ------------------------------------------------------------------------
 */

#pragma once

#include <optional>
#include <string>

namespace ember::win32::contracts {

/** 帮助文档的查看。 */
class HelpViewer {
 public:
  virtual ~HelpViewer() = default;

  /**
   * 打开帮助文档。
   * @param page 要打开的页面（目前只有 changelog）；为空时打开首页。
   * @returns 成功时为空文字，失败时是错误说明。
   */
  [[nodiscard]] virtual std::string open(const std::optional<std::string>& page) = 0;
};

}  // namespace ember::win32::contracts
