/*
 * ------------------------------------------------------------------------
 * 名称：preferences-model.hpp
 * 说明：首选项模块给界面层的模型：对话框左边的分组与右边每一行设置项要显示的内容、各控件的取值，以及用户的操作（修改、恢复默认、选择文件夹）；“应用”分组里“用户数据目录”一行的内容与操作。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-20
 * 备注：设置项来自核心（preferences.list），由各模块登记，界面不认识具体的设置项；
 *       文字与可用状态由逻辑层算好，界面只显示；操作的结果与错误用外壳通知给出，模型不显示结果；规则见 docs/05-界面.md 第 4.5 节。
 * ------------------------------------------------------------------------
 */

#pragma once

#include <functional>
#include <string>
#include <vector>

namespace ember::win32::contracts {

/** 数据的读取状态。 */
enum class PreferencesLoad {
  /** 正在读取（核心还没有就绪或核心正在重启）。 */
  Loading,
  Ready,
  /** 读取失败，说明在 message 里。 */
  Failed
};

/** 设置项用哪种控件。 */
enum class PreferenceControl {
  /** 下拉框选一个值。 */
  Choice,
  /** 开关（复选框）。 */
  Switch,
  /** 整数输入框，回车或失焦时提交。 */
  Integer,
  /** 文字输入框，回车或失焦时提交。 */
  Text
};

/** 下拉框的一个选项。 */
struct PreferenceChoice {
  std::string value;
  std::string label;
  bool operator==(const PreferenceChoice&) const = default;
};

/** 一行设置项。 */
struct PreferenceRow {
  /** 设置项标识，如 appearance.theme。 */
  std::string id;
  std::string label;
  /** 说明，空表示没有说明。 */
  std::string description;
  PreferenceControl control{PreferenceControl::Text};
  /** 下拉框的选项（Choice）。 */
  std::vector<PreferenceChoice> options;
  /** 当前取值的文字：Choice 是选中项的值，Integer 与 Text 是输入框里的文字。 */
  std::string text;
  /** 开关是否打开（Switch）。 */
  bool checked{false};
  /** 取值是否与出厂默认值不同：不同时显示“恢复默认”。 */
  bool changed{false};
  /** 是否有“选择”文件夹的按钮（取值是文件夹路径的文字设置项）。 */
  bool directoryPicker{false};
  bool operator==(const PreferenceRow&) const = default;
};

/** 左边导航里的一个分组。 */
struct PreferenceGroupView {
  std::string id;
  std::string label;
  /** 分组里显示的设置项。 */
  std::vector<PreferenceRow> rows;
  bool operator==(const PreferenceGroupView&) const = default;
};

/** 首选项对话框此刻要显示的内容。 */
struct PreferencesView {
  PreferencesLoad load{PreferencesLoad::Loading};
  /** 读取失败的说明。 */
  std::string message;
  /** 至少有一个显示的设置项的分组，按显示顺序排列；读取完成前为空。 */
  std::vector<PreferenceGroupView> groups;
  bool operator==(const PreferencesView&) const = default;
};

/** 首选项模块的模型。 */
class PreferencesModel {
 public:
  virtual ~PreferencesModel() = default;

  [[nodiscard]] virtual PreferencesView view() const = 0;

  /** 选一个值（Choice）；保存失败时用通知给出原因，取值保持原样。 */
  virtual void setChoice(const std::string& id, const std::string& value) = 0;
  /** 打开或关闭开关（Switch）。 */
  virtual void setSwitch(const std::string& id, bool value) = 0;
  /**
   * 提交输入框里的文字（Integer、Text）：整数先检查是不是整数、在不在范围内，不合格时返回说明且不提交；文字原样提交；与当前取值相同时什么都不做。
   * @returns 校验不通过的说明；空表示已提交（保存是否成功由通知给出）或无须提交。
   */
  virtual std::string submitText(const std::string& id, const std::string& text) = 0;
  /** 恢复出厂默认值。 */
  virtual void reset(const std::string& id) = 0;
  /** 弹出系统的选择文件夹窗口，选定后保存为这个设置项的取值；用户取消时什么都不发生。 */
  virtual void pickDirectory(const std::string& id) = 0;

  /** 订阅内容变化（包括保存失败后取值没变的情况，界面据此把控件恢复成模型里的值）；返回取消订阅的函数。 */
  [[nodiscard]] virtual std::function<void()> subscribe(std::function<void()> handler) = 0;
};

/** “应用”分组里“用户数据目录”一行要显示的内容。 */
struct UserDataView {
  /** 当前使用的用户数据目录。 */
  std::string directory;
  /** 切换按钮上的文字：改用软件所在目录，或改用用户数据目录。 */
  std::string switchLabel;
  /** 不能切换的原因；空表示能切换。 */
  std::string unavailableReason;
  bool operator==(const UserDataView&) const = default;
};

/** “应用”分组里“用户数据目录”一行的模型。 */
class UserDataModel {
 public:
  virtual ~UserDataModel() = default;

  [[nodiscard]] virtual UserDataView view() const = 0;

  /** 用系统的文件管理器打开当前的用户数据目录，打不开时用通知给出原因。 */
  virtual void openDirectory() = 0;
  /** 切换到另一处：先询问，同意后软件关闭，把数据复制到另一处并核对，再自动重新启动；失败时用通知给出原因。 */
  virtual void switchPlace() = 0;
};

}  // namespace ember::win32::contracts
