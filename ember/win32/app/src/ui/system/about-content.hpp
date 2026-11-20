/*
 * ------------------------------------------------------------------------
 * 名称：about-content.hpp
 * 说明：“关于 Ember”对话框的内容：头部是标志、应用名（大字，与标志垂直居中），右边靠右对齐两行：应用版本与核心版本、下一行是编译时间；作者、邮箱、QQ 号、项目地址（链接）、许可证（竖排），以及“第三方依赖”小节（一行说明和一张表：名称与版本、作者、许可）。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-21
 * 备注：文字取自模型，界面只显示；版本读取中显示“正在读取…”，读不到显示“读取失败”；表格里名称一列的悬停提示是该项的用途；规则见 docs/05-界面.md 第 8 节。
 * ------------------------------------------------------------------------
 */

#pragma once

#include <functional>
#include <memory>
#include <string>

#include "contracts/system/about-model.hpp"
#include "rujian/ui/button.hpp"
#include "rujian/ui/description-row.hpp"
#include "rujian/ui/layout.hpp"
#include "rujian/ui/scroll-area.hpp"
#include "rujian/ui/table.hpp"
#include "rujian/ui/widget.hpp"

namespace ember::win32::ui {

/** “关于 Ember”对话框的内容。 */
class AboutContent final : public rujian::ui::Widget {
 public:
  /**
   * @param model 关于对话框的模型。
   * @param logo 标志控件（应用图标），由装配代码提供；为空时不显示标志。
   */
  AboutContent(contracts::AboutModel& model, std::unique_ptr<rujian::ui::Widget> logo);
  ~AboutContent() override;

  [[nodiscard]] rujian::ui::Size measure(rujian::ui::Size available) override;
  /** 头部、信息行不折行、表格各列放得下所需的宽度；高度没有要求（内容可以滚动）。 */
  [[nodiscard]] rujian::ui::Size minimumSize() override;
  void arrange(const rujian::ui::Rect& bounds) override;

  /** 右上角的版本一行文字。 */
  [[nodiscard]] std::string versionsText() const { return versions_->text(); }
  /** 版本下面的编译时间一行文字。 */
  [[nodiscard]] std::string buildTimeText() const { return buildTime_->text(); }
  /** 项目地址的链接按钮。 */
  [[nodiscard]] rujian::ui::Button& projectLink() const { return *projectLink_; }
  /** 第三方依赖表格。 */
  [[nodiscard]] rujian::ui::Table& table() const { return *table_; }

 private:
  void refresh();

  contracts::AboutModel& model_;
  std::function<void()> unsubscribe_;
  rujian::ui::ScrollArea* scroll_{nullptr};
  rujian::ui::Widget* header_{nullptr};
  rujian::ui::Widget* facts_{nullptr};
  rujian::ui::DescriptionRow* versions_{nullptr};
  rujian::ui::DescriptionRow* buildTime_{nullptr};
  rujian::ui::Button* projectLink_{nullptr};
  rujian::ui::Table* table_{nullptr};
  std::string shownVersions_;
};

}  // namespace ember::win32::ui
