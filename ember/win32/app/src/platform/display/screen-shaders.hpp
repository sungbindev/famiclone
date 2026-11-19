/*
 * ------------------------------------------------------------------------
 * 名称：screen-shaders.hpp
 * 说明：运行画面着色器的字节码（由 screen.hlsl 编译，定义在 screen-shaders.generated.cpp）。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-19
 * 备注：重新生成的办法见 ember/tools/build/gen-screen-shaders.mjs。
 * ------------------------------------------------------------------------
 */

#pragma once

#include <cstddef>

namespace ember::win32::platform {

extern const unsigned char kScreenVertexShader[];
extern const std::size_t kScreenVertexShaderSize;
extern const unsigned char kScreenPixelShader[];
extern const std::size_t kScreenPixelShaderSize;

}  // namespace ember::win32::platform
