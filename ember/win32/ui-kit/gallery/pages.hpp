/*
 * ------------------------------------------------------------------------
 * 名称：pages.hpp
 * 说明：预览程序的示例页：每页是一组控件，与 tools/reference/page/pages 里的网页版参考页一一对应。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-19
 * 备注：samples 是页里参与对照的控件，顺序必须与参考页里带 data-sample 属性的元素顺序一致；
 *       inset 是对照像素时每个样本内部不参与对照的边距（里面是文字与图标）；targets 是做悬停、按下、焦点场景的样本序号。
 *       改一边要同步另一边。
 * ------------------------------------------------------------------------
 */

#pragma once

#include <functional>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "rujian/ui/geometry.hpp"
#include "rujian/ui/notification.hpp"
#include "rujian/ui/widget.hpp"

namespace gallery {

/** 一个示例页的说明（不需要创建控件就能知道）。 */
struct PageInfo {
  /** 页名，用于命令行与参考页的 page 参数。 */
  std::string name;
  /** 页面的逻辑大小；宽高都取 4 的倍数，这样 125%、150% 缩放下像素大小都是整数，网页版与原生版的截图大小才一致。 */
  rujian::ui::Size size;
  /** 对照像素时每个样本内部不参与对照的边距。 */
  rujian::ui::Insets inset;
  /** 个别样本（序号）用不同的边距，比如链接按钮整个区域都是文字。 */
  std::vector<std::pair<int, rujian::ui::Insets>> insetOverrides;
  /** 样本之后的额外矩形（如下拉弹层里的选项）对照像素时内部不参与对照的边距。 */
  rujian::ui::Insets extraInset;
  /** 做悬停、按下、焦点场景的样本序号。 */
  std::vector<int> targets;
  /** 这页要对照的交互场景（hover、pressed、focus），对 targets 里的每个样本各做一遍；另有不带交互的 normal 场景。 */
  std::vector<std::string> scenes;
  /** setup 回调里才登记的样本个数（对照工具据此区分样本与之后的额外矩形）。 */
  int lateSamples{0};
};

/** 建好的示例页。 */
struct Page {
  /** 整页的内容，交给 Root。 */
  std::unique_ptr<rujian::ui::Widget> content;
  /** 参与对照的控件，按页里的先后顺序。 */
  std::vector<rujian::ui::Widget*> samples;
  /** 布局之后执行：用窗口的通知中心发通知、打开面板，并把通知等之后才有的控件登记为样本；没有时为空。 */
  std::function<void(rujian::ui::NotificationCenter&, std::vector<rujian::ui::Widget*>&)> setup;
};

/** 全部示例页的说明。 */
[[nodiscard]] const std::vector<PageInfo>& pageInfos();

/**
 * 建一个示例页。
 * @param name 页名，必须在 pageInfos 里。
 */
[[nodiscard]] Page buildPage(const std::string& name);

}  // namespace gallery
