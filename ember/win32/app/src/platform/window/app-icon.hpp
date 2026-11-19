/*
 * ------------------------------------------------------------------------
 * 名称：app-icon.hpp
 * 说明：应用图标的像素：从可执行文件自带的图标资源（与 Electron 外壳是同一张蓝色卡带图）里取指定大小的一张，转成预乘透明度的像素，给标题栏的标志画。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-20
 * 备注：图标资源里有 16、24、32、48、64、128、256 像素七种尺寸，系统取最接近的一张，尺寸不一致时缩放；
 *       资源编号 1 是 ember-win.rc 里的应用图标；没有这个资源（如测试程序）时返回空。
 * ------------------------------------------------------------------------
 */

#pragma once

#include <cstdint>
#include <vector>

namespace ember::win32::platform {

/**
 * 取应用图标的像素。
 * @param pixelSize 边长（设备像素）。
 * @returns pixelSize × pixelSize 个像素，每个按 0xAARRGGBB 排列、颜色已预乘透明度；取不到图标或 pixelSize 不是正数时为空。
 */
[[nodiscard]] std::vector<std::uint32_t> renderAppIcon(int pixelSize);

}  // namespace ember::win32::platform
