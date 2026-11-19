/*
 * ------------------------------------------------------------------------
 * 名称：runtime-panel-content.hpp
 * 说明：底部面板“运行”的内容，没有分组标题，只有一行：四个控制按钮（开机或断电、暂停或继续、软复位、硬复位）与声音（静音按钮、音量滑块与数值）。主机型号、卡带、mapper 插件显示在系统栏。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-19
 * 备注：规则见 docs/05-界面.md 第 4.1 节；按钮的可用状态来自模型，操作结束后焦点回到画面（模型负责）；拖动音量滑块与点静音时不把焦点抢回画面；
 *       面板收起时内容卸载，展开时重新创建，所以模型的订阅在控件销毁时取消。
 * ------------------------------------------------------------------------
 */

#pragma once

#include <functional>

#include "contracts/runtime/runtime-model.hpp"
#include "rujian/ui/button.hpp"
#include "rujian/ui/label.hpp"
#include "rujian/ui/slider.hpp"
#include "rujian/ui/widget.hpp"

namespace ember::win32::ui {

/** 运行面板的内容。 */
class RuntimePanelContent final : public rujian::ui::Widget {
 public:
  /** @param model 运行模块的模型，必须比本控件活得久。 */
  explicit RuntimePanelContent(contracts::RuntimeModel& model);
  ~RuntimePanelContent() override;

  [[nodiscard]] rujian::ui::Size measure(rujian::ui::Size available) override;

  /** 按钮与滑块（测试用）。 */
  [[nodiscard]] rujian::ui::Button& powerButton() { return *power_; }
  [[nodiscard]] rujian::ui::Button& pauseButton() { return *pause_; }
  [[nodiscard]] rujian::ui::Button& softResetButton() { return *softReset_; }
  [[nodiscard]] rujian::ui::Button& hardResetButton() { return *hardReset_; }
  [[nodiscard]] rujian::ui::Button& muteButton() { return *mute_; }
  [[nodiscard]] rujian::ui::Slider& volumeSlider() { return *volume_; }
  [[nodiscard]] const rujian::ui::Label& volumeLabel() const { return *volumeValue_; }

 private:
  void refresh();

  contracts::RuntimeModel& model_;
  rujian::ui::Widget* row_{nullptr};
  rujian::ui::Button* power_{nullptr};
  rujian::ui::Button* pause_{nullptr};
  rujian::ui::Button* softReset_{nullptr};
  rujian::ui::Button* hardReset_{nullptr};
  rujian::ui::Button* mute_{nullptr};
  rujian::ui::Slider* volume_{nullptr};
  rujian::ui::Label* volumeValue_{nullptr};
  std::function<void()> unsubscribe_;
};

}  // namespace ember::win32::ui
