/*
 * ------------------------------------------------------------------------
 * 名称：window.hpp
 * 说明：窗口宿主：创建一个原生窗口，把鼠标、键盘与缩放事件转给 Root，用 Direct2D 把控件树画到窗口上。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-19
 * 备注：窗口按当前显示器的 DPI 缩放；程序需要在清单里声明 DPI 感知，否则系统会自己拉伸窗口。
 *       应用的主窗口（自绘标题栏等）在这个基础上扩展，不在这里实现。
 * ------------------------------------------------------------------------
 */

#pragma once

#include <memory>
#include <string>

#include "rujian/ui/geometry.hpp"
#include "rujian/ui/root.hpp"

namespace rujian::ui {

/** 窗口宿主。 */
class HostWindow {
 public:
  /** 创建窗口的选项。 */
  struct Options {
    /** 窗口标题。 */
    std::wstring title;
    /** 客户区的逻辑大小。 */
    Size clientSize{800, 600};
    /** 初始主题。 */
    ThemeName theme{ThemeName::Dark};
  };

  /**
   * 创建窗口（尚未显示）。
   * @throws std::runtime_error 创建窗口失败时。
   */
  explicit HostWindow(const Options& options);
  ~HostWindow();
  HostWindow(const HostWindow&) = delete;
  HostWindow& operator=(const HostWindow&) = delete;

  /** 根容器，设置内容、切换主题都经它。 */
  [[nodiscard]] Root& root();

  /** 显示窗口。 */
  void show();

  /** 运行消息循环，直到窗口关闭；返回退出码。 */
  static int runMessageLoop();

 private:
  struct Impl;
  std::unique_ptr<Impl> impl_;
};

}  // namespace rujian::ui
