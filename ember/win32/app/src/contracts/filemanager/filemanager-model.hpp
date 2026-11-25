/*
 * ------------------------------------------------------------------------
 * 名称：filemanager-model.hpp
 * 说明：媒体库给界面层的模型：左边目录树的内容、右边所选镜像里当前文件夹的内容，和对镜像的操作（重命名、删除、移动、复制、粘贴、新建文件夹、撤销、导入、导出、新建空白镜像、载入软驱、在资源管理器中显示）。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-25
 * 备注：数据来自核心（filemanager.*），文字由逻辑层算好，界面只显示；操作立即生效，只有删除与移动由控制器先弹确认；
 *       操作的结果与错误用外壳通知给出，模型不显示结果；某个文件系统不支持的操作由界面按 canModify、subdirectories 判断并给出提示，不去调用；规则见 docs/05-界面.md 第 4.8 节。
 * ------------------------------------------------------------------------
 */

#pragma once

#include <cstdint>
#include <functional>
#include <string>
#include <vector>

namespace ember::win32::contracts {

/** 目录树的读取状态。 */
enum class FileManagerState {
  /** 正在读取（核心还没有就绪或核心正在重启）。 */
  Loading,
  /** 还没有选过目录。 */
  NoRoot,
  /** 目录不存在或读不了。 */
  Unavailable,
  Ready,
  /** 读取失败，原因在 message 里。 */
  Failed
};

/** 目录树节点的种类（与核心 filemanager.tree 的 kind 一致）。 */
inline constexpr const char* kKindFloppy = "floppy";
inline constexpr const char* kKindCartridge = "cartridge";
inline constexpr const char* kKindAudio = "audio";
inline constexpr const char* kKindPicture = "picture";
inline constexpr const char* kKindUnsupported = "unsupported";

/** 目录树里的一个节点。 */
struct DiskTreeNode {
  /** 相对目录的路径，用 / 分隔，也是树里的标识。 */
  std::string path;
  /** 完整的文件名或文件夹名。 */
  std::string name;
  bool directory{false};
  /** 种类：folder、floppy（软盘镜像）、cartridge（nes 卡带）、audio（音频）、picture（png 图片）。 */
  std::string kind;
  /** 文件大小（字节）；文件夹为 0。 */
  std::uint64_t size{0};
  bool operator==(const DiskTreeNode&) const = default;
};

/** 目录树此刻要显示的内容：已经读出来的文件夹（目录本身与展开过的文件夹）里的节点。 */
struct DiskTreeView {
  FileManagerState state{FileManagerState::Loading};
  std::string message;
  /** 目录的完整路径，用来显示。 */
  std::string root;
  /** 节点：先序（文件夹后面紧跟它里面的节点），同级文件夹在前；没读出来的文件夹里面没有节点。 */
  std::vector<DiskTreeNode> nodes;
  bool operator==(const DiskTreeView&) const = default;
};

/** 镜像里一个文件或文件夹。 */
struct DiskEntryRow {
  std::string name;
  bool directory{false};
  /** 类型文字：FAT 是扩展名，DOS 3.3 与 ProDOS 是类型名，文件夹是“文件夹”。 */
  std::string type;
  /** 大小文字；文件夹为空。 */
  std::string size;
  /** 修改时间文字；文件系统没有时间时为空。 */
  std::string modified;
  /** 只读或锁定。 */
  bool locked{false};
  bool operator==(const DiskEntryRow&) const = default;
};

/** 所选文件此刻要显示的内容：软盘镜像是它当前文件夹里的文件，其他文件只有它自己一行。 */
struct DiskImageView {
  /** 是否选中了文件。 */
  bool selected{false};
  /** 选中文件的种类，取值同 DiskTreeNode::kind（没选中时为空）。 */
  std::string kind;
  /** 文件的相对路径与文件名。 */
  std::string image;
  std::string imageName;
  /** 当前文件夹：从根目录逐级的名称，空是根目录。 */
  std::vector<std::string> directory;
  /** 正在读取。 */
  bool loading{false};
  /** 读取失败（比如不认识格式），原因在 message 里；失败时没有 entries。 */
  bool failed{false};
  std::string message;
  /** 文件系统的格式标识与显示名称。 */
  std::string format;
  std::string formatName;
  /** 有没有子目录：没有时（DOS 3.3）不能新建文件夹，也不能移动文件。 */
  bool subdirectories{false};
  /** 文件系统能不能修改（.nib、.woz 只能看）。 */
  bool writable{false};
  /** 此刻不能修改的原因（位流格式、软驱里的盘片写保护或被取出）；可以修改时为空。 */
  std::string readOnlyReason;
  /** 镜像插在哪个软驱里（编号从 0 起）；没有插入为 -1。 */
  int drive{-1};
  /** 能撤销几步。 */
  int undoDepth{0};
  std::vector<DiskEntryRow> entries;

  /** 此刻能不能改镜像里的内容。 */
  [[nodiscard]] bool canModify() const { return selected && !failed && !loading && writable && readOnlyReason.empty(); }
  bool operator==(const DiskImageView&) const = default;
};

/** 能新建的一种空白镜像。 */
struct DiskBlankKind {
  std::string id;
  std::string title;
};

/** 拖出到 Windows 的一个文件或文件夹。 */
struct DiskExportItem {
  /** 相对路径，用 / 分隔，第一级是拖出的条目的 Windows 文件名。 */
  std::string path;
  bool directory{false};
  std::vector<std::uint8_t> content;
};

/** 媒体库的模型。 */
class FileManagerModel {
 public:
  virtual ~FileManagerModel() = default;

  [[nodiscard]] virtual DiskTreeView tree() const = 0;
  [[nodiscard]] virtual DiskImageView image() const = 0;
  /** 能新建的空白镜像种类，按显示顺序；还没读到时为空。 */
  [[nodiscard]] virtual std::vector<DiskBlankKind> blankKinds() const = 0;
  /** 剪贴板里有没有剪切或复制的条目。 */
  [[nodiscard]] virtual bool hasClipboard() const = 0;
  /** 当前镜像当前文件夹里被剪切、等着粘贴的条目名称（列表里弱化显示）。 */
  [[nodiscard]] virtual std::vector<std::string> cutNames() const = 0;

  /** 选目录：弹出系统的选择文件夹窗口，用户取消时什么都不发生。 */
  virtual void chooseRoot() = 0;
  /** 树里展开了一个文件夹：还没读过它里面的内容就去读（读完后节点才会出现在树里）。 */
  virtual void expandFolder(const std::string& path) = 0;
  /** 选中一个镜像（路径不是镜像时什么都不做）；开始读取它的内容。 */
  virtual void selectImage(const std::string& path) = 0;
  /** 进入当前文件夹里的一个文件夹。 */
  virtual void openFolder(const std::string& name) = 0;
  /** 回到上一级文件夹。 */
  virtual void goUp() = 0;

  /** 重命名当前文件夹里的条目。 */
  virtual void rename(const std::string& name, const std::string& newName) = 0;
  /** 删除当前文件夹里的条目（先确认）。 */
  virtual void remove(const std::vector<std::string>& names) = 0;
  /** 把当前文件夹里的条目移进当前文件夹里的一个文件夹。 */
  virtual void moveInto(const std::vector<std::string>& names, const std::string& folder) = 0;
  /** 在当前文件夹里新建文件夹，名称自动取不重名的；新建成功后用 onCreated 告诉界面它的名称（好接着重命名）。 */
  virtual void makeFolder(std::function<void(const std::string& name)> onCreated) = 0;
  /** 撤销这个镜像里最近一次管理操作。 */
  virtual void undo() = 0;

  /** 记下要剪切或复制的条目（当前文件夹里的）。 */
  virtual void cut(const std::vector<std::string>& names) = 0;
  virtual void copy(const std::vector<std::string>& names) = 0;
  /** 把剪贴板里的条目放进当前文件夹（可以来自另一个镜像）。 */
  virtual void paste() = 0;

  /** 把拖入窗口的文件与文件夹（Windows 路径）导入当前文件夹里的 folder（为空是当前文件夹）。 */
  virtual void importPaths(const std::vector<std::string>& paths, const std::string& folder) = 0;
  /** 读出当前文件夹里条目的内容用来拖出；读好后在界面线程调用 done，失败时用通知提示并以空列表调用。 */
  virtual void exportEntries(const std::vector<std::string>& names, std::function<void(std::vector<DiskExportItem> items)> done) = 0;

  /** 在目录里的文件夹（相对路径，空串是目录本身）新建一个空白镜像并选中它。 */
  virtual void createImage(const std::string& directory, const std::string& kind) = 0;
  /** 把镜像载入软驱 A。 */
  virtual void loadIntoDrive(const std::string& path) = 0;
  /** 在系统的文件管理器里显示文件。 */
  virtual void reveal(const std::string& path) = 0;
  /** 把 nes 卡带加载到模拟器并开机进入游戏。 */
  virtual void runCartridge(const std::string& path) = 0;
  /** 把音频文件作为磁带载入录音机。 */
  virtual void loadTape(const std::string& path) = 0;
  /** 用系统默认的程序打开文件（预览 png 图片）。 */
  virtual void openFile(const std::string& path) = 0;
  // ---- 目录里的文件与文件夹（左边的树）：作用在磁盘上的真实文件，立即生效，删除是移进回收站 ----
  /** 重命名文件或文件夹（path 是相对目录的路径）；选中的镜像跟着它走。 */
  virtual void renameItem(const std::string& path, const std::string& newName) = 0;
  /** 删除（移进回收站）；选中的镜像被删掉时取消选中。 */
  virtual void removeItems(const std::vector<std::string>& paths) = 0;
  /** 移进另一个文件夹（空串是目录本身）。 */
  virtual void moveItems(const std::vector<std::string>& paths, const std::string& targetFolder) = 0;
  /** 在文件夹（空串是目录本身）里新建文件夹，名称自动取不重名的；成功后用 onCreated 告诉界面它的路径（好接着重命名）。 */
  virtual void makeItemFolder(const std::string& parent, std::function<void(const std::string& path)> onCreated) = 0;
  /** 记下要剪切或复制的文件与文件夹。 */
  virtual void cutItems(const std::vector<std::string>& paths) = 0;
  virtual void copyItems(const std::vector<std::string>& paths) = 0;
  /** 把记下的文件与文件夹放进文件夹（空串是目录本身）：剪切的移动，复制的复制。 */
  virtual void pasteItems(const std::string& targetFolder) = 0;
  /** 有没有剪切或复制的文件与文件夹。 */
  [[nodiscard]] virtual bool hasItemClipboard() const = 0;
  /** 目录里被剪切、等着粘贴的文件与文件夹的路径（树里弱化显示）。 */
  [[nodiscard]] virtual std::vector<std::string> cutItemPaths() const = 0;
  /** 订阅内容变化；返回取消订阅的函数。 */
  [[nodiscard]] virtual std::function<void()> subscribe(std::function<void()> handler) = 0;
};

/** 把条目拖到 Windows 里的能力（系统的拖放由应用层完成，界面不直接碰）。 */
class FileManagerDragHost {
 public:
  virtual ~FileManagerDragHost() = default;

  /**
   * 开始一次拖放，到拖放结束才返回。
   * @param items 拖到别的程序里的内容；为空时只有窗口内部的拖动。
   * @param token 窗口内部拖动的标记，不为 0。
   */
  virtual void startDrag(const std::vector<DiskExportItem>& items, std::uint64_t token) = 0;
};

}  // namespace ember::win32::contracts
