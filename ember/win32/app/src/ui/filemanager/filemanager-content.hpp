/*
 * ------------------------------------------------------------------------
 * 名称：filemanager-content.hpp
 * 说明：“媒体库”对话框的内容：左边是标题为“资源管理器”的左栏（标题行有“选择目录/更改目录”“折叠一级”与“显示/隐藏文件内容”三个图标按钮，悬停时有提示，下面是目录里的文件夹与文件树（磁盘镜像、nes 卡带、png 图片、音频、不支持的文件），像资源管理器，默认全部收起，每行只显示图标与完整文件名（大小等信息在右边），文件夹展开时才读它里面的内容，放不下时出现横向滚动条）；右边是所选文件的内容（磁盘镜像是它当前文件夹里的文件，其他文件只有它自己一行；名称、类型、大小、修改时间四列固定，表头列分割线可拖动调整列宽，放不下时出现横向滚动条），默认不显示，点“显示文件内容”后展开，这时左栏与右栏之间才有分割线。目录的路径不在对话框内容里显示，经 setOnRootChanged 交给窗口放在标题栏标题的后面。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-25
 * 备注：文字与镜像里的内容取自模型；对话框里不显示操作结果，结果与错误由控制器用通知给出；做不了的操作（镜像只读、没有子目录的文件系统里新建文件夹等）不去调用模型，在合适的位置弹一个 2 秒的提示。
 *       树里的文件操作是真正的文件管理：单击文件夹选中并展开或收起（右边不显示内容），单击镜像选中并在右边显示它，点空白处取消选中（右边也不显示内容）；“折叠一级”每点一次只收起最深一层已展开的文件夹；F2 重命名（文件只选中扩展名之前的部分）、Delete 删除（移进回收站，先确认）、Ctrl+X/C/V 剪切复制粘贴，右键菜单还有新建文件夹、新建空白镜像、在资源管理器中显示，并按文件种类给出载入软驱 A（软盘镜像）、运行卡带（nes）、载入磁带（音频）、预览（png）；左右两边的右键菜单相同；条目可以拖到文件夹上移动（移动先确认）；删除与移动之外的操作一律立即生效，不确认。
 *       文件列表里的重命名、新建、粘贴等也立即生效，删除与移动（拖到文件夹上、剪切后粘贴）先确认，没有全选；列表顶上没有镜像信息行，表头固定显示，没有选镜像时表头下面是提示；选中软盘镜像时，最后一行的下面（不是窗口最底下）有一行弱化色的概况：格式、容量与可用空间（如“FAT12 · 容量 1.4 MB · 可用 1.37 MB”）。
 *       键盘：列表里上下、回车打开文件夹、Backspace 回上一级、F2 重命名、Delete 删除、Ctrl+X/C/V 剪切复制粘贴、Ctrl+Z 撤销（树里也撤销当前镜像里的操作）、Shift+F10 与菜单键打开右键菜单。
 *       展开与收起文件内容时窗口跟着变宽与变窄（经 setOnResizeWindow 交给窗口，窗口最大化时不变）；只显示左栏时窗口最窄是它的最小宽度。
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

#include "contracts/filemanager/filemanager-model.hpp"
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

/** “媒体库”对话框的内容。 */
class FileManagerContent final : public rujian::ui::Widget {
 public:
  /**
   * @param model 媒体库的模型，必须比本控件活得久。
   * @param dragHost 把条目拖到 Windows 里的能力，必须比本控件活得久。
   */
  FileManagerContent(contracts::FileManagerModel& model, contracts::FileManagerDragHost& dragHost);
  ~FileManagerContent() override;

  [[nodiscard]] rujian::ui::Size measure(rujian::ui::Size available) override { return available; }
  [[nodiscard]] rujian::ui::Size minimumSize() override;
  void arrange(const rujian::ui::Rect& bounds) override;
  bool onKeyDown(const rujian::ui::KeyEvent& event) override;

  /** 把键盘焦点放到资源管理器的树上（对话框刚打开时用）。 */
  void focusInitial();
  /**
   * 设置调整窗口宽度的函数：展开镜像内容时传正数（要增加的宽度），收起时传负数；窗口调整了就返回真，不能调整（如最大化）返回假。
   * 不设置时窗口大小不变。
   */
  void setOnResizeWindow(std::function<bool(double delta)> handler) { onResizeWindow_ = std::move(handler); }
  /**
   * 设置目录路径变化时的回调（没有选过目录时参数是“未选择目录”）；设置时马上用当前的路径调用一次。
   * 窗口把它显示在标题栏的标题后面。
   */
  void setOnRootChanged(std::function<void(const std::string& root)> handler);

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
  /** 展开或收起右边的镜像内容，窗口跟着变宽或变窄。 */
  void toggleImagePane();

  void showTreeMenu(const std::string& id, rujian::ui::Point position);
  /** 树里一个条目对应的目标文件夹：文件夹是它自己，镜像是它所在的文件夹，空串（找不到的条目）是目录本身。 */
  [[nodiscard]] std::string folderFor(const std::string& id) const;
  /** 在树里的文件夹 parent 里新建文件夹，出现在树里后接着重命名。 */
  void newTreeFolder(const std::string& parent);
  [[nodiscard]] bool canDropOnFolder(const std::string& id, const std::string& target) const;
  bool handleTreeKey(const rujian::ui::KeyEvent& event);
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
  /** 右边是否显示镜像内容：左边选中的正是模型当前的镜像（选中文件夹或取消选中时不显示）。 */
  [[nodiscard]] bool imageShown() const { return imageView_.selected && treeSelected_ == imageView_.image; }
  /** 右边显示的是不是软盘镜像（卡带、音频、图片只有它自己一行，不能做镜像里的操作）。 */
  [[nodiscard]] bool floppyShown() const { return imageView_.kind == contracts::kKindFloppy; }
  [[nodiscard]] std::optional<DropTarget> dropTarget(rujian::ui::Point point, bool internal);

  contracts::FileManagerModel& model_;
  contracts::FileManagerDragHost& dragHost_;
  std::function<void()> unsubscribe_;
  std::shared_ptr<bool> alive_{std::make_shared<bool>(true)};

  rujian::ui::Button* rootButton_{nullptr};
  rujian::ui::Button* collapseButton_{nullptr};
  rujian::ui::Button* paneButton_{nullptr};
  rujian::ui::SplitView* split_{nullptr};
  rujian::ui::Tree* tree_{nullptr};
  rujian::ui::State* treeNotice_{nullptr};
  rujian::ui::ScrollArea* listScroll_{nullptr};
  rujian::ui::FileTable* table_{nullptr};
  rujian::ui::State* listNotice_{nullptr};
  rujian::ui::ContextMenu* menu_{nullptr};
  std::unique_ptr<rujian::ui::HintBubble> hint_;

  contracts::DiskTreeView treeView_;
  contracts::DiskImageView imageView_;
  /** 右边的镜像内容是否展开着。 */
  bool imagePaneShown_{false};
  /** 调整窗口宽度的函数，见 setOnResizeWindow。 */
  std::function<bool(double)> onResizeWindow_;
  /** 目录路径（没有时是“未选择目录”）与它变化时的回调，见 setOnRootChanged。 */
  std::string rootText_;
  std::function<void(const std::string&)> onRootChanged_;
  /** 树里选中的条目（镜像或文件夹）；只有选中镜像时右边才显示它的内容。 */
  std::string treeSelected_;
  /** 上一次刷新时右边是否显示着镜像内容。 */
  bool shownImage_{false};
  /** 刚选中的镜像还没在树里出现（比如刚重命名），出现后要展开到它。 */
  bool revealPending_{false};
  /** 刚在树里新建的文件夹或磁盘镜像，出现后接着重命名。 */
  std::string pendingTreeRename_;
  /** 新建文件夹之后要接着重命名的名称。 */
  std::string pendingRename_;
  std::optional<ActiveDrag> activeDrag_;
  std::uint64_t nextToken_{0};
  /** 这一次拖放是否已经提示过“不能放”。 */
  bool dropHintShown_{false};
};

}  // namespace ember::win32::ui
