/*
 * ------------------------------------------------------------------------
 * 名称：window-shadow-image.hpp
 * 说明：窗口外阴影的图像：按窗口大小算出一圈带透明度的黑色柔和阴影（预乘透明度的 BGRA），给自绘阴影的分层窗口用。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-19
 * 备注：阴影是窗口矩形平移后做高斯模糊的结果：矩形的高斯模糊可以拆成横竖两个方向的一维积分相乘，所以每个像素只要两次乘法；
 *       图像比窗口四周各大出 margin 个像素，窗口本身所在的位置也有值（被窗口盖住，看不到）；有激活时阴影深一些，没有激活时浅一些；尺寸都按设备像素给出。
 * ------------------------------------------------------------------------
 */

#pragma once

#include <cstdint>
#include <vector>

namespace ember::win32::shared {

/** 阴影图像。 */
struct WindowShadowImage {
  /** 图像宽高（设备像素）：窗口宽高加上四周各一个 margin。 */
  int width{0};
  int height{0};
  /** 窗口四周多出的像素数。 */
  int margin{0};
  /** 像素，行优先，每个 32 位按 0xAARRGGBB 排列，颜色预乘透明度（阴影是黑色，所以只有透明度有值）。 */
  std::vector<std::uint32_t> pixels;
};

/** 阴影在四周延伸的距离（逻辑像素）。 */
[[nodiscard]] int shadowMargin(double scale);

/**
 * 画窗口的阴影。
 * @param windowWidth 窗口宽（设备像素）。
 * @param windowHeight 窗口高（设备像素）。
 * @param scale 缩放比例（设备像素 / 逻辑像素）。
 * @param active 窗口是否处于激活状态。
 * @returns 阴影图像；窗口宽或高不是正数时图像为空。
 */
[[nodiscard]] WindowShadowImage renderWindowShadow(int windowWidth, int windowHeight, double scale, bool active);

}  // namespace ember::win32::shared
