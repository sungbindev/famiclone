/*
 * ------------------------------------------------------------------------
 * 名称：app-logo.hpp
 * 说明：标题栏左边的应用标志：画一张由外部提供的方形位图（应用图标），大小跟着窗口的缩放比例取最接近设备像素的一张。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-20
 * 备注：位图的来源（可执行文件的图标资源）由装配代码经函数传入，界面层不直接访问系统接口；提供者返回空时什么也不画；
 *       同一个边长只取一次，缩放比例变化后再取。
 * ------------------------------------------------------------------------
 */

#pragma once

#include <cstdint>
#include <functional>
#include <vector>

#include "rujian/ui/widget.hpp"

namespace ember::win32::ui {

/** 应用标志控件。 */
class AppLogo final : public rujian::ui::Widget {
 public:
  /** 取位图的函数：参数是边长（设备像素），返回边长 × 边长个预乘透明度的 0xAARRGGBB 像素，取不到时返回空。 */
  using BitmapProvider = std::function<std::vector<std::uint32_t>(int pixelSize)>;

  explicit AppLogo(BitmapProvider provider);

  void paint(rujian::ui::Canvas& canvas) override;

 private:
  BitmapProvider provider_;
  int cachedSize_{0};
  std::vector<std::uint32_t> cached_;
};

}  // namespace ember::win32::ui
