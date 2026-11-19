/*
 * ------------------------------------------------------------------------
 * 名称：preferences-rules.hpp
 * 说明：首选项模块用到的纯规则：从核心回复读出分组、设置项与机型，得出对话框要显示的分组与设置行（隐藏项与没有显示项的分组不显示，插入“机型与 BIOS”分组），整数输入的解析与范围校验，机型的 BIOS 状态文字，错误说明的读取。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-20
 * 备注：规则同 Electron 外壳的 preferences-model.ts（见 docs/05-界面.md 第 4.5 节）；不依赖界面与请求，便于单独测试。
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

/** 核心登记的一个机型（machines.list 的一项）。 */
struct Machine {
  std::string id;
  std::string name;
  bool needsBios{false};
  bool supported{false};
  bool hasBios{false};
  std::string biosFileName;
  std::int64_t biosSize{0};
  bool operator==(const Machine&) const = default;
};

/** “机型与 BIOS”分组的标识。 */
inline constexpr const char* kMachinesGroupId = "machines";

/** 从 preferences.list 的结果读出分组与设置项；格式不对时没有值，格式不对的单项跳过。 */
[[nodiscard]] std::optional<SettingListing> parseSettingListing(const nlohmann::json& result);
/** 从 machines.list 的结果读出机型清单，保持核心给出的顺序；格式不对时没有值。 */
[[nodiscard]] std::optional<std::vector<Machine>> parseBiosMachines(const nlohmann::json& result);
/** 从 machines.openBios 等结果里的 machine 读出一个机型；格式不对时没有值。 */
[[nodiscard]] std::optional<Machine> parseBiosMachine(const nlohmann::json& machine);

/** 取值变化之后的设置项列表；不认识的标识原样返回。 */
void applyValue(std::vector<Setting>& settings, const std::string& id, const nlohmann::json& value);
/** 当前取值是否与出厂默认值不同。 */
[[nodiscard]] bool isChanged(const Setting& setting);
/** 取值在输入框里或下拉框里的文字。 */
[[nodiscard]] std::string valueText(const nlohmann::json& value);

/** 解析整数输入并按设置项登记的范围校验。 */
[[nodiscard]] IntegerParse parseInteger(const std::string& text, std::optional<std::int64_t> minimum, std::optional<std::int64_t> maximum);

/**
 * 对话框此刻要显示的分组与设置行：只有显示的设置项，没有显示项的分组不出现；在“输入”分组之后（没有输入分组时在“声音”之后，都没有时放在最后）插入“机型与 BIOS”分组。
 */
[[nodiscard]] std::vector<contracts::PreferenceGroupView> buildGroupViews(const SettingListing& listing);

/** 一个机型的 BIOS 状态文字：暂未支持、未添加，或文件名与大小（KB，四舍五入）。 */
[[nodiscard]] std::string biosStatusText(const Machine& machine);
/** “机型与 BIOS”分组的行：只有需要 BIOS 的机型。 */
[[nodiscard]] std::vector<contracts::BiosRow> buildBiosRows(const std::vector<Machine>& machines);

/** 从请求失败的原因里取出给用户看的说明；没有说明时为“操作失败。”。 */
[[nodiscard]] std::string errorText(const contracts::CoreError& error);

}  // namespace ember::win32::logic
