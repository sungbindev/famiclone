/*
 * ------------------------------------------------------------------------
 * 名称：filemanager-controller.hpp
 * 说明：媒体库对话框的控制器：核心就绪后把保存的目录交给核心，按文件夹读取目录树（目录本身与展开过的文件夹），订阅 filemanager.treeChanged、filemanager.imageChanged 与 media.changed 保持最新；
 *       记住选中的文件（磁盘镜像还有当前文件夹），执行对镜像里文件的各种操作，选中卡带、音频、图片时右边只有它自己一行，并保存剪切与复制的条目。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-25
 * 备注：操作的结果与错误都用外壳通知给出，对话框里不显示结果；操作由核心立即生效，核心发 filemanager.imageChanged 后这里重新读取；规则见 docs/05-界面.md 第 4.8 节。
 * ------------------------------------------------------------------------
 */

#pragma once

#include <cstdint>
#include <functional>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include "contracts/core-gateway/core-gateway.hpp"
#include "contracts/filemanager/filemanager-model.hpp"
#include "contracts/shell/confirmation.hpp"
#include "contracts/shell/dropped-files.hpp"
#include "contracts/shell/request-broker.hpp"
#include "contracts/shell/shell-registry.hpp"

namespace ember::win32::logic {

/** 媒体库的控制器。 */
class FileManagerController final : public contracts::FileManagerModel {
 public:
  /**
   * @param broker 请求中转：核心的请求与外壳自己处理的请求（选择目录、导入、载入软驱、显示文件）都经它。
   * @param core 核心网关：订阅核心事件与核心运行状态。
   * @param dropped 拖入文件服务：导入时登记拖入的路径。
   * @param confirmation 删除前的确认询问：镜像里的文件与目录里的文件、文件夹的删除都要确认。
   */
  FileManagerController(contracts::RequestBroker& broker, contracts::CoreGateway& core, contracts::DroppedFilesService& dropped, contracts::Confirmation& confirmation);
  ~FileManagerController() override;
  FileManagerController(const FileManagerController&) = delete;
  FileManagerController& operator=(const FileManagerController&) = delete;

  /**
   * 接上外壳：订阅核心事件与核心运行状态，并读取目录树。
   * @returns 断开的函数。
   */
  std::function<void()> attach(contracts::ShellApi& shell);

  // ---- FileManagerModel ----
  [[nodiscard]] contracts::DiskTreeView tree() const override { return tree_; }
  [[nodiscard]] contracts::DiskImageView image() const override { return image_; }
  [[nodiscard]] std::vector<contracts::DiskBlankKind> blankKinds() const override { return kinds_; }
  [[nodiscard]] bool hasClipboard() const override { return clipboard_.present; }
  [[nodiscard]] std::vector<std::string> cutNames() const override;
  [[nodiscard]] std::vector<std::string> cutItemPaths() const override { return itemClipboard_.present && itemClipboard_.cut ? itemClipboard_.paths : std::vector<std::string>{}; }
  void chooseRoot() override;
  void expandFolder(const std::string& path) override;
  void selectImage(const std::string& path) override;
  void openFolder(const std::string& name) override;
  void goUp() override;
  void rename(const std::string& name, const std::string& newName) override;
  void remove(const std::vector<std::string>& names) override;
  void moveInto(const std::vector<std::string>& names, const std::string& folder) override;
  void makeFolder(std::function<void(const std::string& name)> onCreated) override;
  void undo() override;
  void cut(const std::vector<std::string>& names) override;
  void copy(const std::vector<std::string>& names) override;
  void paste() override;
  void importPaths(const std::vector<std::string>& paths, const std::string& folder) override;
  void exportEntries(const std::vector<std::string>& names, std::function<void(std::vector<contracts::DiskExportItem> items)> done) override;
  void createImage(const std::string& directory, const std::string& kind) override;
  void loadIntoDrive(const std::string& path) override;
  void reveal(const std::string& path) override;
  void runCartridge(const std::string& path) override;
  void loadTape(const std::string& path) override;
  void openFile(const std::string& path) override;
  void renameItem(const std::string& path, const std::string& newName) override;
  void removeItems(const std::vector<std::string>& paths) override;
  void moveItems(const std::vector<std::string>& paths, const std::string& targetFolder) override;
  void makeItemFolder(const std::string& parent, std::function<void(const std::string& path)> onCreated) override;
  void cutItems(const std::vector<std::string>& paths) override;
  void copyItems(const std::vector<std::string>& paths) override;
  void pasteItems(const std::string& targetFolder) override;
  [[nodiscard]] bool hasItemClipboard() const override { return itemClipboard_.present; }
  [[nodiscard]] std::function<void()> subscribe(std::function<void()> handler) override;

 private:
  /** 剪贴板：剪切或复制的条目。 */
  struct Clipboard {
    bool present{false};
    bool cut{false};
    std::string image;
    std::vector<std::vector<std::string>> entries;
  };

  void start();
  /** 重新读取目录本身和已经读出来的每个文件夹（监视发现变化、更换目录时）。 */
  void reloadFolders();
  /** 读取一个文件夹里的直接内容（空串是目录本身）。 */
  void loadFolder(const std::string& folder);
  /** 处理一个文件夹的读取结果，重新拼出树。 */
  void applyFolder(const std::string& folder, const contracts::CoreReply& reply);
  /** 让这个文件夹里的内容出现在树里：重新读它，并把它上面还没读过的各级也读出来。 */
  void showFolder(const std::string& folder);
  /** 把已经读出来的文件夹拼成先序的节点列表。 */
  void appendFolder(const std::string& folder, std::vector<contracts::DiskTreeNode>& out) const;
  /** 选中的软盘镜像的概况文字：格式与文件大小。 */
  [[nodiscard]] std::string floppyInfo() const;
  /** 选中的镜像因为重命名或移动换了路径时，选中项跟着走（from 是旧路径，可以是它所在的文件夹）。 */
  void followSelection(const std::string& from, const std::string& to);
  /** 选中的镜像被删除（它本身或它所在的文件夹）时取消选中。 */
  void dropSelectionUnder(const std::vector<std::string>& paths);
  void loadKinds();
  void loadImage();
  /** 选中的不是软盘镜像（卡带、音频、图片）时，用树里的节点填右边这一个文件的那一行。 */
  void fillSingleFile(const contracts::DiskTreeNode& node);
  /** 目录树重新读取后更新选中的非软盘文件（大小变了、被删掉了）。 */
  void syncSingleFile();
  void handleEvent(const std::string& name, const nlohmann::json& data);
  void handleCoreStatus(const contracts::CoreStatus& status);
  void changed();
  void report(const std::string& text);
  void notifySuccess(const std::string& text);
  /** 请求：回复只在仍接着外壳时处理。 */
  void ask(const std::string& name, nlohmann::json params, std::function<void(const contracts::CoreReply&)> done);
  /** 发一个修改类请求：失败时用通知提示，成功时调用 onSuccess。 */
  void operate(const std::string& name, nlohmann::json params, std::function<void(const nlohmann::json& result)> onSuccess = {});
  [[nodiscard]] std::vector<std::string> entryPath(const std::string& name) const;
  [[nodiscard]] std::vector<std::vector<std::string>> entryPaths(const std::vector<std::string>& names) const;
  [[nodiscard]] const contracts::DiskTreeNode* findNode(const std::string& path) const;

  contracts::RequestBroker& broker_;
  contracts::CoreGateway& core_;
  contracts::DroppedFilesService& dropped_;
  contracts::Confirmation& confirmation_;
  contracts::ShellApi* shell_{nullptr};
  std::vector<std::function<void()>> stops_;
  std::shared_ptr<bool> alive_{std::make_shared<bool>(true)};
  std::uint64_t generation_{0};
  /** 目录树的代：换目录、核心重启或断开时加一，旧一代的读取回复作废。 */
  std::uint64_t treeEpoch_{0};
  std::uint64_t listRequest_{0};

  contracts::DiskTreeView tree_;
  /** 已经读出来的文件夹（相对路径，空串是目录本身）里的直接内容。 */
  std::map<std::string, std::vector<contracts::DiskTreeNode>> folders_;
  /** 每个文件夹最近一次读取的编号：后发的回复取代先发的。 */
  std::map<std::string, std::uint64_t> folderRequests_;
  contracts::DiskImageView image_;
  std::vector<contracts::DiskBlankKind> kinds_;
  Clipboard clipboard_;
  /** 目录里剪切或复制的文件与文件夹。 */
  struct ItemClipboard {
    bool present{false};
    bool cut{false};
    std::vector<std::string> paths;
  };
  ItemClipboard itemClipboard_;
  /** 树重新读取后要选中的镜像（刚新建的空白镜像）。 */
  std::string pendingSelect_;
  std::map<int, std::function<void()>> listeners_;
  int nextListener_{1};
};

}  // namespace ember::win32::logic
