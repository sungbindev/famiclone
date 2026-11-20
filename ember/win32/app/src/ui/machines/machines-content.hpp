/*
 * ------------------------------------------------------------------------
 * 名称：machines-content.hpp
 * 说明：“机型与 BIOS”对话框的内容：一段说明与“打开 BIOS 目录”链接，下面每个需要 BIOS 的整机机型一行，显示 BIOS 状态与按钮（添加 BIOS 文件，或替换与移除）。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-21
 * 备注：文字与可用状态取自模型；页面不显示操作结果，添加、替换、移除的结果与错误用通知（由控制器发出）；模型变化时只在行的内容变了才重建；规则见 docs/05-界面.md 第 4.7 节。
 * ------------------------------------------------------------------------
 */

#pragma once

#include <functional>
#include <string>
#include <vector>

#include "contracts/machines/machines-model.hpp"
#include "rujian/ui/button.hpp"
#include "rujian/ui/layout.hpp"
#include "rujian/ui/scroll-area.hpp"
#include "rujian/ui/widget.hpp"

namespace ember::win32::ui {

/** “机型与 BIOS”对话框的内容。 */
class MachinesContent final : public rujian::ui::Widget {
 public:
  /** 一个机型一行里的按钮（给测试读）。 */
  struct BiosWidgets {
    std::string machine;
    rujian::ui::Button* add{nullptr};
    rujian::ui::Button* replace{nullptr};
    rujian::ui::Button* remove{nullptr};
  };

  explicit MachinesContent(contracts::BiosModel& bios);
  ~MachinesContent() override;

  [[nodiscard]] rujian::ui::Size measure(rujian::ui::Size available) override;
  void arrange(const rujian::ui::Rect& bounds) override;

  [[nodiscard]] rujian::ui::ScrollArea& pane() const { return *pane_; }
  [[nodiscard]] const std::vector<BiosWidgets>& biosRows() const { return biosRows_; }
  /** “打开 BIOS 目录”链接。 */
  [[nodiscard]] rujian::ui::Button* openDirectoryLink() const { return openDirectory_; }

 private:
  void refresh();
  void rebuild(const contracts::BiosView& view);
  [[nodiscard]] static std::string signatureOf(const contracts::BiosView& view);

  contracts::BiosModel& bios_;
  std::function<void()> unsubscribe_;
  rujian::ui::ScrollArea* pane_{nullptr};
  std::string signature_;
  std::vector<BiosWidgets> biosRows_;
  rujian::ui::Button* openDirectory_{nullptr};
};

}  // namespace ember::win32::ui
