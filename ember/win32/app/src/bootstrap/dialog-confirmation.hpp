/*
 * ------------------------------------------------------------------------
 * 名称：dialog-confirmation.hpp
 * 说明：用与其他对话框一致的主题窗口实现的确认询问：弹出带“是”“否”两个按钮的对话框，等用户回答。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-26
 * 备注：模态：归属当前活动窗口（文件管理等对话框里询问时盖在那个对话框上），没有活动窗口时归属主窗口；
 *       等待期间运行嵌套的消息循环，归属窗口不可用；默认按钮是“否”，Esc 与关闭按钮也是“否”；
 *       主题、边框样式与标题栏图标由装配代码在创建主窗口后设置，之前询问用默认的深色主题。
 * ------------------------------------------------------------------------
 */

#pragma once

#include <functional>
#include <memory>
#include <string>

#include "contracts/shell/confirmation.hpp"
#include "rujian/ui/theme.hpp"
#include "rujian/ui/widget.hpp"

namespace ember::win32::bootstrap {

/** 用主题对话框询问。 */
class DialogConfirmation final : public contracts::Confirmation {
 public:
  /** 标题栏图标的工厂：每次询问时创建一个 16×16 的图标控件。 */
  using LogoFactory = std::function<std::unique_ptr<rujian::ui::Widget>()>;

  /** @param fallbackOwner 没有活动窗口时取归属窗口（主窗口句柄）。 */
  explicit DialogConfirmation(std::function<void*()> fallbackOwner);

  void setTheme(rujian::ui::ThemeName theme) { theme_ = theme; }
  void setCustomFrame(bool customFrame) { customFrame_ = customFrame; }
  void setLogo(LogoFactory factory) { logo_ = std::move(factory); }

  [[nodiscard]] bool ask(const std::string& title, const std::string& text) override;

 private:
  std::function<void*()> fallbackOwner_;
  rujian::ui::ThemeName theme_{rujian::ui::ThemeName::Dark};
  bool customFrame_{false};
  LogoFactory logo_;
};

}  // namespace ember::win32::bootstrap
