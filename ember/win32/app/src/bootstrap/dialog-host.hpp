/*
 * ------------------------------------------------------------------------
 * 名称：dialog-host.hpp
 * 说明：对话框的打开者：按外壳登记的对话框标识打开独立的对话框窗口，同一个对话框同时只开一个，再次打开时聚焦已有的。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-20
 * 备注：对话框的标题与大小来自外壳登记表的 ShellDialogDefinition，正文由界面层经工厂提供；
 *       关闭请求（Esc、关闭按钮、Alt+F4）延后到消息处理之外再销毁窗口；主题随主窗口，由装配代码在主题变化时调用 setTheme。
 * ------------------------------------------------------------------------
 */

#pragma once

#include <functional>
#include <map>
#include <memory>
#include <set>
#include <string>

#include "contracts/shell/shell-model.hpp"
#include "contracts/shell/shell-registry.hpp"
#include "platform/window/dialog-window.hpp"
#include "rujian/ui/widget.hpp"

namespace ember::win32::bootstrap {

/** 对话框的打开者。 */
class DialogHost final : public contracts::ShellDialogOpener {
 public:
  /** 对话框正文的工厂：每次打开时创建一个新的正文。 */
  using BodyFactory = std::function<std::unique_ptr<rujian::ui::Widget>()>;
  /** 标题栏图标的工厂：每次打开时创建一个 16×16 的图标控件。 */
  using LogoFactory = std::function<std::unique_ptr<rujian::ui::Widget>()>;

  /**
   * @param owner 取归属窗口（主窗口句柄）。
   * @param post 把任务投递到界面线程（在当前消息处理之后执行）。
   * @param customFrame 是否用自绘的阴影与边框（见 FramelessFrame）。
   */
  DialogHost(std::function<void*()> owner, std::function<void(std::function<void()>)> post, bool customFrame);
  ~DialogHost() override;
  DialogHost(const DialogHost&) = delete;
  DialogHost& operator=(const DialogHost&) = delete;

  /** 登记表里的对话框定义。 */
  void setDefinitions(std::map<std::string, contracts::ShellDialogDefinition> definitions);

  /**
   * 设置一个对话框的正文工厂。
   * @param fills 正文是否铺满对话框的正文区（正文自己负责滚动，如左右分栏的对话框）；否则正文按内容高度排列，放不下时由对话框滚动。
   */
  void setBody(const std::string& dialogId, BodyFactory factory, bool fills = false);

  /** 设置对话框标题栏的图标（应用图标）；没有设置时不显示。 */
  void setLogo(LogoFactory factory) { logo_ = std::move(factory); }

  /** 切换全部对话框（已打开的与以后打开的）的主题。 */
  void setTheme(rujian::ui::ThemeName theme);

  /** 设置一个对话框的物理按键拦截（见 DialogWindowOptions::onPhysicalKey）：对话框打开期间调用它的按键都先经过这里。 */
  void setKeyCapture(const std::string& dialogId, std::function<bool(const std::string& code)> capture);

  /** 设置一个对话框的快捷键拦截（见 DialogWindowOptions::onShortcutKey）：对话框打开期间按下的键都先经过这里。 */
  void setShortcutCapture(const std::string& dialogId, std::function<bool(int virtualKey, unsigned modifiers)> capture);

  /**
   * 打开钩子：对话框窗口创建并显示之后调用，参数是窗口与正文；返回的对象跟着对话框保存，关闭时先于窗口销毁（用来给窗口挂靠窗口句柄的资源，如拖放目标）。
   */
  using OpenHook = std::function<std::shared_ptr<void>(platform::DialogWindow& window, rujian::ui::Widget& body)>;
  void setOpenHook(const std::string& dialogId, OpenHook hook);

  /** 现在打开着的对话框个数。 */
  [[nodiscard]] std::size_t openCount() const { return open_.size(); }

  /** 打开对话框；没有登记、没有正文工厂时什么都不做，已经打开时聚焦它。 */
  void openDialog(const std::string& dialogId) override;

  /** 关闭一个对话框（没有打开时什么都不做）。 */
  void closeDialog(const std::string& dialogId);

  /** 马上关闭并销毁全部打开着的对话框：对话框的内容引用着外壳的模型，要在这些模型之前销毁时用。 */
  void closeAll();

 private:
  std::function<void*()> owner_;
  std::function<void(std::function<void()>)> post_;
  bool customFrame_;
  rujian::ui::ThemeName theme_{rujian::ui::ThemeName::Dark};
  std::map<std::string, contracts::ShellDialogDefinition> definitions_;
  std::map<std::string, BodyFactory> bodies_;
  LogoFactory logo_;
  std::set<std::string> fillingBodies_;
  std::map<std::string, std::function<bool(const std::string&)>> keyCaptures_;
  std::map<std::string, std::function<bool(int, unsigned)>> shortcutCaptures_;
  std::map<std::string, OpenHook> openHooks_;
  /** 打开钩子返回的对象，跟着对话框保存。 */
  std::map<std::string, std::shared_ptr<void>> attachments_;
  std::map<std::string, std::unique_ptr<platform::DialogWindow>> open_;
  std::shared_ptr<bool> alive_{std::make_shared<bool>(true)};
};

}  // namespace ember::win32::bootstrap
