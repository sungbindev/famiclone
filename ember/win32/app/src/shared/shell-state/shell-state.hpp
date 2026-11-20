/*
 * ------------------------------------------------------------------------
 * 名称：shell-state.hpp
 * 说明：界面状态的类型、默认值与校验：窗口位置、上次选择的机型、手动关掉“自动收缩”的底部面板、上次显示更新记录时的应用版本。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-19
 * 备注：规则与文件格式（config/shell-state.json）见 docs/05-界面.md 第 9 节；
 *       读到的内容逐项校验，不合格的项用默认值，不影响其他项；窗口大小与是否最大化不保存，底部面板是否展开与当前页签也不保存。
 * ------------------------------------------------------------------------
 */

#pragma once

#include <optional>
#include <string>
#include <vector>

#include <nlohmann/json.hpp>

namespace ember::win32::shared {

/** 窗口的位置（左上角坐标，设备像素），大小与是否最大化不保存。 */
struct WindowPlacement {
  int x{0};
  int y{0};

  friend bool operator==(const WindowPlacement&, const WindowPlacement&) = default;
};

/** 外壳需要保存的选择。 */
struct ShellLayout {
  /** “机器→选择机型”里上次选的机型标识；为空表示没选过。 */
  std::optional<std::string> selectedMachine;
  /** 用户手动关掉“自动收缩”的底部面板标识，没有重复。 */
  std::vector<std::string> autoCollapseOffPanels;

  friend bool operator==(const ShellLayout&, const ShellLayout&) = default;
};

/** 需要保存的全部界面状态。 */
struct ShellState {
  /** 窗口的位置；为空表示没有保存过，用默认位置。 */
  std::optional<WindowPlacement> window;
  ShellLayout layout;
  /** 上次显示更新记录时的应用版本；为空表示没有记录。 */
  std::optional<std::string> releaseNotesVersion;
};

/**
 * 解析保存的界面状态；内容缺失或损坏时得到默认值，单项不合格时只替换那一项。
 * @param raw 读到的内容。
 */
[[nodiscard]] ShellState parseShellState(const nlohmann::json& raw);

/** 转成要保存的内容。 */
[[nodiscard]] nlohmann::json toJson(const ShellState& state);

}  // namespace ember::win32::shared
