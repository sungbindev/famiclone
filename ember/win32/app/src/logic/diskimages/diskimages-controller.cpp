/*
 * ------------------------------------------------------------------------
 * 名称：diskimages-controller.cpp
 * 说明：磁盘镜像管理控制器的实现。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-25
 * 备注：无。
 * ------------------------------------------------------------------------
 */

#include "logic/diskimages/diskimages-controller.hpp"

#include <algorithm>

#include "logic/diskimages/diskimages-rules.hpp"

namespace ember::win32::logic {

namespace {

/** 导入失败时最多逐个用通知说明几个，其余合并成一条。 */
constexpr std::size_t kMaxImportFailureNotices = 3;
/** 软驱 A 的编号。 */
constexpr int kDriveA = 0;

/** 路径最后一级的名称。 */
std::string baseName(const std::string& path) {
  const std::size_t slash = path.find_last_of('/');
  return slash == std::string::npos ? path : path.substr(slash + 1);
}

}  // namespace

DiskImagesController::DiskImagesController(contracts::RequestBroker& broker, contracts::CoreGateway& core, contracts::DroppedFilesService& dropped) : broker_(broker), core_(core), dropped_(dropped) {}

DiskImagesController::~DiskImagesController() {
  *alive_ = false;
  for (const auto& stop : stops_) stop();
}

std::function<void()> DiskImagesController::attach(contracts::ShellApi& shell) {
  const auto stopAll = [this] {
    for (const auto& stop : stops_) stop();
    stops_.clear();
    shell_ = nullptr;
    ++generation_;
    ++treeRequest_;
    ++listRequest_;
  };
  stopAll();
  shell_ = &shell;
  stops_.push_back(core_.onEvent([this](const std::string& name, const nlohmann::json& data) { handleEvent(name, data); }));
  stops_.push_back(core_.onStatusChange([this](const contracts::CoreStatus& status) { handleCoreStatus(status); }));
  start();
  return stopAll;
}

void DiskImagesController::changed() {
  const auto listeners = listeners_;
  for (const auto& [id, listener] : listeners) {
    if (listeners_.find(id) != listeners_.end()) listener();
  }
}

std::function<void()> DiskImagesController::subscribe(std::function<void()> handler) {
  const int id = nextListener_++;
  listeners_[id] = std::move(handler);
  const auto alive = alive_;
  return [this, alive, id] {
    if (*alive) listeners_.erase(id);
  };
}

void DiskImagesController::ask(const std::string& name, nlohmann::json params, std::function<void(const contracts::CoreReply&)> done) {
  const auto alive = alive_;
  const auto generation = generation_;
  broker_.request(name, std::move(params), [this, alive, generation, done = std::move(done)](contracts::CoreReply reply) {
    if (!*alive || generation != generation_ || shell_ == nullptr) return;
    done(reply);
  });
}

void DiskImagesController::operate(const std::string& name, nlohmann::json params, std::function<void(const nlohmann::json& result)> onSuccess) {
  if (shell_ == nullptr) return;
  ask(name, std::move(params), [this, onSuccess = std::move(onSuccess)](const contracts::CoreReply& reply) {
    if (!reply.ok) {
      report(diskErrorText(reply.error.code, reply.error.message));
      return;
    }
    if (onSuccess) onSuccess(reply.result);
  });
}

void DiskImagesController::report(const std::string& text) {
  if (shell_ != nullptr) shell_->notify(text, contracts::NotificationLevel::Error);
}

void DiskImagesController::notifySuccess(const std::string& text) {
  if (shell_ != nullptr) shell_->notify(text, contracts::NotificationLevel::Success);
}

const contracts::DiskTreeNode* DiskImagesController::findNode(const std::string& path) const {
  const auto found = std::find_if(tree_.nodes.begin(), tree_.nodes.end(), [&path](const contracts::DiskTreeNode& node) { return node.path == path; });
  return found == tree_.nodes.end() ? nullptr : &*found;
}

std::vector<std::string> DiskImagesController::entryPath(const std::string& name) const {
  std::vector<std::string> path = image_.directory;
  path.push_back(name);
  return path;
}

std::vector<std::vector<std::string>> DiskImagesController::entryPaths(const std::vector<std::string>& names) const {
  std::vector<std::vector<std::string>> paths;
  for (const std::string& name : names) paths.push_back(entryPath(name));
  return paths;
}

// ---------------------------------------------------------------- 读取

void DiskImagesController::start() {
  if (shell_ == nullptr) return;
  const auto current = ++treeRequest_;
  // 核心重启之后镜像目录要重新交给它；没有保存过目录时 root 为空，树显示“还没有选择镜像目录”
  ask("diskimages.openRoot", nlohmann::json::object(), [this, current](const contracts::CoreReply& reply) {
    if (treeRequest_ != current) return;
    if (!reply.ok) {
      tree_ = contracts::DiskTreeView{};
      tree_.state = contracts::DiskImagesState::Failed;
      tree_.message = diskErrorText(reply.error.code, reply.error.message);
      changed();
      return;
    }
    loadTree();
    loadKinds();
  });
}

void DiskImagesController::loadTree() {
  if (shell_ == nullptr) return;
  const auto current = ++treeRequest_;
  ask("diskimages.tree", nlohmann::json::object(), [this, current](const contracts::CoreReply& reply) {
    if (treeRequest_ != current) return;
    contracts::DiskTreeView view;
    const auto parsed = reply.ok ? parseTree(reply.result) : std::nullopt;
    if (!parsed) {
      view.state = contracts::DiskImagesState::Failed;
      view.message = reply.ok ? "读到的镜像目录内容不合格" : diskErrorText(reply.error.code, reply.error.message);
    } else if (parsed->root.empty()) {
      view.state = contracts::DiskImagesState::NoRoot;
    } else if (!parsed->available) {
      view.state = contracts::DiskImagesState::Unavailable;
      view.root = parsed->root;
    } else {
      view.state = contracts::DiskImagesState::Ready;
      view.root = parsed->root;
      view.limited = parsed->limited;
      view.nodes = parsed->nodes;
    }
    if (view == tree_) return;
    tree_ = std::move(view);
    changed();
    // 刚新建的镜像出现在树里之后选中它
    if (pendingSelect_.empty()) return;
    const std::string path = pendingSelect_;
    const contracts::DiskTreeNode* node = findNode(path);
    if (node == nullptr || node->directory) return;
    pendingSelect_.clear();
    selectImage(path);
  });
}

void DiskImagesController::loadKinds() {
  ask("diskimages.blankKinds", nlohmann::json::object(), [this](const contracts::CoreReply& reply) {
    const auto kinds = reply.ok ? parseBlankKinds(reply.result) : std::nullopt;
    if (!kinds) return;
    kinds_ = *kinds;
    changed();
  });
}

void DiskImagesController::loadImage() {
  if (shell_ == nullptr || !image_.selected) return;
  const auto current = ++listRequest_;
  ask("diskimages.list", {{"image", image_.image}, {"directory", image_.directory}}, [this, current](const contracts::CoreReply& reply) {
    if (listRequest_ != current || !image_.selected) return;
    image_.loading = false;
    if (!reply.ok && reply.error.code == "diskimages.entryNotFound" && !image_.directory.empty()) {
      // 当前文件夹没了（被别处删除或改名）：回到根目录
      image_.directory.clear();
      loadImage();
      return;
    }
    const auto parsed = reply.ok ? parseList(reply.result) : std::nullopt;
    if (!parsed) {
      image_.failed = true;
      image_.message = reply.ok ? "读到的镜像内容不合格" : diskErrorText(reply.error.code, reply.error.message);
      image_.entries.clear();
      image_.format.clear();
      image_.formatName.clear();
      image_.writable = false;
      image_.subdirectories = false;
      image_.readOnlyReason.clear();
      image_.drive = -1;
      image_.undoDepth = 0;
      changed();
      return;
    }
    image_.failed = false;
    image_.message.clear();
    image_.format = parsed->format;
    image_.formatName = parsed->formatName;
    image_.writable = parsed->writable;
    image_.subdirectories = parsed->subdirectories;
    image_.readOnlyReason = parsed->readOnlyReason;
    image_.drive = parsed->drive;
    image_.undoDepth = parsed->undoDepth;
    image_.entries = parsed->entries;
    changed();
  });
}

void DiskImagesController::handleEvent(const std::string& name, const nlohmann::json& data) {
  if (name == "diskimages.treeChanged") {
    loadTree();
  } else if (name == "diskimages.imageChanged") {
    if (image_.selected && data.is_object() && data.value("image", std::string()) == image_.image) loadImage();
  } else if (name == "media.changed") {
    // 镜像插进或退出软驱、盘片写保护变化：读取它的来源与能不能修改
    if (image_.selected) loadImage();
  }
}

void DiskImagesController::handleCoreStatus(const contracts::CoreStatus& status) {
  if (status.state == contracts::CoreState::Running) {
    start();
    return;
  }
  ++treeRequest_;
  ++listRequest_;
  tree_ = contracts::DiskTreeView{};
  image_ = contracts::DiskImageView{};
  kinds_.clear();
  clipboard_ = Clipboard{};
  pendingSelect_.clear();
  changed();
}

// ---------------------------------------------------------------- 选择与浏览

void DiskImagesController::chooseRoot() {
  if (shell_ == nullptr) return;
  ask("diskimages.pickRoot", nlohmann::json::object(), [this](const contracts::CoreReply& reply) {
    if (!reply.ok) {
      report(diskErrorText(reply.error.code, reply.error.message));
      return;
    }
    if (!reply.result.is_object() || reply.result.value("canceled", false)) return;
    // 换了镜像目录，原来选中的镜像与剪贴板都作废
    image_ = contracts::DiskImageView{};
    clipboard_ = Clipboard{};
    loadTree();
  });
}

void DiskImagesController::selectImage(const std::string& path) {
  const contracts::DiskTreeNode* node = findNode(path);
  if (node == nullptr || node->directory) return;
  if (image_.selected && image_.image == path) return;
  image_ = contracts::DiskImageView{};
  image_.selected = true;
  image_.image = path;
  image_.imageName = node->name;
  image_.loading = true;
  changed();
  loadImage();
}

void DiskImagesController::openFolder(const std::string& name) {
  if (!image_.selected) return;
  const auto found = std::find_if(image_.entries.begin(), image_.entries.end(), [&name](const contracts::DiskEntryRow& row) { return row.name == name && row.directory; });
  if (found == image_.entries.end()) return;
  image_.directory.push_back(found->name);
  image_.loading = true;
  changed();
  loadImage();
}

void DiskImagesController::goUp() {
  if (!image_.selected || image_.directory.empty()) return;
  image_.directory.pop_back();
  image_.loading = true;
  changed();
  loadImage();
}

// ---------------------------------------------------------------- 修改

void DiskImagesController::rename(const std::string& name, const std::string& newName) {
  if (!image_.selected) return;
  operate("diskimages.rename", {{"image", image_.image}, {"entry", entryPath(name)}, {"name", newName}});
}

void DiskImagesController::remove(const std::vector<std::string>& names) {
  if (!image_.selected || names.empty()) return;
  operate("diskimages.remove", {{"image", image_.image}, {"entries", entryPaths(names)}});
}

void DiskImagesController::moveInto(const std::vector<std::string>& names, const std::string& folder) {
  if (!image_.selected || names.empty()) return;
  operate("diskimages.move", {{"image", image_.image}, {"entries", entryPaths(names)}, {"target", entryPath(folder)}});
}

void DiskImagesController::makeFolder(std::function<void(const std::string& name)> onCreated) {
  if (!image_.selected) return;
  const std::string name = uniqueFolderName(image_.entries);
  operate("diskimages.makeDirectory", {{"image", image_.image}, {"directory", image_.directory}, {"name", name}}, [onCreated = std::move(onCreated), name](const nlohmann::json&) {
    if (onCreated) onCreated(name);
  });
}

void DiskImagesController::undo() {
  if (!image_.selected) return;
  operate("diskimages.undo", {{"image", image_.image}}, [this](const nlohmann::json& result) {
    const std::string description = result.is_object() ? result.value("description", std::string()) : std::string();
    notifySuccess(description.empty() ? "已撤销" : "已撤销：" + description);
  });
}

void DiskImagesController::cut(const std::vector<std::string>& names) {
  if (!image_.selected || names.empty()) return;
  clipboard_ = Clipboard{true, true, image_.image, entryPaths(names)};
}

void DiskImagesController::copy(const std::vector<std::string>& names) {
  if (!image_.selected || names.empty()) return;
  clipboard_ = Clipboard{true, false, image_.image, entryPaths(names)};
}

void DiskImagesController::paste() {
  if (!image_.selected || !clipboard_.present) return;
  const Clipboard source = clipboard_;
  if (source.cut && source.image == image_.image) {
    // 同一个镜像里剪切再粘贴：移动
    operate("diskimages.move", {{"image", image_.image}, {"entries", source.entries}, {"target", image_.directory}}, [this](const nlohmann::json&) { clipboard_ = Clipboard{}; });
    return;
  }
  operate("diskimages.copy", {{"image", source.image}, {"entries", source.entries}, {"targetImage", image_.image}, {"target", image_.directory}}, [this, source](const nlohmann::json&) {
    if (!source.cut) return;
    // 剪切到另一个镜像：复制成功后再从来源镜像里删掉
    clipboard_ = Clipboard{};
    operate("diskimages.remove", {{"image", source.image}, {"entries", source.entries}});
  });
}

void DiskImagesController::importPaths(const std::vector<std::string>& paths, const std::string& folder) {
  if (!image_.selected || paths.empty()) return;
  const std::vector<contracts::DroppedFile> registered = dropped_.registerFiles(paths);
  if (registered.empty()) {
    report("拖入的文件无法读取");
    return;
  }
  std::vector<std::int64_t> ids;
  for (const contracts::DroppedFile& file : registered) ids.push_back(file.id);
  std::vector<std::string> directory = image_.directory;
  if (!folder.empty()) directory.push_back(folder);
  operate("diskimages.importDropped", {{"image", image_.image}, {"directory", directory}, {"droppedIds", ids}}, [this](const nlohmann::json& result) {
    const auto outcomes = result.is_object() ? result.find("outcomes") : result.end();
    if (!result.is_object() || outcomes == result.end() || !outcomes->is_array()) return;
    std::size_t failures = 0;
    for (const nlohmann::json& outcome : *outcomes) {
      if (!outcome.is_object() || outcome.value("ok", true)) continue;
      ++failures;
      if (failures <= kMaxImportFailureNotices) report(outcome.value("name", std::string()) + "：" + outcome.value("message", std::string("导入失败")));
    }
    if (failures > kMaxImportFailureNotices) report("另有 " + std::to_string(failures - kMaxImportFailureNotices) + " 个文件导入失败");
  });
}

void DiskImagesController::exportEntries(const std::vector<std::string>& names, std::function<void(std::vector<contracts::DiskExportItem> items)> done) {
  if (!image_.selected || names.empty() || shell_ == nullptr) {
    if (done) done({});
    return;
  }
  ask("diskimages.readEntries", {{"image", image_.image}, {"entries", entryPaths(names)}}, [this, done = std::move(done)](const contracts::CoreReply& reply) {
    const auto items = reply.ok ? parseExport(reply.result) : std::nullopt;
    if (!items) {
      report(reply.ok ? "读到的文件内容不合格" : diskErrorText(reply.error.code, reply.error.message));
      if (done) done({});
      return;
    }
    if (done) done(*items);
  });
}

void DiskImagesController::createImage(const std::string& directory, const std::string& kind) {
  operate("diskimages.createImage", {{"directory", directory}, {"kind", kind}}, [this](const nlohmann::json& result) {
    const std::string path = result.is_object() ? result.value("path", std::string()) : std::string();
    if (path.empty()) return;
    notifySuccess("已新建磁盘镜像：" + baseName(path));
    pendingSelect_ = path;
    loadTree();
  });
}

void DiskImagesController::loadIntoDrive(const std::string& path) {
  operate("diskimages.loadImage", {{"image", path}, {"drive", kDriveA}}, [this, path](const nlohmann::json&) { notifySuccess("已载入软驱 A：" + baseName(path)); });
}

void DiskImagesController::reveal(const std::string& path) { operate("diskimages.revealImage", {{"image", path}}); }

}  // namespace ember::win32::logic
