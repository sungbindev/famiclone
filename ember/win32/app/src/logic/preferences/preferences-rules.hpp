/*
 * ------------------------------------------------------------------------
 * 名称：preferences-rules.hpp
 * 说明：首选项模块用到的纯规则：从核心回复读出分组与设置项，得出对话框要显示的分组与设置行（隐藏项与没有显示项的分组不显示），整数输入的解析与范围校验，错误说明的读取。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-20
 * 备注：规则见 docs/05-界面.md 第 4.5 节；不依赖界面与请求，便于单独测试。
 * ------------------------------------------------------------------------
 */

#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

#include <nlohmann/json.hpp>

#include "contracts/core-gateway/core-gateway.hpp"
#include "contracts/preferences/preferences-model.hpp"

namespace ember::win32::logic {

/** 设置项的种类（preferences.list 里的 kind）。 */
enum class SettingKind { Choice, Switch, Integer, Text };

/** 核心登记的一个设置项（preferences.list 的一项）。 */
struct Setting {
  std::string id;
  std::string group;
  std::string label;
  std::string description;
  SettingKind kind{SettingKind::Text};
  std::vector<contracts::PreferenceChoice> options;
  std::optional<std::int64_t> minimum;
  std::optional<std::int64_t> maximum;
  /** 选择文件夹的按钮：取值 directory 时有。 */
  bool directoryPicker{false};
  /** 取值由核心生成的“更换”按钮：取值 regenerate 时有。 */
  bool regenerates{false};
  /** 隐藏的设置项只保存取值，不在页面里显示（如音量）。 */
  bool hidden{false};
  nlohmann::json defaultValue;
  nlohmann::json value;
};

/** 核心登记的一个分组。 */
struct SettingGroup {
  std::string id;
  std::string label;
};

/** preferences.list 的结果。 */
struct SettingListing {
  std::vector<SettingGroup> groups;
  std::vector<Setting> settings;
};

/** 整数输入的解析结果。 */
struct IntegerParse {
  bool ok{false};
  std::int64_t value{0};
  /** 不通过时的说明。 */
  std::string message;
};

/** 从 preferences.list 的结果读出分组与设置项；格式不对时没有值，格式不对的单项跳过。 */
[[nodiscard]] std::optional<SettingListing> parseSettingListing(const nlohmann::json& result);

/** 取值变化之后的设置项列表；不认识的标识原样返回。 */
void applyValue(std::vector<Setting>& settings, const std::string& id, const nlohmann::json& value);
/** 当前取值是否与出厂默认值不同。 */
[[nodiscard]] bool isChanged(const Setting& setting);
/** 取值在输入框里或下拉框里的文字。 */
[[nodiscard]] std::string valueText(const nlohmann::json& value);

/** 解析整数输入并按设置项登记的范围校验。 */
[[nodiscard]] IntegerParse parseInteger(const std::string& text, std::optional<std::int64_t> minimum, std::optional<std::int64_t> maximum);

/**
 * 对话框此刻要显示的分组与设置行：只有显示的设置项，没有显示项的分组不出现。
 */
[[nodiscard]] std::vector<contracts::PreferenceGroupView> buildGroupViews(const SettingListing& listing);

/** 从请求失败的原因里取出给用户看的说明；没有说明时为“操作失败。”。 */
[[nodiscard]] std::string errorText(const contracts::CoreError& error);

}  // namespace ember::win32::logic
