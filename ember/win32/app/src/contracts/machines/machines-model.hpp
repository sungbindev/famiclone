/*
 * ------------------------------------------------------------------------
 * 名称：machines-model.hpp
 * 说明：机型模块给界面层的模型：“机型与 BIOS”对话框里的机型清单（BIOS 状态）与操作（添加或替换、移除、打开 BIOS 目录）。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-21
 * 备注：机型清单来自核心（machines.list）；文字与可用状态由逻辑层算好，界面只显示；
 *       操作的结果与错误用外壳通知给出，模型不显示结果；规则见 docs/05-界面.md 第 4.7 节。
 * ------------------------------------------------------------------------
 */

#pragma once

#include <functional>
#include <string>
#include <vector>

namespace ember::win32::contracts {

/** 机型清单的读取状态。 */
enum class MachinesLoad {
  /** 正在读取（核心还没有就绪或核心正在重启）。 */
  Loading,
  Ready,
  /** 读取失败，说明在 message 里。 */
  Failed
};

/** “机型与 BIOS”对话框里的一个机型。 */
struct BiosRow {
  /** 机型标识。 */
  std::string machine;
  /** 机型名称。 */
  std::string name;
  /** BIOS 状态：暂未支持、未添加，或文件名与大小（KB）。 */
  std::string status;
  /** 是否已添加 BIOS：已添加时按钮是“替换”和“移除”，否则是“添加 BIOS 文件”。 */
  bool hasBios{false};
  /** 暂未支持的机型整行置灰，按钮不可用。 */
  bool supported{true};
  bool operator==(const BiosRow&) const = default;
};

/** “机型与 BIOS”对话框此刻要显示的内容。 */
struct BiosView {
  MachinesLoad load{MachinesLoad::Loading};
  std::string message;
  /** 需要 BIOS 的机型（经典 FC 不需要，不在其中）。 */
  std::vector<BiosRow> rows;
  bool operator==(const BiosView&) const = default;
};

/** “机型与 BIOS”对话框的模型。 */
class BiosModel {
 public:
  virtual ~BiosModel() = default;

  [[nodiscard]] virtual BiosView view() const = 0;

  /** 添加或替换机型的 BIOS：弹出系统文件选择窗口，用户取消时什么都不发生；结果与错误用通知给出。 */
  virtual void choose(const std::string& machine) = 0;
  /** 移除机型的 BIOS，不弹确认框。 */
  virtual void remove(const std::string& machine) = 0;
  /** 用系统的文件管理器打开 BIOS 目录。 */
  virtual void openDirectory() = 0;

  /** 订阅内容变化；返回取消订阅的函数。 */
  [[nodiscard]] virtual std::function<void()> subscribe(std::function<void()> handler) = 0;
};

}  // namespace ember::win32::contracts
