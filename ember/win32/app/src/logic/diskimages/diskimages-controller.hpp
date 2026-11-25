/*
 * ------------------------------------------------------------------------
 * 名称：diskimages-controller.hpp
 * 说明：磁盘镜像管理对话框的控制器：核心就绪后把保存的镜像目录交给核心并读取目录树，订阅 diskimages.treeChanged、diskimages.imageChanged 与 media.changed 保持最新；
 *       记住选中的镜像与当前文件夹，执行对镜像里文件的各种操作，并保存剪切与复制的条目。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-25
 * 备注：操作的结果与错误都用外壳通知给出，对话框里不显示结果；操作由核心立即生效，核心发 diskimages.imageChanged 后这里重新读取；规则见 docs/05-界面.md 第 4.8 节。
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
#include "contracts/diskimages/diskimages-model.hpp"
#include "contracts/shell/dropped-files.hpp"
#include "contracts/shell/request-broker.hpp"
#include "contracts/shell/shell-registry.hpp"

namespace ember::win32::logic {

/** 磁盘镜像管理的控制器。 */
class DiskImagesController final : public contracts::DiskImagesModel {
 public:
  /**
   * @param broker 请求中转：核心的请求与外壳自己处理的请求（选择镜像目录、导入、载入软驱、显示文件）都经它。
   * @param core 核心网关：订阅核心事件与核心运行状态。
   * @param dropped 拖入文件服务：导入时登记拖入的路径。
   */
  DiskImagesController(contracts::RequestBroker& broker, contracts::CoreGateway& core, contracts::DroppedFilesService& dropped);
  ~DiskImagesController() override;
  DiskImagesController(const DiskImagesController&) = delete;
  DiskImagesController& operator=(const DiskImagesController&) = delete;

  /**
   * 接上外壳：订阅核心事件与核心运行状态，并读取镜像目录树。
   * @returns 断开的函数。
   */
  std::function<void()> attach(contracts::ShellApi& shell);

  // ---- DiskImagesModel ----
  [[nodiscard]] contracts::DiskTreeView tree() const override { return tree_; }
  [[nodiscard]] contracts::DiskImageView image() const override { return image_; }
  [[nodiscard]] std::vector<contracts::DiskBlankKind> blankKinds() const override { return kinds_; }
  [[nodiscard]] bool hasClipboard() const override { return clipboard_.present; }
  void chooseRoot() override;
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
  void loadTree();
  /** 选中的镜像因为重命名或移动换了路径时，选中项跟着走（from 是旧路径，可以是它所在的文件夹）。 */
  void followSelection(const std::string& from, const std::string& to);
  /** 选中的镜像被删除（它本身或它所在的文件夹）时取消选中。 */
  void dropSelectionUnder(const std::vector<std::string>& paths);
  void loadKinds();
  void loadImage();
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
  contracts::ShellApi* shell_{nullptr};
  std::vector<std::function<void()>> stops_;
  std::shared_ptr<bool> alive_{std::make_shared<bool>(true)};
  std::uint64_t generation_{0};
  std::uint64_t treeRequest_{0};
  std::uint64_t listRequest_{0};

  contracts::DiskTreeView tree_;
  contracts::DiskImageView image_;
  std::vector<contracts::DiskBlankKind> kinds_;
  Clipboard clipboard_;
  /** 镜像目录里剪切或复制的文件与文件夹。 */
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
