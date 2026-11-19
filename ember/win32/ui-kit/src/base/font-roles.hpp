/*
 * ------------------------------------------------------------------------
 * 名称：font-roles.hpp
 * 说明：字体分工：按字符决定用哪一类字体，规则与网页版的字体声明一致。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-19
 * 备注：网页版的字体声明是“宋体类（只管弯引号、破折号、省略号）、Segoe UI、微软雅黑”；
 *       Windows 7 的文字绘制没有自动换字体的功能，所以这里自己按字符分段：
 *       标点专用 → 宋体类，西文与符号 → Segoe UI，汉字与全角字符 → 微软雅黑。
 * ------------------------------------------------------------------------
 */

#pragma once

namespace rujian::ui {

/** 字符使用的字体类别。 */
enum class FontRole {
  /** 西文、数字、半角符号：Segoe UI。 */
  Latin,
  /** 汉字、假名、全角字符：微软雅黑。 */
  Cjk,
  /** 弯引号、破折号、省略号：宋体类字体。 */
  Punctuation
};

/**
 * 判断字符使用的字体类别。
 * @param codePoint Unicode 码点。
 */
[[nodiscard]] constexpr FontRole fontRoleOf(char32_t codePoint) {
  if (codePoint == 0x2014 || (codePoint >= 0x2018 && codePoint <= 0x201D) || codePoint == 0x2026) return FontRole::Punctuation;
  if (codePoint < 0x2E80) return FontRole::Latin;
  return FontRole::Cjk;
}

}  // namespace rujian::ui
