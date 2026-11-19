/*
 * ------------------------------------------------------------------------
 * 名称：font-data.hpp
 * 说明：嵌入的字体数据：标题栏应用名用的 Quicksand（TrueType 字节），由生成器写入 src/generated/font-data.generated.cpp。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-19
 * 备注：文字引擎用它建自定义字体集，不依赖系统是否装了这些字体。
 * ------------------------------------------------------------------------
 */

#pragma once

#include <cstddef>
#include <vector>

namespace rujian::ui {

/** 一个嵌入的字体文件（TrueType 字节）。 */
struct EmbeddedFont {
  const unsigned char* data;
  std::size_t size;
};

/** 全部嵌入的字体：Quicksand 500 与 700 两个字重。 */
const std::vector<EmbeddedFont>& embeddedFonts();

}  // namespace rujian::ui
