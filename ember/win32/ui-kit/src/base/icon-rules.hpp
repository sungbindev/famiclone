/*
 * ------------------------------------------------------------------------
 * 名称：icon-rules.hpp
 * 说明：按钮文字到图标的自动匹配：规则表由生成器从组件库的 icon-rules.ts 生成，这里负责按文字查找。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-19
 * 备注：规则按书写顺序匹配，先匹配到的生效；匹配前先去掉文字首尾的空白；规则只含分组、选择与 ^ $ 锚点，按 UTF-8 字节匹配。
 * ------------------------------------------------------------------------
 */

#pragma once

#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "rujian/ui/icon-id.hpp"

namespace rujian::ui {

/** 一条规则（生成器输出）。 */
struct LabelIconRule {
  /** 匹配式（ECMAScript 语法）。 */
  std::string pattern;
  /** 图标。 */
  IconId icon;
  /** 图标是否放在文字后面。 */
  bool atEnd;
};

/** 匹配结果。 */
struct LabelIconMatch {
  IconId icon;
  bool atEnd;
};

/** 全部规则，由生成的文件提供。 */
const std::vector<LabelIconRule>& labelIconRules();

/**
 * 按文字含义匹配图标。
 * @param label 按钮文字（UTF-8）。
 * @return 匹配结果；没有匹配的规则时为空。
 */
[[nodiscard]] std::optional<LabelIconMatch> iconForLabel(std::string_view label);

}  // namespace rujian::ui
