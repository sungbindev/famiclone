/*
 * ------------------------------------------------------------------------
 * 名称：preferences-content.hpp
 * 说明：“首选项”对话框的内容：左边是分组导航（只用来切换），右边是该分组的设置项（每项一行），改动立即保存，取值与出厂默认不同时可恢复默认；“机型与 BIOS”分组显示说明、“打开 BIOS 目录”链接和每个整机机型一行。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-20
 * 备注：文字与可用状态取自模型，设置项由核心登记，界面不认识具体的设置项；
 *       页面不显示操作结果：保存失败用通知（由控制器发出），控件恢复成模型里的值；输入框在回车或失焦时提交，整数不合格时只把输入框标红并在行内给出说明；
 *       模型变化时：分组或行的结构变了才重建右边的内容，只有取值变了就原地更新（输入中的内容不被无关的变化冲掉）；规则见 docs/05-界面.md 第 4.5 节。
 * ------------------------------------------------------------------------
 */

#pragma once

#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "contracts/preferences/preferences-model.hpp"
#include "rujian/ui/button.hpp"
#include "rujian/ui/checkbox.hpp"
#include "rujian/ui/layout.hpp"
#include "rujian/ui/list.hpp"
#include "rujian/ui/scroll-area.hpp"
#include "rujian/ui/select.hpp"
#include "rujian/ui/setting-row.hpp"
#include "rujian/ui/split-view.hpp"
#include "rujian/ui/text-input.hpp"
#include "rujian/ui/tree.hpp"
#include "rujian/ui/widget.hpp"

namespace ember::win32::ui {

/** 首选项对话框的内容。 */
class PreferencesContent final : public rujian::ui::Widget {
 public:
  /** 一行设置项的控件（给测试读）。 */
  struct RowWidgets {
    std::string id;
    contracts::PreferenceControl control{contracts::PreferenceControl::Text};
    rujian::ui::SettingRow* row{nullptr};
    rujian::ui::Select* select{nullptr};
    rujian::ui::Checkbox* checkbox{nullptr};
    rujian::ui::TextInput* input{nullptr};
    rujian::ui::Button* pick{nullptr};
    rujian::ui::Button* reset{nullptr};
    /** 输入框里的内容是用户改过、还没有提交的。 */
    bool dirty{false};
    /** 下一次模型变化时无条件把输入框恢复成模型里的值（刚提交过）。 */
    bool resync{false};
    /** 输入框里上一次按模型设置的文字。 */
    std::string shownText;
    /** 行里显示的校验说明，空表示没有。 */
    std::string error;
  };

  /** “机型与 BIOS”分组里一行的按钮（给测试读）。 */
  struct BiosWidgets {
    std::string machine;
    rujian::ui::Button* add{nullptr};
    rujian::ui::Button* replace{nullptr};
    rujian::ui::Button* remove{nullptr};
  };

  /**
   * @param preferences 首选项模型。
   * @param bios “机型与 BIOS”分组的模型。
   * @param userData “应用”分组里“用户数据目录”一行的模型。
   */
  PreferencesContent(contracts::PreferencesModel& preferences, contracts::BiosModel& bios, contracts::UserDataModel& userData);
  ~PreferencesContent() override;

  [[nodiscard]] rujian::ui::Size measure(rujian::ui::Size available) override;
  void arrange(const rujian::ui::Rect& bounds) override;

  [[nodiscard]] rujian::ui::Tree& tree() const { return *tree_; }
  [[nodiscard]] rujian::ui::SplitView& split() const { return *split_; }
  /** 右边内容区的滚动区域。 */
  [[nodiscard]] rujian::ui::ScrollArea& pane() const { return *pane_; }
  [[nodiscard]] const std::string& selectedGroupId() const { return selected_; }
  /** 当前分组的各行设置项。 */
  [[nodiscard]] const std::vector<std::unique_ptr<RowWidgets>>& rows() const { return rows_; }
  [[nodiscard]] const std::vector<BiosWidgets>& biosRows() const { return biosRows_; }
  /** “打开 BIOS 目录”链接；当前不是“机型与 BIOS”分组时为空。 */
  [[nodiscard]] rujian::ui::Button* openDirectoryLink() const { return openDirectory_; }
  /** “应用”分组里的“用户数据目录”链接与切换按钮；当前不是“应用”分组时为空。 */
  [[nodiscard]] rujian::ui::Button* userDataLink() const { return userDataLink_; }
  [[nodiscard]] rujian::ui::Button* userDataSwitch() const { return userDataSwitch_; }

 private:
  void refresh();
  void rebuildPane(const contracts::PreferencesView& view, const contracts::PreferenceGroupView* group, const contracts::BiosView& bios, const contracts::UserDataView& userData);
  void updateRows(const contracts::PreferenceGroupView& group);
  std::unique_ptr<RowWidgets> makeRow(const contracts::PreferenceRow& row, rujian::ui::List& list);
  /** 在列表末尾加“用户数据目录”一行（“应用”分组）。 */
  void addUserDataRow(rujian::ui::List& list, const contracts::UserDataView& userData);
  void submit(RowWidgets& state);
  [[nodiscard]] static std::string signatureOf(const contracts::PreferencesView& view, const contracts::PreferenceGroupView* group, const contracts::BiosView& bios, const contracts::UserDataView& userData);

  contracts::PreferencesModel& preferences_;
  contracts::BiosModel& bios_;
  contracts::UserDataModel& userData_;
  std::function<void()> unsubscribePreferences_;
  std::function<void()> unsubscribeBios_;
  rujian::ui::SplitView* split_{nullptr};
  rujian::ui::Tree* tree_{nullptr};
  rujian::ui::ScrollArea* pane_{nullptr};
  std::string selected_;
  std::string paneSignature_;
  std::string treeSignature_;
  std::vector<std::unique_ptr<RowWidgets>> rows_;
  std::vector<BiosWidgets> biosRows_;
  rujian::ui::Button* openDirectory_{nullptr};
  rujian::ui::Button* userDataLink_{nullptr};
  rujian::ui::Button* userDataSwitch_{nullptr};
};

}  // namespace ember::win32::ui
