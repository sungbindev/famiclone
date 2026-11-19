/*
 * ------------------------------------------------------------------------
 * 名称：icon-rules.cpp
 * 说明：按文字匹配图标的实现：第一次使用时把规则表编译成正则表达式。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-19
 * 备注：规则表已由生成器检查过写法，编译正则不会失败；单线程使用。
 * ------------------------------------------------------------------------
 */

#include "base/icon-rules.hpp"

#include <regex>

namespace rujian::ui {

std::optional<LabelIconMatch> iconForLabel(std::string_view label) {
  static const std::vector<std::regex> compiled = [] {
    std::vector<std::regex> result;
    for (const LabelIconRule& rule : labelIconRules()) result.emplace_back(rule.pattern, std::regex::ECMAScript);
    return result;
  }();

  while (!label.empty() && (label.front() == ' ' || label.front() == '\t' || label.front() == '\n' || label.front() == '\r')) label.remove_prefix(1);
  while (!label.empty() && (label.back() == ' ' || label.back() == '\t' || label.back() == '\n' || label.back() == '\r')) label.remove_suffix(1);
  const std::string text(label);
  const auto& rules = labelIconRules();
  for (std::size_t i = 0; i < rules.size(); ++i) {
    if (std::regex_search(text, compiled[i])) return LabelIconMatch{rules[i].icon, rules[i].atEnd};
  }
  return std::nullopt;
}

}  // namespace rujian::ui
