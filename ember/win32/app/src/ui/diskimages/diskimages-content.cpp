/*
 * ------------------------------------------------------------------------
 * 名称：diskimages-content.cpp
 * 说明：“磁盘镜像管理”对话框内容的实现。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-25
 * 备注：无。
 * ------------------------------------------------------------------------
 */

#include "ui/diskimages/diskimages-content.hpp"

#include <algorithm>

#include "rujian/ui/button.hpp"
#include "rujian/ui/context-menu.hpp"
#include "rujian/ui/file-table.hpp"
#include "rujian/ui/hint-bubble.hpp"
#include "rujian/ui/label.hpp"
#include "rujian/ui/scroll-area.hpp"
#include "rujian/ui/split-view.hpp"
#include "rujian/ui/state.hpp"
#include "rujian/ui/tree.hpp"

namespace ember::win32::ui {

namespace {

using namespace rujian::ui;

/** 用户可见的文字。 */
constexpr const char* kRootNone = "未选择镜像目录";
constexpr const char* kChooseRoot = "选择目录";
constexpr const char* kChangeRoot = "更改目录";
constexpr const char* kExplorerTitle = "资源管理器";
constexpr const char* kCollapseTip = "折叠一级";
constexpr const char* kShowImageTip = "显示镜像内容";
constexpr const char* kHideImageTip = "隐藏镜像内容";
constexpr const char* kNoRootNotice = "还没有选择镜像目录。点上方的“选择目录”，选一个放磁盘镜像的文件夹，里面的镜像会列在这里。";
constexpr const char* kUnavailableNotice = "镜像目录不存在或无法读取，请点上方的“更改目录”重新选择。";
constexpr const char* kEmptyTreeNotice = "目录里是空的。右键点这里可以新建文件夹或空白镜像。";
constexpr const char* kLoadingNotice = "正在读取…";
constexpr const char* kNoImageNotice = "从左边选一个磁盘镜像，这里会显示它里面的文件。";
constexpr const char* kEmptyFolder = "这个文件夹是空的";
constexpr const char* kColumnName = "名称";
constexpr const char* kColumnType = "类型";
constexpr const char* kColumnSize = "大小";
constexpr const char* kColumnModified = "修改时间";
constexpr const char* kParentName = "..";

constexpr const char* kMenuNewImage = "新建空白镜像";
constexpr const char* kMenuLoad = "载入软驱 A";
constexpr const char* kMenuReveal = "在资源管理器中显示";
constexpr const char* kMenuOpen = "打开";
constexpr const char* kMenuRename = "重命名";
constexpr const char* kMenuCut = "剪切";
constexpr const char* kMenuCopy = "复制";
constexpr const char* kMenuPaste = "粘贴";
constexpr const char* kMenuDelete = "删除";
constexpr const char* kMenuNewFolder = "新建文件夹";
constexpr const char* kMenuUndo = "撤销";

constexpr const char* kHintReadOnly = "这个镜像现在不能修改";
constexpr const char* kHintNothingToUndo = "没有可以撤销的操作";
constexpr const char* kHintNoFolders = " 磁盘没有文件夹，不能新建文件夹";

/** “上一级”那一行的标识：条目名里不会有 /，不会重名。 */
constexpr const char* kParentRowId = "/..";

/** 命令标识（右键菜单项的标识）。 */
constexpr const char* kCommandLoad = "load";
constexpr const char* kCommandReveal = "reveal";
constexpr const char* kCommandNewPrefix = "new:";
constexpr const char* kCommandOpen = "open";
constexpr const char* kCommandRename = "rename";
constexpr const char* kCommandCut = "cut";
constexpr const char* kCommandCopy = "copy";
constexpr const char* kCommandPaste = "paste";
constexpr const char* kCommandDelete = "delete";
constexpr const char* kCommandNewFolder = "newfolder";
constexpr const char* kCommandUndo = "undo";

/** 虚拟键码。 */
constexpr int kKeyBack = 0x08;
constexpr int kKeyDelete = 0x2E;
constexpr int kKeyF2 = 0x71;
constexpr int kKeyC = 0x43;
constexpr int kKeyV = 0x56;
constexpr int kKeyX = 0x58;
constexpr int kKeyZ = 0x5A;

/** 资源管理器上方标题行的高度、内边距与标题文字。 */
constexpr double kHeaderRowHeight = 28.0;
constexpr double kHeaderPadding = 10.0;
constexpr double kButtonsPadding = 6.0;
constexpr double kButtonGap = 2.0;
constexpr double kTitleFontSize = 12.0;
constexpr int kTitleFontWeight = 600;
/** 标题首字笔画左边的字面空隙，对齐展开箭头时要扣掉。 */
constexpr double kTitleBearing = 1.0;
/** 资源管理器的初始宽度、最小宽度与最大宽度，镜像内容展开时窗口增加的宽度，以及镜像内容至少要有的宽度。 */
constexpr double kSideWidth = 320.0;
constexpr double kSideMinWidth = 260.0;
constexpr double kSideMaxWidth = 640.0;
constexpr double kImagePaneWidth = 580.0;
constexpr double kImagePaneMinWidth = 300.0;
/** 文件列表里名称列的初始宽度（最后一列占满剩余）。 */
constexpr double kNameColumnWidth = 200.0;
/** 说明块离窗格边缘的距离。 */
constexpr double kNoticeMargin = 8.0;

/** 不拦截鼠标的说明块（盖在树或列表的空白处，右键仍然交给下面的控件）。 */
class Notice final : public State {
 public:
  [[nodiscard]] Widget* hitTest(Point /*point*/) override { return nullptr; }
};

/** 右边的窗格：放在滚动区域里的文件列表（表头固定显示，没选镜像时表头下面是提示），失败时在列表上方盖一块说明。 */
class MainPane final : public Widget {
 public:
  MainPane() {
    auto scroll = std::make_unique<ScrollArea>();
    scroll->setHorizontalScroll(true);
    auto table = std::make_unique<FileTable>();
    table_ = table.get();
    scroll->setContent(std::move(table));
    scroll_ = static_cast<ScrollArea*>(addChild(std::move(scroll)));
    notice_ = static_cast<Notice*>(addChild(std::make_unique<Notice>()));
    notice_->setVisible(false);
  }

  [[nodiscard]] ScrollArea* scroll() const { return scroll_; }
  [[nodiscard]] FileTable* table() const { return table_; }
  [[nodiscard]] State* notice() const { return notice_; }

  [[nodiscard]] Size measure(Size available) override { return available; }

  void arrange(const Rect& bounds) override {
    setBounds(bounds);
    const Rect listArea = bounds;
    scroll_->arrange(listArea);
    if (notice_->visible()) {
      const double width = std::max(0.0, listArea.width - 2 * kNoticeMargin);
      notice_->arrange(Rect{listArea.x + kNoticeMargin, listArea.y + kNoticeMargin, width, notice_->measure(Size{width, 0}).height});
    }
  }

 private:
  ScrollArea* scroll_{nullptr};
  FileTable* table_{nullptr};
  Notice* notice_{nullptr};
};

/** 左边的资源管理器：最上面是标题行（标题与“选择/更改目录”“折叠一级”“显示/隐藏镜像内容”三个图标按钮），下面是放在滚动区域里的树；树没有内容时在它上方盖一块说明。 */
class ExplorerPane final : public Widget {
 public:
  ExplorerPane() {
    auto title = std::make_unique<Label>();
    title->setText(kExplorerTitle);
    title->setWrap(false);
    title->setFontSize(kTitleFontSize);
    title->setFontWeight(kTitleFontWeight);
    title_ = static_cast<Label*>(addChild(std::move(title)));
    change_ = addIconButton(IconId::FolderOpen, kChooseRoot);
    collapse_ = addIconButton(IconId::SquareMinus, kCollapseTip);
    pane_ = addIconButton(IconId::LayoutSidebarRightExpand, kShowImageTip);
    auto scroll = std::make_unique<ScrollArea>();
    scroll->setHorizontalScroll(true);
    auto tree = std::make_unique<Tree>();
    tree_ = tree.get();
    scroll->setContent(std::move(tree));
    scroll_ = static_cast<ScrollArea*>(addChild(std::move(scroll)));
    notice_ = static_cast<Notice*>(addChild(std::make_unique<Notice>()));
    notice_->setVisible(false);
  }

  [[nodiscard]] Button* collapse() const { return collapse_; }
  [[nodiscard]] Button* pane() const { return pane_; }
  [[nodiscard]] Button* change() const { return change_; }
  [[nodiscard]] Tree* tree() const { return tree_; }
  [[nodiscard]] State* notice() const { return notice_; }

  [[nodiscard]] Size measure(Size available) override { return available; }

  void arrange(const Rect& bounds) override {
    setBounds(bounds);
    // 标题行：按钮靠右（显示镜像内容的按钮在最右，往左依次是折叠按钮、选择目录按钮），标题靠左
    double right = bounds.right() - kButtonsPadding;
    for (Button* button : {pane_, collapse_, change_}) {
      const Size size = button->measure(Size{});
      right -= size.width;
      button->arrange(Rect{right, bounds.y + (kHeaderRowHeight - size.height) / 2.0, size.width, size.height});
      right -= kButtonGap;
    }
    // 标题与树第一层的展开箭头左对齐
    const double titleLeft = bounds.x + Tree::arrowLeftInset() - kTitleBearing;
    const double titleWidth = std::max(0.0, right - titleLeft);
    const double titleHeight = title_->measure(Size{titleWidth, 0}).height;
    title_->arrange(Rect{titleLeft, bounds.y + (kHeaderRowHeight - titleHeight) / 2.0, titleWidth, titleHeight});
    const Rect treeArea{bounds.x, bounds.y + kHeaderRowHeight, bounds.width, std::max(0.0, bounds.height - kHeaderRowHeight)};
    scroll_->arrange(treeArea);
    if (notice_->visible()) {
      const double width = std::max(0.0, treeArea.width - 2 * kNoticeMargin);
      notice_->arrange(Rect{treeArea.x + kNoticeMargin, treeArea.y + kNoticeMargin, width, notice_->measure(Size{width, 0}).height});
    }
  }

 private:
  /** 标题行上只有图标的按钮：名称在悬停提示里。 */
  Button* addIconButton(IconId icon, const char* tip) {
    auto button = std::make_unique<Button>(tip);
    button->setVariant(ButtonVariant::Ghost);
    button->setIconOnly(true);
    button->setCompact(true);
    button->setIcon(icon);
    button->setTooltip(tip);
    return static_cast<Button*>(addChild(std::move(button)));
  }

  Label* title_{nullptr};
  Button* collapse_{nullptr};
  Button* pane_{nullptr};
  Button* change_{nullptr};
  ScrollArea* scroll_{nullptr};
  Tree* tree_{nullptr};
  Notice* notice_{nullptr};
};

/** 路径里的 / 个数，即这个节点在树里的层级。 */
int depthOf(const std::string& path) { return static_cast<int>(std::count(path.begin(), path.end(), '/')); }

std::string lowered(std::string text) {
  for (char& c : text) {
    if (c >= 'A' && c <= 'Z') c = static_cast<char>(c - 'A' + 'a');
  }
  return text;
}

/** 树里一个节点的图标。 */
IconId nodeIcon(const contracts::DiskTreeNode& node) {
  if (node.directory) return IconId::Folder;
  if (node.format == "appledos33" || node.format == "prodos") return IconId::Apple;
  if (node.format.empty() || node.format.rfind("fat", 0) == 0) return IconId::DeviceFloppy;
  return IconId::FileUnknown;
}

/** 文件列表里一行的图标：文件夹，或按类型文字（扩展名、类型名）分的几类。 */
IconId entryIcon(const contracts::DiskEntryRow& entry) {
  if (entry.directory) return IconId::Folder;
  const std::string type = lowered(entry.type);
  const auto in = [&type](std::initializer_list<const char*> names) { return std::any_of(names.begin(), names.end(), [&type](const char* name) { return type == name; }); };
  if (in({"txt", "text", "t", "doc", "md", "log", "ini", "cfg"})) return IconId::FileText;
  if (in({"bas", "a", "i", "int", "asm", "s", "c", "h", "inc", "src", "bat", "cmd"})) return IconId::FileCode;
  if (in({"exe", "com", "sys", "bin", "b", "obj", "dll", "rel", "r", "ovl", "drv"})) return IconId::Binary;
  if (in({"bmp", "gif", "jpg", "png", "pcx", "pic", "pac", "shr"})) return IconId::Photo;
  if (in({"zip", "arc", "lzh", "sit"})) return IconId::FileZip;
  return IconId::File;
}

MenuItem menuItem(const char* id, const char* label, bool disabled = false, const char* shortcut = "", std::optional<IconId> icon = std::nullopt, bool separatorBefore = false) {
  MenuItem item;
  item.id = id;
  item.label = label;
  item.disabled = disabled;
  item.shortcut = shortcut;
  item.icon = icon;
  item.separatorBefore = separatorBefore;
  return item;
}

}  // namespace

DiskImagesContent::DiskImagesContent(contracts::DiskImagesModel& model, contracts::DiskImagesDragHost& dragHost) : model_(model), dragHost_(dragHost) {
  auto split = std::make_unique<SplitView>();
  auto explorer = std::make_unique<ExplorerPane>();
  rootButton_ = explorer->change();
  rootButton_->setOnClick([this] { model_.chooseRoot(); });
  collapseButton_ = explorer->collapse();
  collapseButton_->setOnClick([this] { tree_->collapseDeepestLevel(); });
  paneButton_ = explorer->pane();
  paneButton_->setOnClick([this] { toggleImagePane(); });
  treeNotice_ = explorer->notice();
  tree_ = explorer->tree();
  tree_->setDefaultExpanded(false);
  tree_->setToggleOnClick(true);
  tree_->setSelectOnMove(true);
  tree_->setOnSelect([this](const std::string& id) {
    const contracts::DiskTreeNode* node = findNode(id);
    if (node == nullptr) return;
    treeSelected_ = id;
    // 只有选中镜像时右边才显示它的内容；选中文件夹时右边收起内容（模型里仍记着上一个镜像）
    if (!node->directory) model_.selectImage(id);
    refresh();
  });
  tree_->setOnClearSelection([this] {
    treeSelected_.clear();
    refresh();
  });
  tree_->setOnRename([this](const std::string& id, const std::string& name) { model_.renameItem(id, name); });
  tree_->setOnKey([this](const KeyEvent& event) { return handleTreeKey(event); });
  tree_->setCanDrop([this](const std::string& id, const std::string& target) { return canDropOnFolder(id, target); });
  tree_->setOnDrop([this](const std::string& id, const std::string& target) { model_.moveItems({id}, target); });
  tree_->setOnContextMenu([this](const std::string& id, Point position) { showTreeMenu(id, position); });
  split->setSide(std::move(explorer));
  split->setSideFit(true);
  split->setSideWidth(kSideWidth);
  split->setSideWidthRange(kSideMinWidth, kSideMaxWidth);
  auto pane = std::make_unique<MainPane>();
  listScroll_ = pane->scroll();
  table_ = pane->table();
  listNotice_ = pane->notice();
  split->setMain(std::move(pane));
  // 默认只显示资源管理器，镜像内容由标题行上的按钮展开
  split->setMainVisible(false);
  split->setDividerReserved(true);
  split_ = static_cast<SplitView*>(addChild(std::move(split)));
  menu_ = static_cast<ContextMenu*>(addChild(std::make_unique<ContextMenu>()));


  table_->setOnActivate([this](const std::string& id) {
    if (id == kParentRowId) {
      model_.goUp();
      return;
    }
    const auto found = std::find_if(imageView_.entries.begin(), imageView_.entries.end(), [&id](const contracts::DiskEntryRow& row) { return row.name == id; });
    if (found != imageView_.entries.end() && found->directory) model_.openFolder(id);
  });
  table_->setOnContextMenu([this](const std::vector<std::string>& ids, Point position) { showListMenu(ids, position, false); });
  table_->setOnKey([this](const KeyEvent& event) { return handleListKey(event); });
  table_->setOnRename([this](const std::string& id, const std::string& name) { model_.rename(id, name); });
  table_->setOnDragStart([this](const std::vector<std::string>& ids) { startDrag(ids); });
  table_->setEmptyText(kEmptyFolder);

  refresh();
  unsubscribe_ = model_.subscribe([this] { refresh(); });
}

DiskImagesContent::~DiskImagesContent() {
  *alive_ = false;
  if (unsubscribe_) unsubscribe_();
}

void DiskImagesContent::arrange(const Rect& bounds) {
  setBounds(bounds);
  // 镜像内容展开时，资源管理器的宽度不能挤得镜像内容低于最小宽度
  double sideMaximum = kSideMaxWidth;
  if (imagePaneShown_) {
    const double gap = split_->sideExtent() - split_->sideWidth();
    sideMaximum = std::clamp(bounds.width - kImagePaneMinWidth - gap, kSideMinWidth, kSideMaxWidth);
  }
  split_->setSideWidthRange(kSideMinWidth, sideMaximum);
  split_->arrange(bounds);
}

Size DiskImagesContent::minimumSize() {
  // 只显示资源管理器时窗口最窄是它的最小宽度；展开镜像内容后要再放得下它
  const double width = imagePaneShown_ ? split_->sideExtent() + kImagePaneMinWidth : kSideMinWidth;
  return Size{width, 0};
}

void DiskImagesContent::toggleImagePane() {
  if (imagePaneShown_) {
    // 窗口缩回去：去掉镜像内容占的宽度
    const double extent = split_->bounds().width - split_->sideWidth();
    imagePaneShown_ = false;
    split_->setMainVisible(false);
    if (onResizeWindow_) onResizeWindow_(-extent);
  } else {
    // 窗口变宽：资源管理器保持现在的宽度，右边多出来的是镜像内容
    const double current = split_->bounds().width;
    const bool resized = onResizeWindow_ && onResizeWindow_(kImagePaneWidth);
    imagePaneShown_ = true;
    split_->setSideWidth(resized ? current : kSideWidth);
    split_->setMainVisible(true);
  }
  paneButton_->setIcon(imagePaneShown_ ? IconId::LayoutSidebarRightCollapse : IconId::LayoutSidebarRightExpand);
  paneButton_->setTooltip(imagePaneShown_ ? kHideImageTip : kShowImageTip);
  requestLayout();
}

void DiskImagesContent::setOnRootChanged(std::function<void(const std::string& root)> handler) {
  onRootChanged_ = std::move(handler);
  if (onRootChanged_) onRootChanged_(rootText_);
}

void DiskImagesContent::focusInitial() {
  if (host() != nullptr) host()->setFocus(tree_, false);
}

const contracts::DiskTreeNode* DiskImagesContent::findNode(const std::string& path) const {
  const auto found = std::find_if(treeView_.nodes.begin(), treeView_.nodes.end(), [&path](const contracts::DiskTreeNode& node) { return node.path == path; });
  return found == treeView_.nodes.end() ? nullptr : &*found;
}

// ---------------------------------------------------------------- 刷新

void DiskImagesContent::refresh() {
  const contracts::DiskTreeView tree = model_.tree();
  const contracts::DiskImageView previous = imageView_;
  const bool treeChanged = !(tree == treeView_);
  treeView_ = tree;
  imageView_ = model_.image();
  // 模型换了选中的镜像（新建、重命名后）：树里跟着选中，并展开到它
  if (imageView_.selected && imageView_.image != previous.image) {
    treeSelected_ = imageView_.image;
    revealPending_ = true;
  } else if (!imageView_.selected && previous.selected && treeSelected_ == previous.image) {
    treeSelected_.clear();
  }
  if (treeChanged) updateTree();
  else tree_->setSelectedId(treeSelected_);
  const bool shown = imageShown();
  if (!(imageView_ == previous) || treeChanged || shown != shownImage_) updateImage(previous);
  shownImage_ = shown;
  if (revealPending_ && findNode(treeSelected_) != nullptr) {
    tree_->reveal(treeSelected_);
    revealPending_ = false;
  }
  requestLayout();
}

void DiskImagesContent::updateTree() {
  const bool hasRoot = !treeView_.root.empty();
  rootText_ = hasRoot ? treeView_.root : kRootNone;
  if (onRootChanged_) onRootChanged_(rootText_);
  rootButton_->setLabel(hasRoot ? kChangeRoot : kChooseRoot);
  rootButton_->setTooltip(hasRoot ? kChangeRoot : kChooseRoot);

  std::vector<TreeItem> items;
  for (const contracts::DiskTreeNode& node : treeView_.nodes) {
    TreeItem item;
    item.id = node.path;
    item.label = node.name;
    item.depth = depthOf(node.path);
    item.icon = nodeIcon(node);
    item.trailing = node.detail;
    item.folder = node.directory;
    items.push_back(std::move(item));
  }
  tree_->setItems(std::move(items));
  tree_->setSelectedId(treeSelected_);

  std::string notice;
  bool error = false;
  switch (treeView_.state) {
    case contracts::DiskImagesState::Loading:
      notice = kLoadingNotice;
      break;
    case contracts::DiskImagesState::NoRoot:
      notice = kNoRootNotice;
      break;
    case contracts::DiskImagesState::Unavailable:
      notice = kUnavailableNotice;
      error = true;
      break;
    case contracts::DiskImagesState::Failed:
      notice = treeView_.message;
      error = true;
      break;
    case contracts::DiskImagesState::Ready:
      if (treeView_.nodes.empty()) notice = kEmptyTreeNotice;
      break;
  }
  treeNotice_->setText(notice);
  treeNotice_->setError(error);
  treeNotice_->setVisible(!notice.empty());

  // 刚新建的文件夹出现在树里之后，选中它并接着重命名
  if (!pendingTreeRename_.empty() && findNode(pendingTreeRename_) != nullptr) {
    const std::string path = pendingTreeRename_;
    pendingTreeRename_.clear();
    treeSelected_ = path;
    tree_->setSelectedId(path);
    tree_->beginRename(path);
  }
}

void DiskImagesContent::updateImage(const contracts::DiskImageView& previous) {
  const contracts::DiskImageView& view = imageView_;
  const bool shown = imageShown();
  // 换了镜像或文件夹：选中与滚动位置重新来
  const bool moved = view.image != previous.image || view.directory != previous.directory;
  if (moved) {
    table_->cancelRename();
    table_->setSelection({});
    listScroll_->setScrollTop(0);
  }

  // 左边没有可选的镜像时（没设目录、目录读不了或为空），左边已有说明，右边只留表头
  const bool treeUsable = treeView_.state == contracts::DiskImagesState::Ready && !treeView_.nodes.empty();

  std::string notice;
  bool error = false;
  if (shown && view.failed) {
    notice = view.message;
    error = true;
  }
  listNotice_->setText(notice);
  listNotice_->setError(error);
  listNotice_->setVisible(!notice.empty());
  // 有说明时不显示列表（表头与“文件夹是空的”不能压在说明上）；没有说明时表头一直显示
  listScroll_->setVisible(notice.empty());

  const bool showModified = shown && std::any_of(view.entries.begin(), view.entries.end(), [](const contracts::DiskEntryRow& row) { return !row.modified.empty(); });
  std::vector<FileTableColumn> columns{{kColumnName, kNameColumnWidth, false}, {kColumnType, 70, false}, {kColumnSize, 90, true}};
  if (showModified) columns.push_back({kColumnModified, 120, false});
  table_->setColumns(std::move(columns));
  // 读取中还没有内容，不能显示“这个文件夹是空的”；没选镜像时表头下面是提示
  const char* emptyText = "";
  if (!shown) emptyText = treeUsable ? kNoImageNotice : "";
  else if (!view.loading) emptyText = kEmptyFolder;
  table_->setEmptyText(emptyText);

  std::vector<FileTableRow> rows;
  if (shown && !view.failed && !view.directory.empty()) {
    FileTableRow parent;
    parent.id = kParentRowId;
    parent.icon = IconId::ArrowUp;
    parent.cells = {kParentName};
    rows.push_back(std::move(parent));
  }
  if (shown && !view.failed) {
    for (const contracts::DiskEntryRow& entry : view.entries) {
      FileTableRow row;
      row.id = entry.name;
      row.icon = entryIcon(entry);
      row.folder = entry.directory;
      row.cells = {entry.name, entry.type, entry.size};
      if (showModified) row.cells.push_back(entry.modified);
      rows.push_back(std::move(row));
    }
  }
  table_->setRows(std::move(rows));
  tryStartRename();
}

void DiskImagesContent::tryStartRename() {
  if (pendingRename_.empty()) return;
  const auto found = std::find_if(imageView_.entries.begin(), imageView_.entries.end(), [this](const contracts::DiskEntryRow& row) { return row.name == pendingRename_; });
  if (found == imageView_.entries.end()) return;
  const std::string name = pendingRename_;
  pendingRename_.clear();
  table_->setSelection({name});
  if (host() != nullptr) host()->setFocus(table_, false);
  table_->beginRename(name);
}

// ---------------------------------------------------------------- 右键菜单

void DiskImagesContent::showTreeMenu(const std::string& id, Point position) {
  if (treeView_.state != contracts::DiskImagesState::Ready) return;
  const contracts::DiskTreeNode* node = findNode(id);
  const bool isFile = node != nullptr && !node->directory;
  // 新建与粘贴的目标文件夹：点在文件夹上是它自己，点在镜像上是它所在的文件夹，点在空白处是镜像目录本身
  const std::string folder = folderFor(id);
  std::vector<MenuItem> items;
  if (isFile) {
    items.push_back(menuItem(kCommandLoad, kMenuLoad, false, "", IconId::DeviceFloppy));
    items.push_back(menuItem(kCommandReveal, kMenuReveal, false, "", IconId::FolderOpen));
  }
  MenuItem submenu = menuItem("new", kMenuNewImage, model_.blankKinds().empty(), "", IconId::FolderPlus, isFile);
  for (const contracts::DiskBlankKind& kind : model_.blankKinds()) submenu.items.push_back(menuItem((std::string(kCommandNewPrefix) + kind.id).c_str(), kind.title.c_str()));
  items.push_back(std::move(submenu));
  items.push_back(menuItem(kCommandNewFolder, kMenuNewFolder, false, "", IconId::FolderPlus));
  if (node != nullptr) {
    items.push_back(menuItem(kCommandCut, kMenuCut, false, "Ctrl+X", IconId::Cut, true));
    items.push_back(menuItem(kCommandCopy, kMenuCopy, false, "Ctrl+C", IconId::Copy));
  }
  items.push_back(menuItem(kCommandPaste, kMenuPaste, !model_.hasItemClipboard(), "Ctrl+V", IconId::Clipboard, node == nullptr));
  if (node != nullptr) {
    items.push_back(menuItem(kCommandRename, kMenuRename, false, "F2", IconId::Pencil, true));
    items.push_back(menuItem(kCommandDelete, kMenuDelete, false, "Delete", IconId::Trash));
  }
  const std::weak_ptr<bool> alive = alive_;
  menu_->open(position, std::move(items), [this, alive, id, folder](const std::string& command) {
    if (alive.expired() || !*alive.lock()) return;
    if (command == kCommandLoad) model_.loadIntoDrive(id);
    else if (command == kCommandReveal) model_.reveal(id);
    else if (command.rfind(kCommandNewPrefix, 0) == 0) model_.createImage(folder, command.substr(std::string(kCommandNewPrefix).size()));
    else if (command == kCommandNewFolder) newTreeFolder(folder);
    else if (command == kCommandCut) model_.cutItems({id});
    else if (command == kCommandCopy) model_.copyItems({id});
    else if (command == kCommandPaste) model_.pasteItems(folder);
    else if (command == kCommandRename) tree_->beginRename(id);
    else if (command == kCommandDelete) model_.removeItems({id});
  });
}

std::string DiskImagesContent::folderFor(const std::string& id) const {
  const contracts::DiskTreeNode* node = findNode(id);
  if (node == nullptr) return {};
  if (node->directory) return node->path;
  const std::size_t slash = node->path.find_last_of('/');
  return slash == std::string::npos ? std::string() : node->path.substr(0, slash);
}

void DiskImagesContent::newTreeFolder(const std::string& parent) {
  const std::weak_ptr<bool> alive = alive_;
  model_.makeItemFolder(parent, [this, alive](const std::string& path) {
    if (alive.expired() || !*alive.lock()) return;
    pendingTreeRename_ = path;
  });
}

bool DiskImagesContent::canDropOnFolder(const std::string& id, const std::string& target) const {
  // 放回它原来所在的文件夹、放进它自己或它里面都不行
  if (target == id || folderFor(id) == target) return false;
  return target.size() <= id.size() || target.compare(0, id.size(), id) != 0 || target[id.size()] != '/';
}

bool DiskImagesContent::handleTreeKey(const KeyEvent& event) {
  if ((event.modifiers & kModifierAlt) != 0) return false;
  const bool ctrl = (event.modifiers & kModifierCtrl) != 0;
  const bool shift = (event.modifiers & kModifierShift) != 0;
  const std::string id = tree_->focusedId();
  switch (event.virtualKey) {
    case kKeyF2:
      if (ctrl || shift || id.empty()) return false;
      tree_->beginRename(id);
      return true;
    case kKeyDelete:
      if (id.empty()) return false;
      model_.removeItems({id});
      return true;
    case kKeyX:
      if (!ctrl || shift || id.empty()) return false;
      model_.cutItems({id});
      return true;
    case kKeyC:
      if (!ctrl || shift || id.empty()) return false;
      model_.copyItems({id});
      return true;
    case kKeyV:
      if (!ctrl || shift || !model_.hasItemClipboard()) return false;
      model_.pasteItems(folderFor(id));
      return true;
    default:
      return false;
  }
}

void DiskImagesContent::showListMenu(const std::vector<std::string>& ids, Point position, bool highlightFirst) {
  if (!imageShown() || imageView_.failed || imageView_.loading) return;
  std::vector<std::string> names;
  for (const std::string& id : ids) {
    if (id != kParentRowId) names.push_back(id);
  }
  const bool modify = imageView_.canModify();
  const bool selection = !names.empty();
  std::vector<MenuItem> items;
  if (selection) {
    const bool folder = names.size() == 1 && std::any_of(imageView_.entries.begin(), imageView_.entries.end(), [&names](const contracts::DiskEntryRow& row) { return row.name == names.front() && row.directory; });
    if (folder) items.push_back(menuItem(kCommandOpen, kMenuOpen, false, "Enter"));
    items.push_back(menuItem(kCommandRename, kMenuRename, !modify, "F2", IconId::Pencil, folder));
    items.push_back(menuItem(kCommandCut, kMenuCut, !modify, "Ctrl+X", IconId::Cut, true));
    items.push_back(menuItem(kCommandCopy, kMenuCopy, false, "Ctrl+C", IconId::Copy));
    items.push_back(menuItem(kCommandPaste, kMenuPaste, !modify || !model_.hasClipboard(), "Ctrl+V", IconId::Clipboard));
    items.push_back(menuItem(kCommandDelete, kMenuDelete, !modify, "Delete", IconId::Trash, true));
    items.push_back(menuItem(kCommandNewFolder, kMenuNewFolder, !modify || !imageView_.subdirectories, "", IconId::FolderPlus, true));
  } else {
    items.push_back(menuItem(kCommandPaste, kMenuPaste, !modify || !model_.hasClipboard(), "Ctrl+V", IconId::Clipboard));
    items.push_back(menuItem(kCommandNewFolder, kMenuNewFolder, !modify || !imageView_.subdirectories, "", IconId::FolderPlus, true));
  }
  items.push_back(menuItem(kCommandUndo, kMenuUndo, !modify || imageView_.undoDepth == 0, "Ctrl+Z", IconId::ArrowBackUp));
  const std::weak_ptr<bool> alive = alive_;
  menu_->open(position, std::move(items), [this, alive](const std::string& command) {
    if (alive.expired() || !*alive.lock()) return;
    runListCommand(command);
  }, highlightFirst);
}

void DiskImagesContent::runListCommand(const std::string& command) {
  if (command == kCommandOpen) commandOpen();
  else if (command == kCommandRename) commandRename();
  else if (command == kCommandCut) commandCut();
  else if (command == kCommandCopy) commandCopy();
  else if (command == kCommandPaste) commandPaste();
  else if (command == kCommandDelete) commandDelete();
  else if (command == kCommandNewFolder) commandNewFolder();
  else if (command == kCommandUndo) commandUndo();
}

// ---------------------------------------------------------------- 键盘与命令

bool DiskImagesContent::handleListKey(const KeyEvent& event) {
  if ((event.modifiers & kModifierAlt) != 0) return false;
  const bool ctrl = (event.modifiers & kModifierCtrl) != 0;
  const bool shift = (event.modifiers & kModifierShift) != 0;
  switch (event.virtualKey) {
    case kKeyF2:
      if (ctrl || shift) return false;
      commandRename();
      return true;
    case kKeyDelete:
      commandDelete();
      return true;
    case kKeyBack:
      if (ctrl || shift) return false;
      if (!imageView_.directory.empty()) model_.goUp();
      return true;
    case kKeyX:
      if (!ctrl || shift) return false;
      commandCut();
      return true;
    case kKeyC:
      if (!ctrl || shift) return false;
      commandCopy();
      return true;
    case kKeyV:
      if (!ctrl || shift) return false;
      commandPaste();
      return true;
    case kKeyZ:
      if (!ctrl || shift) return false;
      commandUndo();
      return true;
    default:
      return false;
  }
}

bool DiskImagesContent::onKeyDown(const KeyEvent& event) {
  // 焦点在树上时 Ctrl+Z 也撤销当前镜像里的操作
  if (event.virtualKey == kKeyZ && (event.modifiers & kModifierCtrl) != 0 && (event.modifiers & (kModifierShift | kModifierAlt)) == 0) {
    commandUndo();
    return true;
  }
  return false;
}

std::vector<std::string> DiskImagesContent::selectedNames() const {
  std::vector<std::string> names;
  for (const std::string& id : table_->selectedIds()) {
    if (id != kParentRowId) names.push_back(id);
  }
  return names;
}

Rect DiskImagesContent::focusAnchor() const {
  const Rect row = table_->rowRect(table_->focusId());
  if (row.width > 0) return row;
  const Rect area = listScroll_->bounds();
  return Rect{area.x + kNoticeMargin, area.y + kNoticeMargin, area.width / 2.0, 22.0};
}

void DiskImagesContent::showHint(const Rect& anchor, const std::string& text) {
  if (host() == nullptr) return;
  if (hint_ == nullptr) hint_ = std::make_unique<HintBubble>(*host());
  hint_->show(anchor, text);
}

bool DiskImagesContent::requireModify(const Rect& anchor) {
  if (imageView_.canModify()) return true;
  if (imageShown() && !imageView_.failed && !imageView_.loading) showHint(anchor, imageView_.readOnlyReason.empty() ? std::string(kHintReadOnly) : imageView_.readOnlyReason);
  return false;
}

void DiskImagesContent::commandOpen() {
  const std::vector<std::string> names = selectedNames();
  if (names.size() == 1) model_.openFolder(names.front());
}

void DiskImagesContent::commandRename() {
  const std::vector<std::string> names = selectedNames();
  if (names.empty() || !requireModify(focusAnchor())) return;
  const std::string focus = table_->focusId();
  table_->beginRename(std::find(names.begin(), names.end(), focus) != names.end() ? focus : names.front());
}

void DiskImagesContent::commandDelete() {
  const std::vector<std::string> names = selectedNames();
  if (names.empty() || !requireModify(focusAnchor())) return;
  model_.remove(names);
}

void DiskImagesContent::commandCut() {
  const std::vector<std::string> names = selectedNames();
  if (names.empty() || !requireModify(focusAnchor())) return;
  model_.cut(names);
}

void DiskImagesContent::commandCopy() {
  const std::vector<std::string> names = selectedNames();
  if (!names.empty()) model_.copy(names);
}

void DiskImagesContent::commandPaste() {
  if (!imageShown() || !model_.hasClipboard() || !requireModify(focusAnchor())) return;
  model_.paste();
}

void DiskImagesContent::commandNewFolder() {
  if (!imageShown() || !requireModify(focusAnchor())) return;
  if (!imageView_.subdirectories) {
    showHint(focusAnchor(), imageView_.formatName + kHintNoFolders);
    return;
  }
  const std::weak_ptr<bool> alive = alive_;
  model_.makeFolder([this, alive](const std::string& name) {
    if (alive.expired() || !*alive.lock()) return;
    pendingRename_ = name;
    tryStartRename();
  });
}

void DiskImagesContent::commandUndo() {
  if (!imageShown()) return;
  if (imageView_.undoDepth == 0) {
    showHint(focusAnchor(), kHintNothingToUndo);
    return;
  }
  if (!requireModify(focusAnchor())) return;
  model_.undo();
}

// ---------------------------------------------------------------- 拖放

void DiskImagesContent::startDrag(const std::vector<std::string>& ids) {
  std::vector<std::string> names;
  for (const std::string& id : ids) {
    if (id != kParentRowId) names.push_back(id);
  }
  if (names.empty() || !imageShown()) return;
  ActiveDrag drag{++nextToken_, imageView_.image, imageView_.directory, names};
  const std::weak_ptr<bool> alive = alive_;
  model_.exportEntries(names, [this, alive, drag](std::vector<contracts::DiskExportItem> items) {
    if (alive.expired() || !*alive.lock() || items.empty()) return;
    activeDrag_ = drag;
    dragHost_.startDrag(items, drag.token);
    activeDrag_.reset();
    dragLeave();
  });
}

std::optional<DiskImagesContent::DropTarget> DiskImagesContent::dropTarget(Point point, bool internal) {
  if (!imagePaneShown_ || !listScroll_->bounds().contains(point) || !imageShown() || imageView_.failed || imageView_.loading) return std::nullopt;
  const std::optional<std::string> row = table_->rowIdAt(point);
  std::string folder;
  if (row && *row != kParentRowId) {
    const auto found = std::find_if(imageView_.entries.begin(), imageView_.entries.end(), [&row](const contracts::DiskEntryRow& entry) { return entry.name == *row; });
    if (found != imageView_.entries.end() && found->directory) folder = found->name;
  }
  // 本程序拖出来的条目只能放到文件夹上（放在别处还是原来的文件夹，不用动）
  if (internal && folder.empty()) return std::nullopt;
  // 拖的条目自己是这个文件夹：不能放进自己里面
  if (internal && activeDrag_ && std::find(activeDrag_->names.begin(), activeDrag_->names.end(), folder) != activeDrag_->names.end()) return std::nullopt;
  return DropTarget{folder};
}

bool DiskImagesContent::dragOver(bool internal, Point point) {
  const std::optional<DropTarget> target = dropTarget(point, internal);
  if (!target) {
    table_->setDropHighlight("");
    return false;
  }
  if (!imageView_.canModify()) {
    table_->setDropHighlight("");
    // 这次拖放只提示一次，别一直重新计时
    if (!dropHintShown_) {
      dropHintShown_ = true;
      showHint(Rect{point.x, point.y, 1.0, 1.0}, imageView_.readOnlyReason.empty() ? std::string(kHintReadOnly) : imageView_.readOnlyReason);
    }
    return false;
  }
  table_->setDropHighlight(target->folder);
  return true;
}

void DiskImagesContent::dragLeave() {
  table_->setDropHighlight("");
  dropHintShown_ = false;
}

void DiskImagesContent::dropFiles(Point point, const std::vector<std::string>& paths) {
  const std::optional<DropTarget> target = dropTarget(point, false);
  dragLeave();
  if (!target || !imageView_.canModify() || paths.empty()) return;
  model_.importPaths(paths, target->folder);
}

void DiskImagesContent::dropInternal(Point point) {
  const std::optional<DropTarget> target = dropTarget(point, true);
  const std::optional<ActiveDrag> drag = activeDrag_;
  dragLeave();
  if (!target || !drag || !imageView_.canModify() || drag->image != imageView_.image || drag->directory != imageView_.directory) return;
  // 没有子目录的文件系统里没有文件夹可放，走不到这里；有的话立即移动
  model_.moveInto(drag->names, target->folder);
}

}  // namespace ember::win32::ui
