/*
 * ------------------------------------------------------------------------
 * 名称：shell-controller.hpp
 * 说明：外壳控制器：同时是外壳接口（给模块用）与外壳的界面模型（给界面层用）：处理命令与快捷键、菜单的内容与选择、底部面板的展开与自动收缩、状态项与标题栏项的内容、拖入的文件。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-19
 * 备注：规则见 docs/05-界面.md 第 2、3、5、6、9 节。
 *       底部面板只经系统栏左侧的面板按钮展开：面板关着时点按钮展开并切到该页签，开着时点同一个按钮收起，点另一个按钮切页签；没有快捷键，每次启动都是关闭，页签选择不保存；
 *       面板的标题栏有“自动收缩”开关的（面板登记 autoCollapse），开关默认开着，开着时点在面板之外就收起面板；用户手动关掉的面板按面板保存在界面状态里，下次启动沿用。
 * ------------------------------------------------------------------------
 */

#pragma once

#include <functional>
#include <set>
#include <string>
#include <vector>

#include "contracts/shell/dropped-files.hpp"
#include "contracts/shell/shell-model.hpp"
#include "contracts/shell/shell-state-service.hpp"
#include "logic/shell/shell-registry-builder.hpp"

namespace ember::win32::logic {

/** 外壳控制器。 */
class ShellController final : public contracts::ShellApi, public contracts::ShellModel {
 public:
  /**
   * @param registry 汇总检查过的登记表。
   * @param window 窗口控制（全屏、置顶、退出）。
   * @param dialogs 对话框的打开者。
   * @param notifier 通知的发出者。
   * @param state 界面状态（保存“自动收缩”开关）。
   * @param dropped 拖入文件的登记服务。
   */
  ShellController(ShellRegistry registry, contracts::ShellWindowControl& window, contracts::ShellDialogOpener& dialogs, contracts::ShellNotifier& notifier, contracts::ShellStateService& state,
                  contracts::DroppedFilesService& dropped);
  ~ShellController() override;
  ShellController(const ShellController&) = delete;
  ShellController& operator=(const ShellController&) = delete;

  /** 接上全部模块的常驻部分；重复调用无效。 */
  void attachModules();
  /** 断开全部模块的常驻部分。 */
  void detachModules();

  /** 窗口的全屏与置顶状态变了（窗口所在的层调用）。 */
  void setWindowState(bool fullScreen, bool alwaysOnTop);

  /** 登记表。 */
  [[nodiscard]] const ShellRegistry& registry() const { return registry_; }

  // ---- ShellApi ----
  void openDialog(const std::string& dialogId) override;
  void togglePanel(const std::string& panelId) override;
  void toggleFullScreen() override;
  void toggleAlwaysOnTop() override;
  void quit() override;
  void runCommand(const std::string& commandId) override;
  void refreshCommands() override;
  void notify(const std::string& text, contracts::NotificationLevel level) override;

  // ---- ShellModel ----
  [[nodiscard]] std::vector<contracts::ShellMenuModel> menus() override;
  void selectMenuItem(const std::string& itemId) override;
  [[nodiscard]] std::optional<std::string> menuForShortcut(const std::string& shortcut) override;
  [[nodiscard]] std::optional<std::string> shortcutFromKey(int virtualKey, unsigned modifiers) const override;
  bool runShortcut(const std::string& shortcut, bool textEditing) override;
  [[nodiscard]] std::vector<contracts::ShellPanelButton> panelButtons() override;
  [[nodiscard]] std::vector<contracts::ShellStatusItemModel> statusItems(bool alignEnd) override;
  [[nodiscard]] std::vector<contracts::ShellTitleBarItemModel> titleBarItems() override;
  void clickStatusItem(const std::string& itemId) override;
  void clickTitleBarItem(const std::string& itemId) override;
  [[nodiscard]] std::optional<contracts::ShellOpenPanel> openPanel() const override;
  void closePanel() override;
  void setPanelAutoCollapse(const std::string& panelId, bool on) override;
  void pressedOutsidePanel() override;
  [[nodiscard]] bool fullScreen() const override { return view_.fullScreen; }
  void dropFiles(const std::vector<std::string>& paths) override;
  void setOnChanged(std::function<void()> handler) override { onChanged_ = std::move(handler); }

 private:
  void changed();
  [[nodiscard]] bool autoCollapseOn(const std::string& panelId) const;
  [[nodiscard]] const contracts::ShellPanelDefinition* findPanel(const std::string& panelId) const;

  ShellRegistry registry_;
  contracts::ShellWindowControl& window_;
  contracts::ShellDialogOpener& dialogs_;
  contracts::ShellNotifier& notifier_;
  contracts::ShellStateService& state_;
  contracts::DroppedFilesService& dropped_;
  contracts::ShellViewState view_;
  /** 用户手动关掉“自动收缩”的面板标识。 */
  std::set<std::string> autoCollapseOff_;
  std::vector<std::function<void()>> detachers_;
  std::function<void()> onChanged_;
};

}  // namespace ember::win32::logic
