/*
 * ------------------------------------------------------------------------
 * 名称：filemanager-controller.cpp
 * 说明：媒体库控制器的实现。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-25
 * 备注：无。
 * ------------------------------------------------------------------------
 */

#include "logic/filemanager/filemanager-controller.hpp"

#include <algorithm>

#include "logic/filemanager/filemanager-rules.hpp"

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

/** 文件名的扩展名，保留原来的大小写、不带点；没有扩展名时为空。 */
std::string fileExtension(const std::string& name) {
  const std::size_t dot = name.rfind('.');
  return dot == std::string::npos || dot == 0 ? std::string() : name.substr(dot + 1);
}

/** 文件名的扩展名，大写、不带点；没有扩展名时为空。 */
std::string upperExtension(const std::string& name) {
  const std::size_t dot = name.rfind('.');
  if (dot == std::string::npos) return {};
  std::string extension = name.substr(dot + 1);
  for (char& c : extension) {
    if (c >= 'a' && c <= 'z') c = static_cast<char>(c - 'a' + 'A');
  }
  return extension;
}

/** path 是 from 本身或在 from 这个文件夹里面时，把前面的 from 换成 to；否则原样返回。 */
std::string rebase(const std::string& path, const std::string& from, const std::string& to) {
  if (path == from) return to;
  if (path.size() > from.size() && path.compare(0, from.size(), from) == 0 && path[from.size()] == '/') return to + path.substr(from.size());
  return path;
}

/** path 是 folder 本身或在它里面。 */
bool isUnder(const std::string& path, const std::string& folder) { return rebase(path, folder, std::string()) != path; }

/** 新建文件夹的基本名称。 */
constexpr const char* kNewFolderName = "新建文件夹";

/** 删除确认的标题。 */
constexpr const char* kDeleteTitle = "删除";
/** 移动确认的标题。 */
constexpr const char* kMoveTitle = "移动";

/** 目录里的路径列表对应的名称（路径最后一级）。 */
std::vector<std::string> itemNames(const std::vector<std::string>& paths) {
  std::vector<std::string> names;
  for (const std::string& path : paths) names.push_back(baseName(path));
  return names;
}

/** 镜像里逐级路径列表对应的名称（路径最后一级）。 */
std::vector<std::string> entryNames(const std::vector<std::vector<std::string>>& entries) {
  std::vector<std::string> names;
  for (const std::vector<std::string>& entry : entries) {
    if (!entry.empty()) names.push_back(entry.back());
  }
  return names;
}

/** 目录里的目标文件夹的说法：空路径是目录本身。 */
std::string folderPhrase(const std::string& folder) { return folder.empty() ? std::string("目录") : "文件夹“" + baseName(folder) + "”"; }

/** 移动前的确认：把 names 描述的条目移动到 destination（已经写成“文件夹“x””这样的说法）。 */
bool confirmMove(contracts::Confirmation& confirmation, const std::vector<std::string>& names, const std::string& destination) {
  const std::string what = names.size() == 1 ? "“" + names.front() + "”" : "选中的 " + std::to_string(names.size()) + " 项";
  return confirmation.ask(kMoveTitle, "把" + what + "移动到" + destination + "？");
}

}  // namespace

FileManagerController::FileManagerController(contracts::RequestBroker& broker, contracts::CoreGateway& core, contracts::DroppedFilesService& dropped, contracts::Confirmation& confirmation)
    : broker_(broker), core_(core), dropped_(dropped), confirmation_(confirmation) {}

FileManagerController::~FileManagerController() {
  *alive_ = false;
  for (const auto& stop : stops_) stop();
}

std::function<void()> FileManagerController::attach(contracts::ShellApi& shell) {
  const auto stopAll = [this] {
    for (const auto& stop : stops_) stop();
    stops_.clear();
    shell_ = nullptr;
    ++generation_;
    ++treeEpoch_;
    ++listRequest_;
  };
  stopAll();
  shell_ = &shell;
  stops_.push_back(core_.onEvent([this](const std::string& name, const nlohmann::json& data) { handleEvent(name, data); }));
  stops_.push_back(core_.onStatusChange([this](const contracts::CoreStatus& status) { handleCoreStatus(status); }));
  start();
  return stopAll;
}

void FileManagerController::changed() {
  const auto listeners = listeners_;
  for (const auto& [id, listener] : listeners) {
    if (listeners_.find(id) != listeners_.end()) listener();
  }
}

std::function<void()> FileManagerController::subscribe(std::function<void()> handler) {
  const int id = nextListener_++;
  listeners_[id] = std::move(handler);
  const auto alive = alive_;
  return [this, alive, id] {
    if (*alive) listeners_.erase(id);
  };
}

void FileManagerController::ask(const std::string& name, nlohmann::json params, std::function<void(const contracts::CoreReply&)> done) {
  const auto alive = alive_;
  const auto generation = generation_;
  broker_.request(name, std::move(params), [this, alive, generation, done = std::move(done)](contracts::CoreReply reply) {
    if (!*alive || generation != generation_ || shell_ == nullptr) return;
    done(reply);
  });
}

void FileManagerController::operate(const std::string& name, nlohmann::json params, std::function<void(const nlohmann::json& result)> onSuccess) {
  if (shell_ == nullptr) return;
  ask(name, std::move(params), [this, onSuccess = std::move(onSuccess)](const contracts::CoreReply& reply) {
    if (!reply.ok) {
      report(diskErrorText(reply.error.code, reply.error.message));
      return;
    }
    if (onSuccess) onSuccess(reply.result);
  });
}

void FileManagerController::report(const std::string& text) {
  if (shell_ != nullptr) shell_->notify(text, contracts::NotificationLevel::Error);
}

void FileManagerController::notifySuccess(const std::string& text) {
  if (shell_ != nullptr) shell_->notify(text, contracts::NotificationLevel::Success);
}

const contracts::DiskTreeNode* FileManagerController::findNode(const std::string& path) const {
  const auto found = std::find_if(tree_.nodes.begin(), tree_.nodes.end(), [&path](const contracts::DiskTreeNode& node) { return node.path == path; });
  return found == tree_.nodes.end() ? nullptr : &*found;
}

std::vector<std::string> FileManagerController::entryPath(const std::string& name) const {
  std::vector<std::string> path = image_.directory;
  path.push_back(name);
  return path;
}

std::vector<std::vector<std::string>> FileManagerController::entryPaths(const std::vector<std::string>& names) const {
  std::vector<std::vector<std::string>> paths;
  for (const std::string& name : names) paths.push_back(entryPath(name));
  return paths;
}

// ---------------------------------------------------------------- 读取

void FileManagerController::start() {
  if (shell_ == nullptr) return;
  const auto epoch = ++treeEpoch_;
  folders_.clear();
  folderRequests_.clear();
  // 核心重启之后目录要重新交给它；没有保存过目录时 root 为空，树显示“还没有选择目录”
  ask("filemanager.openRoot", nlohmann::json::object(), [this, epoch](const contracts::CoreReply& reply) {
    if (!reply.ok) {
      if (treeEpoch_ != epoch) return;
      tree_ = contracts::DiskTreeView{};
      tree_.state = contracts::FileManagerState::Failed;
      tree_.message = diskErrorText(reply.error.code, reply.error.message);
      changed();
      return;
    }
    // 核心设置目录时会发 treeChanged，目录本身可能已经被它读过：此时不必再读，但空白镜像的种类还要读
    loadKinds();
    if (treeEpoch_ == epoch && folderRequests_.find(std::string()) == folderRequests_.end()) loadFolder(std::string());
  });
}

void FileManagerController::reloadFolders() {
  if (shell_ == nullptr) return;
  std::vector<std::string> folders{std::string()};
  for (const auto& [folder, nodes] : folders_) {
    if (!folder.empty()) folders.push_back(folder);
  }
  for (const std::string& folder : folders) loadFolder(folder);
}

void FileManagerController::loadFolder(const std::string& folder) {
  if (shell_ == nullptr) return;
  const auto epoch = treeEpoch_;
  const auto current = ++folderRequests_[folder];
  ask("filemanager.tree", {{"directory", folder}}, [this, folder, epoch, current](const contracts::CoreReply& reply) {
    if (treeEpoch_ != epoch || folderRequests_[folder] != current) return;
    applyFolder(folder, reply);
  });
}

void FileManagerController::applyFolder(const std::string& folder, const contracts::CoreReply& reply) {
  const auto parsed = reply.ok ? parseTree(reply.result) : std::nullopt;
  if (folder.empty()) {
    contracts::DiskTreeView view;
    if (!parsed) {
      view.state = contracts::FileManagerState::Failed;
      view.message = reply.ok ? "读到的目录内容不合格" : diskErrorText(reply.error.code, reply.error.message);
      folders_.clear();
    } else if (parsed->root.empty()) {
      view.state = contracts::FileManagerState::NoRoot;
      folders_.clear();
    } else if (!parsed->available) {
      view.state = contracts::FileManagerState::Unavailable;
      view.root = parsed->root;
      folders_.clear();
    } else {
      view.state = contracts::FileManagerState::Ready;
      view.root = parsed->root;
      folders_[folder] = parsed->nodes;
      appendFolder(folder, view.nodes);
    }
    if (view == tree_) return;
    tree_ = std::move(view);
  } else {
    // 这个文件夹没了（被删除、改名或移走）：不再读它
    if (!parsed || !parsed->available) folders_.erase(folder);
    else folders_[folder] = parsed->nodes;
    std::vector<contracts::DiskTreeNode> nodes;
    appendFolder(std::string(), nodes);
    if (nodes == tree_.nodes) return;
    tree_.nodes = std::move(nodes);
  }
  syncSingleFile();
  changed();
  // 刚新建的镜像出现在树里之后选中它
  if (pendingSelect_.empty()) return;
  const std::string path = pendingSelect_;
  const contracts::DiskTreeNode* node = findNode(path);
  if (node == nullptr || node->directory) return;
  pendingSelect_.clear();
  selectImage(path);
}

void FileManagerController::appendFolder(const std::string& folder, std::vector<contracts::DiskTreeNode>& out) const {
  const auto found = folders_.find(folder);
  if (found == folders_.end()) return;
  for (const contracts::DiskTreeNode& node : found->second) {
    out.push_back(node);
    if (node.directory) appendFolder(node.path, out);
  }
}

void FileManagerController::expandFolder(const std::string& path) {
  if (tree_.state != contracts::FileManagerState::Ready || folders_.find(path) != folders_.end()) return;
  loadFolder(path);
}

void FileManagerController::showFolder(const std::string& folder) {
  if (tree_.state != contracts::FileManagerState::Ready) return;
  std::string prefix;
  std::size_t start = 0;
  while (!folder.empty()) {
    const std::size_t slash = folder.find('/', start);
    prefix = folder.substr(0, slash);
    if (prefix != folder && folders_.find(prefix) == folders_.end()) loadFolder(prefix);
    if (slash == std::string::npos) break;
    start = slash + 1;
  }
  loadFolder(folder);
}

void FileManagerController::loadKinds() {
  ask("filemanager.blankKinds", nlohmann::json::object(), [this](const contracts::CoreReply& reply) {
    const auto kinds = reply.ok ? parseBlankKinds(reply.result) : std::nullopt;
    if (!kinds) return;
    kinds_ = *kinds;
    changed();
  });
}

void FileManagerController::loadImage() {
  if (shell_ == nullptr || !image_.selected || image_.kind != contracts::kKindFloppy) return;
  const auto current = ++listRequest_;
  ask("filemanager.list", {{"image", image_.image}, {"directory", image_.directory}}, [this, current](const contracts::CoreReply& reply) {
    if (listRequest_ != current || !image_.selected) return;
    image_.loading = false;
    if (!reply.ok && reply.error.code == "filemanager.entryNotFound" && !image_.directory.empty()) {
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

void FileManagerController::handleEvent(const std::string& name, const nlohmann::json& data) {
  if (name == "filemanager.treeChanged") {
    reloadFolders();
  } else if (name == "filemanager.imageChanged") {
    if (image_.selected && data.is_object() && data.value("image", std::string()) == image_.image) loadImage();
  } else if (name == "media.changed") {
    // 镜像插进或退出软驱、盘片写保护变化：读取它的来源与能不能修改
    if (image_.selected) loadImage();
  }
}

void FileManagerController::handleCoreStatus(const contracts::CoreStatus& status) {
  if (status.state == contracts::CoreState::Running) {
    start();
    return;
  }
  ++treeEpoch_;
  ++listRequest_;
  tree_ = contracts::DiskTreeView{};
  folders_.clear();
  folderRequests_.clear();
  image_ = contracts::DiskImageView{};
  kinds_.clear();
  clipboard_ = Clipboard{};
  itemClipboard_ = ItemClipboard{};
  pendingSelect_.clear();
  changed();
}

// ---------------------------------------------------------------- 选择与浏览

void FileManagerController::chooseRoot() {
  if (shell_ == nullptr) return;
  ask("filemanager.pickRoot", nlohmann::json::object(), [this](const contracts::CoreReply& reply) {
    if (!reply.ok) {
      report(diskErrorText(reply.error.code, reply.error.message));
      return;
    }
    if (!reply.result.is_object() || reply.result.value("canceled", false)) return;
    // 换了目录，原来选中的镜像与剪贴板都作废
    image_ = contracts::DiskImageView{};
    clipboard_ = Clipboard{};
    itemClipboard_ = ItemClipboard{};
    ++treeEpoch_;
    folders_.clear();
    folderRequests_.clear();
    loadFolder(std::string());
  });
}

void FileManagerController::selectImage(const std::string& path) {
  const contracts::DiskTreeNode* node = findNode(path);
  if (node == nullptr || node->directory) return;
  if (image_.selected && image_.image == path) return;
  image_ = contracts::DiskImageView{};
  image_.selected = true;
  image_.image = path;
  image_.imageName = node->name;
  image_.kind = node->kind;
  if (node->kind != contracts::kKindFloppy) {
    // 卡带、音频、图片不用读内容，右边只有这一个文件；上一个软盘镜像正在读的回复作废
    ++listRequest_;
    fillSingleFile(*node);
    changed();
    return;
  }
  image_.loading = true;
  changed();
  loadImage();
}

void FileManagerController::fillSingleFile(const contracts::DiskTreeNode& node) {
  image_.imageName = node.name;
  image_.loading = false;
  image_.failed = false;
  image_.message.clear();
  image_.writable = false;
  image_.subdirectories = false;
  image_.readOnlyReason.clear();
  image_.drive = -1;
  image_.undoDepth = 0;
  if (image_.kind == contracts::kKindUnsupported) {
    // 不支持的文件：右边只显示说明，没有表头与内容
    const std::string extension = fileExtension(node.name);
    image_.failed = true;
    image_.message = extension.empty() ? "不支持的文件类型" : "不支持的文件类型（." + extension + "）";
    image_.entries.clear();
    return;
  }
  contracts::DiskEntryRow row;
  row.name = node.name;
  row.type = upperExtension(node.name);
  row.size = entrySizeText(node.size, false);
  image_.entries = {std::move(row)};
}

void FileManagerController::syncSingleFile() {
  if (!image_.selected || image_.kind == contracts::kKindFloppy) return;
  const contracts::DiskTreeNode* node = findNode(image_.image);
  // 所在的文件夹还没读出来（刚移进去）时还不能说它没了
  const std::size_t slash = image_.image.find_last_of('/');
  const std::string parent = slash == std::string::npos ? std::string() : image_.image.substr(0, slash);
  if (node == nullptr && folders_.find(parent) == folders_.end()) return;
  if (node == nullptr || node->directory || node->kind != image_.kind) {
    // 文件被删掉或换成别的东西了：取消选中
    image_ = contracts::DiskImageView{};
    return;
  }
  fillSingleFile(*node);
}

void FileManagerController::openFolder(const std::string& name) {
  if (!image_.selected) return;
  const auto found = std::find_if(image_.entries.begin(), image_.entries.end(), [&name](const contracts::DiskEntryRow& row) { return row.name == name && row.directory; });
  if (found == image_.entries.end()) return;
  image_.directory.push_back(found->name);
  image_.loading = true;
  changed();
  loadImage();
}

void FileManagerController::goUp() {
  if (!image_.selected || image_.directory.empty()) return;
  image_.directory.pop_back();
  image_.loading = true;
  changed();
  loadImage();
}

// ---------------------------------------------------------------- 修改

void FileManagerController::rename(const std::string& name, const std::string& newName) {
  if (!image_.selected) return;
  operate("filemanager.rename", {{"image", image_.image}, {"entry", entryPath(name)}, {"name", newName}});
}

void FileManagerController::remove(const std::vector<std::string>& names) {
  if (!image_.selected || names.empty()) return;
  const std::string what = names.size() == 1 ? "“" + names.front() + "”" : "选中的 " + std::to_string(names.size()) + " 项";
  if (!confirmation_.ask(kDeleteTitle, "从镜像“" + image_.imageName + "”里删除" + what + "？删除后可以用 Ctrl+Z 撤销。")) return;
  operate("filemanager.remove", {{"image", image_.image}, {"entries", entryPaths(names)}});
}

void FileManagerController::moveInto(const std::vector<std::string>& names, const std::string& folder) {
  if (!image_.selected || names.empty()) return;
  if (!confirmMove(confirmation_, names, "文件夹“" + folder + "”")) return;
  operate("filemanager.move", {{"image", image_.image}, {"entries", entryPaths(names)}, {"target", entryPath(folder)}});
}

void FileManagerController::makeFolder(std::function<void(const std::string& name)> onCreated) {
  if (!image_.selected) return;
  const std::string name = uniqueFolderName(image_.entries);
  operate("filemanager.makeDirectory", {{"image", image_.image}, {"directory", image_.directory}, {"name", name}}, [onCreated = std::move(onCreated), name](const nlohmann::json&) {
    if (onCreated) onCreated(name);
  });
}

void FileManagerController::undo() {
  if (!image_.selected) return;
  operate("filemanager.undo", {{"image", image_.image}}, [this](const nlohmann::json& result) {
    const std::string description = result.is_object() ? result.value("description", std::string()) : std::string();
    notifySuccess(description.empty() ? "已撤销" : "已撤销：" + description);
  });
}

void FileManagerController::cut(const std::vector<std::string>& names) {
  if (!image_.selected || names.empty()) return;
  // 剪贴板只有一份：新的剪切或复制取代原来的，原来弱化显示的条目恢复正常
  clipboard_ = Clipboard{true, true, image_.image, entryPaths(names)};
  itemClipboard_ = ItemClipboard{};
  changed();
}

void FileManagerController::copy(const std::vector<std::string>& names) {
  if (!image_.selected || names.empty()) return;
  clipboard_ = Clipboard{true, false, image_.image, entryPaths(names)};
  itemClipboard_ = ItemClipboard{};
  changed();
}

std::vector<std::string> FileManagerController::cutNames() const {
  std::vector<std::string> names;
  if (!clipboard_.present || !clipboard_.cut || !image_.selected || clipboard_.image != image_.image) return names;
  for (const std::vector<std::string>& entry : clipboard_.entries) {
    // 只算当前文件夹里的条目：路径比当前文件夹多一级，前面逐级相同
    if (entry.size() != image_.directory.size() + 1 || !std::equal(image_.directory.begin(), image_.directory.end(), entry.begin())) continue;
    names.push_back(entry.back());
  }
  return names;
}

void FileManagerController::paste() {
  if (!image_.selected || !clipboard_.present) return;
  const Clipboard source = clipboard_;
  if (source.cut && source.image == image_.image) {
    // 同一个镜像里剪切再粘贴：移动
    if (!confirmMove(confirmation_, entryNames(source.entries), "当前文件夹")) return;
    operate("filemanager.move", {{"image", image_.image}, {"entries", source.entries}, {"target", image_.directory}}, [this](const nlohmann::json&) {
      clipboard_ = Clipboard{};
      changed();
    });
    return;
  }
  // 剪切到另一个镜像：复制过去再从来源镜像里删掉，也是移动
  if (source.cut && !confirmMove(confirmation_, entryNames(source.entries), "镜像“" + image_.imageName + "”")) return;
  operate("filemanager.copy", {{"image", source.image}, {"entries", source.entries}, {"targetImage", image_.image}, {"target", image_.directory}}, [this, source](const nlohmann::json&) {
    if (!source.cut) return;
    // 剪切到另一个镜像：复制成功后再从来源镜像里删掉
    clipboard_ = Clipboard{};
    changed();
    operate("filemanager.remove", {{"image", source.image}, {"entries", source.entries}});
  });
}

void FileManagerController::importPaths(const std::vector<std::string>& paths, const std::string& folder) {
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
  operate("filemanager.importDropped", {{"image", image_.image}, {"directory", directory}, {"droppedIds", ids}}, [this](const nlohmann::json& result) {
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

void FileManagerController::exportEntries(const std::vector<std::string>& names, std::function<void(std::vector<contracts::DiskExportItem> items)> done) {
  if (!image_.selected || names.empty() || shell_ == nullptr) {
    if (done) done({});
    return;
  }
  ask("filemanager.readEntries", {{"image", image_.image}, {"entries", entryPaths(names)}}, [this, done = std::move(done)](const contracts::CoreReply& reply) {
    const auto items = reply.ok ? parseExport(reply.result) : std::nullopt;
    if (!items) {
      report(reply.ok ? "读到的文件内容不合格" : diskErrorText(reply.error.code, reply.error.message));
      if (done) done({});
      return;
    }
    if (done) done(*items);
  });
}

void FileManagerController::createImage(const std::string& directory, const std::string& kind) {
  operate("filemanager.createImage", {{"directory", directory}, {"kind", kind}}, [this, directory](const nlohmann::json& result) {
    const std::string path = result.is_object() ? result.value("path", std::string()) : std::string();
    if (path.empty()) return;
    notifySuccess("已新建磁盘镜像：" + baseName(path));
    pendingSelect_ = path;
    showFolder(directory);
  });
}

void FileManagerController::loadIntoDrive(const std::string& path) {
  operate("filemanager.loadImage", {{"image", path}, {"drive", kDriveA}}, [this, path](const nlohmann::json&) { notifySuccess("已载入软驱 A：" + baseName(path)); });
}

void FileManagerController::reveal(const std::string& path) { operate("filemanager.revealImage", {{"image", path}}); }

void FileManagerController::runCartridge(const std::string& path) {
  operate("filemanager.runCartridge", {{"file", path}}, [this, path](const nlohmann::json&) { notifySuccess("已运行卡带：" + baseName(path)); });
}

void FileManagerController::loadTape(const std::string& path) {
  operate("filemanager.loadTape", {{"file", path}}, [this, path](const nlohmann::json&) { notifySuccess("已载入磁带：" + baseName(path)); });
}

void FileManagerController::openFile(const std::string& path) { operate("filemanager.openFile", {{"file", path}}); }

// ---------------------------------------------------------------- 目录里的文件与文件夹

void FileManagerController::followSelection(const std::string& from, const std::string& to) {
  if (!image_.selected) return;
  const std::string moved = rebase(image_.image, from, to);
  if (moved == image_.image) return;
  image_.image = moved;
  image_.imageName = baseName(moved);
  // 移进的文件夹可能还没读过：读出来，树里才找得到它
  const std::size_t slash = moved.find_last_of('/');
  showFolder(slash == std::string::npos ? std::string() : moved.substr(0, slash));
  changed();
}

void FileManagerController::dropSelectionUnder(const std::vector<std::string>& paths) {
  if (!image_.selected) return;
  const bool removed = std::any_of(paths.begin(), paths.end(), [this](const std::string& path) { return isUnder(image_.image, path); });
  if (!removed) return;
  ++listRequest_;
  image_ = contracts::DiskImageView{};
  changed();
}

void FileManagerController::renameItem(const std::string& path, const std::string& newName) {
  operate("filemanager.renameItem", {{"path", path}, {"name", newName}}, [this, path](const nlohmann::json& result) {
    const std::string renamed = result.is_object() ? result.value("path", std::string()) : std::string();
    if (!renamed.empty()) followSelection(path, renamed);
  });
}

void FileManagerController::removeItems(const std::vector<std::string>& paths) {
  if (paths.empty()) return;
  std::string text;
  if (paths.size() == 1) {
    const contracts::DiskTreeNode* node = findNode(paths.front());
    const std::string name = node != nullptr ? node->name : baseName(paths.front());
    text = node != nullptr && node->directory ? "删除文件夹“" + name + "”及里面的全部内容？它们会被移进回收站。" : "删除“" + name + "”？它会被移进回收站。";
  } else {
    text = "删除选中的 " + std::to_string(paths.size()) + " 项？它们会被移进回收站。";
  }
  if (!confirmation_.ask(kDeleteTitle, text)) return;
  operate("filemanager.removeItems", {{"paths", paths}}, [this, paths](const nlohmann::json&) { dropSelectionUnder(paths); });
}

void FileManagerController::moveItems(const std::vector<std::string>& paths, const std::string& targetFolder) {
  if (paths.empty()) return;
  if (!confirmMove(confirmation_, itemNames(paths), folderPhrase(targetFolder))) return;
  operate("filemanager.moveItems", {{"paths", paths}, {"target", targetFolder}}, [this, paths, targetFolder](const nlohmann::json& result) {
    showFolder(targetFolder);
    const auto moved = result.is_object() ? result.find("paths") : result.end();
    if (!result.is_object() || moved == result.end() || !moved->is_array() || moved->size() != paths.size()) return;
    for (std::size_t i = 0; i < paths.size(); ++i) followSelection(paths[i], (*moved)[i].get<std::string>());
  });
}

void FileManagerController::makeItemFolder(const std::string& parent, std::function<void(const std::string& path)> onCreated) {
  operate("filemanager.makeFolder", {{"directory", parent}, {"name", kNewFolderName}}, [this, parent, onCreated = std::move(onCreated)](const nlohmann::json& result) {
    const std::string path = result.is_object() ? result.value("path", std::string()) : std::string();
    if (path.empty()) return;
    showFolder(parent);
    if (onCreated) onCreated(path);
  });
}

void FileManagerController::cutItems(const std::vector<std::string>& paths) {
  if (paths.empty()) return;
  itemClipboard_ = ItemClipboard{true, true, paths};
  clipboard_ = Clipboard{};
  changed();
}

void FileManagerController::copyItems(const std::vector<std::string>& paths) {
  if (paths.empty()) return;
  itemClipboard_ = ItemClipboard{true, false, paths};
  clipboard_ = Clipboard{};
  changed();
}

void FileManagerController::pasteItems(const std::string& targetFolder) {
  if (!itemClipboard_.present) return;
  const ItemClipboard source = itemClipboard_;
  if (source.cut) {
    if (!confirmMove(confirmation_, itemNames(source.paths), folderPhrase(targetFolder))) return;
    operate("filemanager.moveItems", {{"paths", source.paths}, {"target", targetFolder}}, [this, source, targetFolder](const nlohmann::json& result) {
      itemClipboard_ = ItemClipboard{};
      showFolder(targetFolder);
      const auto moved = result.is_object() ? result.find("paths") : result.end();
      if (result.is_object() && moved != result.end() && moved->is_array() && moved->size() == source.paths.size()) {
        for (std::size_t i = 0; i < source.paths.size(); ++i) followSelection(source.paths[i], (*moved)[i].get<std::string>());
      }
      changed();
    });
    return;
  }
  operate("filemanager.copyItems", {{"paths", source.paths}, {"target", targetFolder}}, [this, targetFolder](const nlohmann::json&) { showFolder(targetFolder); });
}

}  // namespace ember::win32::logic
