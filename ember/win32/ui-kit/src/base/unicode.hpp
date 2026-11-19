/*
 * ------------------------------------------------------------------------
 * 名称：unicode.hpp
 * 说明：UTF-8 与 UTF-16 互转，以及按码点判断字符类别的辅助函数。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-19
 * 备注：无效的 UTF-8 序列与落单的代理项一律换成 U+FFFD；不依赖 Windows 头文件，可在纯逻辑代码里使用。
 * ------------------------------------------------------------------------
 */

#pragma once

#include <string>
#include <string_view>

namespace rujian::ui {

/** UTF-8 转 UTF-16。 */
[[nodiscard]] std::u16string toUtf16(std::string_view utf8);

/** UTF-16 转 UTF-8。 */
[[nodiscard]] std::string toUtf8(std::u16string_view utf16);

/** 码点转 UTF-16（一个或两个单元）。 */
[[nodiscard]] std::u16string toUtf16(char32_t codePoint);

/** 在 text 的 index 处取出码点；index 落在代理对中间时按单个代理项返回。 */
[[nodiscard]] char32_t codePointAt(std::u16string_view text, std::size_t index);

/** index 处的码点占几个 UTF-16 单元（1 或 2）。 */
[[nodiscard]] std::size_t codePointLength(std::u16string_view text, std::size_t index);

}  // namespace rujian::ui
