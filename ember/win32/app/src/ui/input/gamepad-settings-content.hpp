/*
 * ------------------------------------------------------------------------
 * 名称：gamepad-settings-content.hpp
 * 说明：“手柄设置”对话框的内容：顶部一段说明，下面是“玩家 1”“玩家 2”两个页签，每个页签是一个玩家的设置（输入来源、使用的手柄、手柄图、说明行与“清除”），最下面是“恢复默认”。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-20
 * 备注：对应 Electron 外壳的 GamepadSettings.tsx 与 PadSlotEditor.tsx；文字与可用状态取自模型，界面只显示与转发操作；页面不显示操作结果（保存失败的通知由控制器发出）；
 *       模型变化时原地更新所有控件，只有说明文字、下拉框是否显示这类会改变高度的内容变化时才请求重新布局；对话框内容显示期间按模型给的间隔定时读手柄（刷新手柄列表、学习实体手柄）；规则见 docs/05-界面.md 第 4.6 节。
 * ------------------------------------------------------------------------
 */

#pragma once

#include <memory>
#include <string>
#include <vector>

#include "contracts/input/gamepad-settings-model.hpp"
#include "rujian/ui/button.hpp"
#include "rujian/ui/callout.hpp"
#include "rujian/ui/description-row.hpp"
#include "rujian/ui/field.hpp"
#include "rujian/ui/layout.hpp"
#include "rujian/ui/pad-diagram.hpp"
#include "rujian/ui/radio-group.hpp"
#include "rujian/ui/select.hpp"
#include "rujian/ui/state.hpp"
#include "rujian/ui/tabs.hpp"
#include "rujian/ui/widget.hpp"

namespace ember::win32::ui {

/** 手柄设置对话框的内容。 */
class GamepadSettingsContent final : public rujian::ui::Widget {
 public:
  /** 一个玩家页签里的控件（给测试读）。 */
  struct PlayerWidgets {
    rujian::ui::RadioGroup* source{nullptr};
    rujian::ui::Field* deviceField{nullptr};
    rujian::ui::Select* device{nullptr};
    rujian::ui::PadDiagram* diagram{nullptr};
    rujian::ui::DescriptionRow* status{nullptr};
    rujian::ui::Button* clear{nullptr};
  };

  /** @param model 手柄设置模型，必须比本对象活得久。 */
  explicit GamepadSettingsContent(contracts::GamepadSettingsModel& model);
  ~GamepadSettingsContent() override;

  [[nodiscard]] rujian::ui::Size measure(rujian::ui::Size available) override;
  void arrange(const rujian::ui::Rect& bounds) override;
  void onTimer(unsigned id) override;

  [[nodiscard]] rujian::ui::Tabs& tabs() const { return *tabs_; }
  [[nodiscard]] rujian::ui::Button& resetButton() const { return *reset_; }
  [[nodiscard]] rujian::ui::State& failedState() const { return *failed_; }
  [[nodiscard]] const std::vector<PlayerWidgets>& players() const { return players_; }

 private:
  void refresh();
  void scheduleTimer();
  PlayerWidgets makePlayer(int player, rujian::ui::Widget& page);

  contracts::GamepadSettingsModel& model_;
  std::function<void()> unsubscribe_;
  rujian::ui::Stack* stack_{nullptr};
  rujian::ui::Callout* intro_{nullptr};
  rujian::ui::State* failed_{nullptr};
  rujian::ui::Tabs* tabs_{nullptr};
  rujian::ui::Widget* resetRow_{nullptr};
  rujian::ui::Button* reset_{nullptr};
  std::vector<PlayerWidgets> players_;
  /** 上一次影响布局的内容签名：没变就不重新布局。 */
  std::string layoutSignature_;
  unsigned timer_{0};
};

}  // namespace ember::win32::ui
