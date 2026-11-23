/*
 * ------------------------------------------------------------------------
 * 名称：runtime-rules.hpp
 * 说明：运行模块用到的纯规则：从 runtime.status 的结果得出机器的状态（未开机、已断电、已暂停、运行中）、各控制是否可用，运行面板与系统栏显示的文字，以及“选择机型”的选项。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-19
 * 备注：规则见 docs/05-界面.md 第 4.1 节：没有开机时暂停、复位置灰；断电时暂停与软复位置灰；硬复位只在通电时可用；
 *       “开机”按已选机型判断（见 powerToggleAction）；“关闭卡带”只在装着卡带时可用；状态还没读到时全部置灰。
 *       卡带、mapper 插件没有信息时文字函数返回空值，界面不显示这一项，不用“—”或“无”占位；主机型号与 ROM 版本在整机机型时有值（未开机时是已选机型），经典 FC 与状态还没读到时没有。
 * ------------------------------------------------------------------------
 */

#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

#include <nlohmann/json.hpp>

#include "contracts/runtime/runtime-model.hpp"

namespace ember::win32::logic {

/** 经典 FC 的机型标识：选它时要求已经装入 FC 卡带。 */
inline constexpr const char* kClassicMachine = "fc";

/** 当前装入的卡带。 */
struct RuntimeCartridge {
  std::string path;
  std::string pluginName;
};

/** runtime.status 的结果。 */
struct RuntimeStatus {
  bool powerOn{false};
  bool running{false};
  /** 主机型号；没有装入卡带也没有开机时没有。 */
  std::optional<std::string> machine;
  std::optional<RuntimeCartridge> cartridge;
  /** 整机机型开机所用 ROM 的版本（版本名或文件名）；经典 FC 与没有主机时没有。 */
  std::optional<std::string> romLabel;
  /** 机器现在是否带鼠标。 */
  bool mouse{false};
};

/** 机型清单里的一项。 */
struct MachineEntry {
  std::string id;
  std::string name;
  bool needsRom{false};
  bool supported{false};
  /** 是否已添加 ROM。 */
  bool hasRom{false};
  /** 已添加 ROM 的版本（版本名或文件名）；没有 ROM 时为空。 */
  std::string romLabel;
};

/** 最近打开的清单里的一项：编号与显示名，没有路径。 */
struct RecentCartridge {
  std::int64_t id{0};
  std::string name;
};

/** 从 runtime.status 的结果读出状态；格式不对时没有值。 */
[[nodiscard]] std::optional<RuntimeStatus> parseRuntimeStatus(const nlohmann::json& result);
/** 从 machines.list 的结果读出机型清单；格式不对时为空。 */
[[nodiscard]] std::vector<MachineEntry> parseMachines(const nlohmann::json& result);
/** 从 cartridges.recent 的结果读出清单；格式不对时为空。 */
[[nodiscard]] std::vector<RecentCartridge> parseRecent(const nlohmann::json& result);

/** 路径里的文件名部分。 */
[[nodiscard]] std::string fileNameOf(const std::string& path);

/** 机器的状态；status 没有值（还没读到）时为 Unknown。 */
[[nodiscard]] contracts::MachineState machineState(const std::optional<RuntimeStatus>& status);
/** 状态文字（未开机、已断电、已暂停、运行中）；还没读到状态时没有信息。 */
[[nodiscard]] std::optional<std::string> stateText(const std::optional<RuntimeStatus>& status);
/** 主机型号的显示文字：开机后是正在运行的主机，没有开机时是已选机型；经典 FC（有卡带名就够了）与状态还没读到时没有信息。 */
[[nodiscard]] std::optional<std::string> hostText(const std::optional<RuntimeStatus>& status, const std::string& selectedMachine);
/** ROM 版本的显示文字：开机后是正在运行的 ROM，没有开机时是已选机型已添加的 ROM；经典 FC、状态还没读到、没有 ROM 时没有信息。 */
[[nodiscard]] std::optional<std::string> romVersionText(const std::optional<RuntimeStatus>& status, const std::string& selectedMachine, const std::vector<MachineEntry>& machines);
/** 卡带的显示文字：卡带文件名；没有卡带（含整机机型用 ROM 开机）时没有信息。 */
[[nodiscard]] std::optional<std::string> cartridgeText(const std::optional<RuntimeStatus>& status);
/** mapper 插件的显示文字：插件名；没有卡带时没有信息。 */
[[nodiscard]] std::optional<std::string> pluginText(const std::optional<RuntimeStatus>& status);
/** 开机或断电按钮与菜单项的名称：通电时是“断电”，否则是“开机”。 */
[[nodiscard]] std::string powerToggleText(const std::optional<RuntimeStatus>& status);
/** 暂停或继续按钮与菜单项的名称：连续运行时是“暂停”，否则是“继续”。 */
[[nodiscard]] std::string pauseToggleText(const std::optional<RuntimeStatus>& status);

/** 开机或断电按钮这次要做的事：断电、上电（已装入的主机）、用已选机型的 ROM 开机，或不能做。 */
enum class PowerToggleAction { PowerOff, PowerOn, Boot, None };

/**
 * 开机或断电按钮这次要做什么。通电时是断电；未通电时按已选机型：整机机型（学习机）已支持并已添加 ROM 就开机（不需要卡带，当前装的是别的主机也用它重新开机），
 * 经典 FC 要求已经装入 FC 卡带，有就上电。状态还没读到时不能做。
 */
[[nodiscard]] PowerToggleAction powerToggleAction(const std::optional<RuntimeStatus>& status, const std::string& selectedMachine, const std::vector<MachineEntry>& machines);

/** 各控制是否可用；状态还没读到时全部置灰。 */
[[nodiscard]] contracts::RuntimeControls controlAvailability(const std::optional<RuntimeStatus>& status, const std::string& selectedMachine, const std::vector<MachineEntry>& machines);

/** 是否捕获鼠标：首选项“点击画面捕获鼠标”打开，并且机器现在带鼠标。 */
[[nodiscard]] bool capturesMouse(bool enabled, const std::optional<RuntimeStatus>& status);

/** 键盘是否独占：接入了 Family BASIC 键盘，或已开机的是学习机整机机型（不是经典 FC）。 */
[[nodiscard]] bool exclusiveKeyboard(bool familyKeyboard, const std::optional<RuntimeStatus>& status);

/** “选择机型”里的一个选项。 */
struct MachineChoice {
  std::string id;
  std::string label;
  /** 不可选的原因；可选时为空。 */
  std::string hint;
  /** 上面是否加分隔线：不同品牌之间分开。 */
  bool separatorBefore{false};
};

/** “选择机型”的选项：四个整机机型，顺序固定；要程序里有实现并已添加 ROM 才可选，不可选的注明原因；经典 FC 不在里面。 */
[[nodiscard]] std::vector<MachineChoice> machineChoices(const std::vector<MachineEntry>& machines);

/** “机器”菜单第一项的名称：已选了整机机型时是它的名字，没选过（或已选的是经典 FC）时是“选择机型”。 */
[[nodiscard]] std::string machineMenuTitle(const std::vector<MachineEntry>& machines, const std::string& selectedMachine);

}  // namespace ember::win32::logic
