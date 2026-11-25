/*
 * ------------------------------------------------------------------------
 * 名称：diskimages-content.hpp
 * 说明：“磁盘镜像管理”对话框的内容：上面一条显示镜像目录与“选择目录”按钮，下面左边是镜像目录里的文件与文件夹树（只读，显示完整文件名、文件系统格式与大小），右边是所选镜像当前文件夹里的文件列表（名称、类型、大小、修改时间）。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-25
 * 备注：文字与镜像里的内容取自模型；对话框里不显示操作结果，结果与错误由控制器用通知给出；做不了的操作（镜像只读、没有子目录的文件系统里新建文件夹等）不去调用模型，在合适的位置弹一个 2 秒的提示。
 *       树里右键只有不改变镜像内容的项（新建空白镜像、载入软驱 A、在资源管理器中显示）；文件列表里的重命名、移动、删除等一律立即生效，不确认，没有全选。
 *       键盘：列表里上下、回车打开文件夹、Backspace 回上一级、F2 重命名、Delete 删除、Ctrl+X/C/V 剪切复制粘贴、Ctrl+Z 撤销、Shift+F10 与菜单键打开右键菜单。
 *       拖放：文件列表里的条目拖到 Windows 里是导出，拖到列表里的文件夹上是移动；从 Windows 拖进列表是导入（拖在文件夹上导进那个文件夹，否则导进当前文件夹）；拖放由应用层完成，这里只给出 dragOver、dropFiles 等入口。
 * ------------------------------------------------------------------------
 */

#pragma once

#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "contracts/diskimages/diskimages-model.hpp"
#include "rujian/ui/widget.hpp"

namespace rujian::ui {
class Button;
class ContextMenu;
class FileTable;
class HintBubble;
class Label;
class ScrollArea;
class SplitView;
class State;
class Tree;
}  // namespace rujian::ui

namespace ember::win32::ui {

/** “磁盘镜像管理”对话框的内容。 */
class DiskImagesContent final : public rujian::ui::Widget {
 public:
  /**
   * @param model 磁盘镜像管理的模型，必须比本控件活得久。
   * @param dragHost 把条目拖到 Windows 里的能力，必须比本控件活得久。
   */
  DiskImagesContent(contracts::DiskImagesModel& model, contracts::DiskImagesDragHost& dragHost);
  ~DiskImagesContent() override;

  [[nodiscard]] rujian::ui::Size measure(rujian::ui::Size available) override { return available; }
  void arrange(const rujian::ui::Rect& bounds) override;
  bool onKeyDown(const rujian::ui::KeyEvent& event) override;

  /** 把键盘焦点放到文件列表上（对话框刚打开时用）。 */
  void focusList();

  /**
   * 拖动经过窗口：返回这里能不能放；窗口坐标（逻辑像素）。
   * @param internal 是本程序自己拖出来的条目（移动）还是从 Windows 拖进来的文件（导入）。
   */
  bool dragOver(bool internal, rujian::ui::Point point);
  /** 拖动离开窗口或放下之后：去掉放置目标的标记。 */
  void dragLeave();
  /** 从 Windows 拖进来的文件放下：导入。 */
  void dropFiles(rujian::ui::Point point, const std::vector<std::string>& paths);
  /** 本程序自己拖出来的条目放下：移动到放下处的文件夹里。 */
  void dropInternal(rujian::ui::Point point);

  [[nodiscard]] rujian::ui::Tree& tree() const { return *tree_; }
  [[nodiscard]] rujian::ui::FileTable& table() const { return *table_; }
  [[nodiscard]] rujian::ui::ContextMenu& contextMenu() const { return *menu_; }

 private:
  /** 本程序自己正在拖的条目。 */
  struct ActiveDrag {
    std::uint64_t token{0};
    std::string image;
    std::vector<std::string> directory;
    std::vector<std::string> names;
  };
  /** 放下的位置对应的目标：要放进的文件夹（空是当前文件夹）。 */
  struct DropTarget {
    std::string folder;
  };

  void refresh();
  void updateTree();
  void updateImage(const contracts::DiskImageView& previous);
  void tryStartRename();

  void showTreeMenu(const std::string& id, rujian::ui::Point position);
  void showListMenu(const std::vector<std::string>& ids, rujian::ui::Point position, bool highlightFirst);
  void runListCommand(const std::string& command);
  bool handleListKey(const rujian::ui::KeyEvent& event);

  void commandOpen();
  void commandRename();
  void commandDelete();
  void commandCut();
  void commandCopy();
  void commandPaste();
  void commandNewFolder();
  void commandUndo();
  void startDrag(const std::vector<std::string>& ids);

  /** 当前镜像不能修改时弹出原因并返回假。 */
  bool requireModify(const rujian::ui::Rect& anchor);
  /** 选中的条目名（不含“上一级”那一行）。 */
  [[nodiscard]] std::vector<std::string> selectedNames() const;
  /** 键盘操作被拒绝时提示的位置：焦点行。 */
  [[nodiscard]] rujian::ui::Rect focusAnchor() const;
  void showHint(const rujian::ui::Rect& anchor, const std::string& text);
  [[nodiscard]] const contracts::DiskTreeNode* findNode(const std::string& path) const;
  [[nodiscard]] std::optional<DropTarget> dropTarget(rujian::ui::Point point, bool internal);

  contracts::DiskImagesModel& model_;
  contracts::DiskImagesDragHost& dragHost_;
  std::function<void()> unsubscribe_;
  std::shared_ptr<bool> alive_{std::make_shared<bool>(true)};

  rujian::ui::Label* rootLabel_{nullptr};
  rujian::ui::Button* rootButton_{nullptr};
  rujian::ui::SplitView* split_{nullptr};
  rujian::ui::Tree* tree_{nullptr};
  rujian::ui::State* treeNotice_{nullptr};
  rujian::ui::Label* infoLabel_{nullptr};
  rujian::ui::ScrollArea* listScroll_{nullptr};
  rujian::ui::FileTable* table_{nullptr};
  rujian::ui::State* listNotice_{nullptr};
  rujian::ui::ContextMenu* menu_{nullptr};
  std::unique_ptr<rujian::ui::HintBubble> hint_;

  contracts::DiskTreeView treeView_;
  contracts::DiskImageView imageView_;
  /** 新建文件夹之后要接着重命名的名称。 */
  std::string pendingRename_;
  std::optional<ActiveDrag> activeDrag_;
  std::uint64_t nextToken_{0};
  /** 这一次拖放是否已经提示过“不能放”。 */
  bool dropHintShown_{false};
};

}  // namespace ember::win32::ui
