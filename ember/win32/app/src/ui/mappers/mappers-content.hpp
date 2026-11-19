/*
 * ------------------------------------------------------------------------
 * 名称：mappers-content.hpp
 * 说明：“mapper 插件”对话框的内容：顶部是搜索框、统计文字、来源筛选与“安装插件”按钮，下面是插件表格（名称、版本、匹配、主机型号、来源、状态），选中一行后在这一行正下方展开清单摘要与“停用/启用”“卸载”按钮。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-21
 * 备注：文字与可用状态取自模型，界面只显示；预装插件的“卸载”置灰；操作的结果与错误用通知，页面不显示；
 *       模型变化时只重建表格部分，搜索框与下拉框保持不动（输入中的内容与焦点不丢）；规则见 docs/05-界面.md 第 4.4 节。
 * ------------------------------------------------------------------------
 */

#pragma once

#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "contracts/mappers/mappers-model.hpp"
#include "rujian/ui/button.hpp"
#include "rujian/ui/layout.hpp"
#include "rujian/ui/scroll-area.hpp"
#include "rujian/ui/select.hpp"
#include "rujian/ui/table.hpp"
#include "rujian/ui/text-input.hpp"
#include "rujian/ui/toolbar.hpp"
#include "rujian/ui/widget.hpp"

namespace ember::win32::ui {

/** mapper 插件对话框的内容。 */
class MappersContent final : public rujian::ui::Widget {
 public:
  /** 表格里选中行详情的按钮（给测试读）。 */
  struct DetailWidgets {
    rujian::ui::Button* toggle{nullptr};
    rujian::ui::Button* uninstall{nullptr};
  };

  explicit MappersContent(contracts::MappersModel& model);
  ~MappersContent() override;

  [[nodiscard]] rujian::ui::Size measure(rujian::ui::Size available) override;
  void arrange(const rujian::ui::Rect& bounds) override;
  void onTimer(unsigned id) override;

  [[nodiscard]] rujian::ui::TextInput& search() const { return *search_; }
  [[nodiscard]] rujian::ui::Select& source() const { return *source_; }
  [[nodiscard]] rujian::ui::Button& install() const { return *install_; }
  [[nodiscard]] rujian::ui::Toolbar& toolbar() const { return *toolbar_; }
  /** 插件表格；没有可显示的插件（读取中、失败、没有符合条件的）时为空。 */
  [[nodiscard]] rujian::ui::Table* table() const { return table_; }
  /** 表格各行对应的插件标识。 */
  [[nodiscard]] const std::vector<std::string>& rowIds() const { return rowIds_; }
  /** 选中插件的详情按钮；没有选中时两个指针都为空。 */
  [[nodiscard]] const DetailWidgets& detail() const { return detail_; }
  /** 表格上方的提示文字（读取中、失败、没有符合条件的插件）；没有提示时为空。 */
  [[nodiscard]] const std::string& statusText() const { return statusText_; }

 private:
  void refresh();
  void rebuildBody(const contracts::MappersView& view);
  /** 选中行的详情：标题是插件名，右侧是启停与卸载按钮，内容是清单摘要。 */
  [[nodiscard]] std::unique_ptr<rujian::ui::Widget> makeDetail(const contracts::MappersView& view);
  [[nodiscard]] static std::string signatureOf(const contracts::MappersView& view);

  contracts::MappersModel& model_;
  std::function<void()> unsubscribe_;
  rujian::ui::ScrollArea* scroll_{nullptr};
  rujian::ui::Toolbar* toolbar_{nullptr};
  rujian::ui::TextInput* search_{nullptr};
  rujian::ui::Select* source_{nullptr};
  rujian::ui::Button* install_{nullptr};
  rujian::ui::Stack* body_{nullptr};
  rujian::ui::Table* table_{nullptr};
  std::vector<std::string> rowIds_;
  DetailWidgets detail_;
  std::string statusText_;
  std::string signature_;
  std::string shownStats_;
  /** 延后重建的定时器编号，0 表示没有。 */
  unsigned refreshTimer_{0};
  /** 正在按模型设置搜索框与下拉框的值：这时它们的变化回调不再回到模型。 */
  bool syncing_{false};
};

}  // namespace ember::win32::ui
