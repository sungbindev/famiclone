/*
 * ------------------------------------------------------------------------
 * 名称：clipboard.hpp
 * 说明：系统剪贴板的文字读写（UTF-8）。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-19
 * 备注：用一个隐藏的消息窗口作剪贴板的所有者，不依赖具体的界面窗口，离屏绘制与测试里也能用；只在界面线程调用。
 * ------------------------------------------------------------------------
 */

#pragma once

#include <optional>
#include <string>
#include <string_view>

namespace rujian::ui {

/** 读剪贴板里的文字；剪贴板打不开或没有文字时为空。 */
[[nodiscard]] std::optional<std::string> readClipboardText();

/** 把文字放进剪贴板，成功返回 true。 */
bool writeClipboardText(std::string_view utf8);

}  // namespace rujian::ui
