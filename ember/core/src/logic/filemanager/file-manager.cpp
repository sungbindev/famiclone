/*
 * ------------------------------------------------------------------------
 * 名称：file-manager.cpp
 * 说明：文件管理服务的实现。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-25
 * 备注：无。
 * ------------------------------------------------------------------------
 */

#include "logic/filemanager/file-manager.hpp"

#include <algorithm>
#include <chrono>
#include <iterator>

#include "contracts/filemanager/filemanager-error.hpp"
#include "logic/filemanager/disk-image-opener.hpp"
#include "logic/filemanager/disk-text.hpp"
#include "shared/errors/core-error.hpp"

namespace ember::logic::filemanager {

namespace api = contracts::filemanager;
namespace hw = contracts::hardware;

namespace {

/** 树里最多列出的镜像文件个数。 */
constexpr std::size_t MAX_IMAGES = 5000;
/** 每个镜像最多记的撤销步数与字节数。 */
constexpr std::size_t MAX_UNDO_STEPS = 100;
constexpr std::size_t MAX_UNDO_BYTES = 32U * 1024U * 1024U;
/** 导出的内容总量上限（字节）。 */
constexpr std::size_t MAX_EXPORT_BYTES = 64U * 1024U * 1024U;
/** 导出时文件夹的最大深度。 */
constexpr int MAX_EXPORT_DEPTH = 32;
/** 两处改动之间相隔不超过这么多个相同的字节时合成一处，少做几次写入。 */
constexpr std::size_t MERGE_GAP = 16;
/** 文件监视来了通知后等多久再扫描，让一阵连续的变化合成一次扫描。 */
constexpr std::chrono::milliseconds SCAN_DELAY{150};
/** 格式识别每识别这么多个镜像通知一次界面。 */
constexpr std::size_t PUBLISH_BATCH = 8;
constexpr const char* BLANK_IMAGE_NAME = "新建磁盘镜像";

[[noreturn]] void fail(std::string_view code, const std::string& message) { throw shared::CoreError(code, message); }

/** 路径最后一个 . 之后的部分，小写；没有时为空。 */
std::string extensionOf(const std::string& path) {
  const std::size_t slash = path.find_last_of('/');
  const std::size_t dot = path.rfind('.');
  if (dot == std::string::npos || (slash != std::string::npos && dot < slash)) return {};
  return lowerAscii(path.substr(dot + 1));
}

/** 路径最后一级的名称。 */
std::string baseNameOf(const std::string& path) {
  const std::size_t slash = path.find_last_of('/');
  return slash == std::string::npos ? path : path.substr(slash + 1);
}

/** 非软盘文件在树里显示的格式名称：NES 卡带、PNG 图片、MP3 音频。 */
std::string fileLabel(api::FileKind kind, const std::string& extension) {
  std::string upper = extension;
  for (char& c : upper) {
    if (c >= 'a' && c <= 'z') c = static_cast<char>(c - 'a' + 'A');
  }
  switch (kind) {
    case api::FileKind::Cartridge:
      return upper + " 卡带";
    case api::FileKind::Picture:
      return upper + " 图片";
    case api::FileKind::Audio:
      return upper + " 音频";
    case api::FileKind::Floppy:
      break;
  }
  return upper;
}

/** 比较两份字节，把有差别的区间收集成改动（相隔不超过 MERGE_GAP 的合成一处）。 */
std::vector<std::size_t> differingStarts(const std::vector<std::uint8_t>& before, const std::vector<std::uint8_t>& after, std::vector<std::size_t>& ends) {
  std::vector<std::size_t> starts;
  std::size_t position = 0;
  while (position < before.size()) {
    if (before[position] == after[position]) {
      ++position;
      continue;
    }
    const std::size_t start = position;
    std::size_t end = position + 1;
    std::size_t equalRun = 0;
    for (position = end; position < before.size() && equalRun < MERGE_GAP; ++position) {
      if (before[position] == after[position]) {
        ++equalRun;
      } else {
        equalRun = 0;
        end = position + 1;
      }
    }
    starts.push_back(start);
    ends.push_back(end);
  }
  return starts;
}

/** 导出时用的一个条目是不是文件夹：在它所在的文件夹里查。 */
bool isDirectoryEntry(api::DiskFileSystem& fileSystem, const api::EntryPath& entry) {
  const api::EntryPath parent(entry.begin(), entry.end() - 1);
  for (const api::DiskEntry& item : fileSystem.list(parent)) {
    if (equalsIgnoreCase(item.name, entry.back())) return item.directory;
  }
  fail(api::ERROR_ENTRY_NOT_FOUND, "找不到 " + entry.back());
}

/** 把一个条目（文件夹连同里面的内容）收进导出的列表。 */
void collectExport(api::DiskFileSystem& fileSystem, const api::EntryPath& entry, const std::vector<std::string>& prefix, std::vector<api::ExportItem>& items, std::size_t& total, int depth) {
  if (depth > MAX_EXPORT_DEPTH) fail(api::ERROR_CORRUPT, "文件夹层数过多，文件系统可能已损坏");
  if (isDirectoryEntry(fileSystem, entry)) {
    std::vector<std::string> path = prefix;
    path.push_back(windowsSafeName(entry.back()));
    items.push_back(api::ExportItem{path, true, {}});
    for (const api::DiskEntry& child : fileSystem.list(entry)) {
      api::EntryPath childPath = entry;
      childPath.push_back(child.name);
      collectExport(fileSystem, childPath, path, items, total, depth + 1);
    }
    return;
  }
  api::ExportedFile file = fileSystem.readFile(entry);
  total += file.data.size();
  if (total > MAX_EXPORT_BYTES) fail(api::ERROR_NO_SPACE, "导出的内容超过 64 MB");
  std::vector<std::string> path = prefix;
  path.push_back(windowsSafeName(file.name));
  items.push_back(api::ExportItem{path, false, base64Encode(file.data)});
}

/** 把镜像里的一个条目（文件夹连同里面的内容）读成拖入内容的样子，复制时再写进目标。 */
api::HostItem readImageItem(api::DiskFileSystem& fileSystem, const api::EntryPath& entry, std::size_t& total, int depth) {
  if (depth > MAX_EXPORT_DEPTH) fail(api::ERROR_CORRUPT, "文件夹层数过多，文件系统可能已损坏");
  api::HostItem item;
  if (isDirectoryEntry(fileSystem, entry)) {
    item.name = entry.back();
    item.directory = true;
    for (const api::DiskEntry& child : fileSystem.list(entry)) {
      api::EntryPath childPath = entry;
      childPath.push_back(child.name);
      item.children.push_back(readImageItem(fileSystem, childPath, total, depth + 1));
    }
    return item;
  }
  api::ExportedFile file = fileSystem.readFile(entry);
  total += file.data.size();
  if (total > MAX_EXPORT_BYTES) fail(api::ERROR_NO_SPACE, "复制的内容超过 64 MB");
  item.name = file.name;
  item.data = std::move(file.data);
  return item;
}

/** 文件夹名不合格时退一步用的名称：只留字母数字、大写、最多 8 个字符，以字母开头。 */
std::string fallbackFolderName(const std::string& name) {
  std::string result;
  for (const char c : upperAscii(name)) {
    if ((c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9')) result.push_back(c);
    if (result.size() == 8) break;
  }
  if (result.empty() || result.front() < 'A' || result.front() > 'Z') result.insert(result.begin(), 'D');
  if (result.size() > 8) result.resize(8);
  return result;
}

/** 把拖入的一个文件或文件夹写进镜像的一个文件夹；失败时抛出，已经写进去的保留。 */
void importItem(api::DiskFileSystem& fileSystem, const api::EntryPath& directory, const api::HostItem& item) {
  if (!item.directory) {
    fileSystem.writeFile(directory, item.name, item.data);
    return;
  }
  if (!fileSystem.traits().subdirectories) fail(api::ERROR_UNSUPPORTED, fileSystem.traits().formatName + " 磁盘没有文件夹，不能导入文件夹");
  std::string folder = item.name;
  try {
    fileSystem.makeDirectory(directory, folder);
  } catch (const shared::CoreError& error) {
    if (error.code() != api::ERROR_INVALID_NAME) throw;
    folder = fallbackFolderName(item.name);
    fileSystem.makeDirectory(directory, folder);
  }
  api::EntryPath inner = directory;
  inner.push_back(folder);
  for (const api::HostItem& child : item.children) importItem(fileSystem, inner, child);
}

}  // namespace

FileManager::FileManager(std::shared_ptr<api::FileManagerStorage> storage, std::shared_ptr<platform::FileWatchFactory> watchFactory, std::shared_ptr<contracts::runtime::RuntimeService> runtime,
                                   std::vector<std::shared_ptr<hw::FloppyDrive>> drives, std::shared_ptr<api::AppleBitstreamDecoder> decoder, std::shared_ptr<platform::EventSink> events,
                                   std::shared_ptr<platform::ErrorLog> errors)
    : storage_(std::move(storage)),
      watchFactory_(std::move(watchFactory)),
      runtime_(std::move(runtime)),
      drives_(std::move(drives)),
      decoder_(std::move(decoder)),
      events_(std::move(events)),
      errors_(std::move(errors)) {
  scanThread_ = std::thread([this] { scanLoop(); });
}

FileManager::~FileManager() { shutdown(); }

void FileManager::shutdown() {
  std::unique_ptr<platform::FileWatcher> watcher;
  {
    std::lock_guard lock(stateMutex_);
    watcher = std::move(watcher_);
  }
  // 监视的回调只碰 signalMutex_，在锁外销毁监视才不会死锁
  watcher.reset();
  {
    std::lock_guard lock(signalMutex_);
    stopping_ = true;
  }
  wake_.notify_all();
  if (scanThread_.joinable()) scanThread_.join();
}

// ---------------------------------------------------------------- 目录与扫描

api::FilemanagerSetRootResult FileManager::setRoot(const api::FilemanagerSetRootParams& params) {
  const std::string path = params.path.value_or(std::string());
  std::unique_ptr<platform::FileWatcher> oldWatcher;
  std::unique_ptr<platform::FileWatcher> newWatcher;
  const bool available = !path.empty() && storage_->isDirectory(path);
  if (available && watchFactory_ != nullptr) {
    newWatcher = watchFactory_->watchDirectory(path, [this] { requestScan(); });
    if (newWatcher == nullptr) logError(std::string(api::ERROR_IMAGE_UNREADABLE), "无法监视目录，目录里的变化不会自动同步", {});
  }
  {
    std::lock_guard lock(stateMutex_);
    oldWatcher = std::move(watcher_);
    watcher_ = std::move(newWatcher);
    root_ = path;
    available_ = available;
    limited_ = false;
    nodes_.clear();
    formats_.clear();
    stamps_.clear();
    undo_.clear();
    undoBytes_ = 0;
  }
  oldWatcher.reset();
  publishTreeChanged();
  requestScan();
  return api::FilemanagerSetRootResult{available};
}

api::FilemanagerTreeResult FileManager::tree() {
  api::FilemanagerTreeResult result;
  std::lock_guard lock(stateMutex_);
  if (!root_.empty()) result.root = root_;
  result.available = available_;
  result.scanning = scanning_;
  result.limited = limited_;
  result.nodes = nodes_;
  return result;
}

void FileManager::requestScan() {
  {
    std::lock_guard lock(signalMutex_);
    scanRequested_ = true;
  }
  wake_.notify_all();
}

void FileManager::scanLoop() {
  std::unique_lock lock(signalMutex_);
  while (true) {
    wake_.wait(lock, [this] { return stopping_ || scanRequested_; });
    if (stopping_) return;
    // 稍等一下，让连续的变化合成一次扫描
    wake_.wait_for(lock, SCAN_DELAY, [this] { return stopping_; });
    if (stopping_) return;
    scanRequested_ = false;
    lock.unlock();
    std::string root;
    {
      std::lock_guard state(stateMutex_);
      root = root_;
    }
    try {
      runScan(root);
    } catch (const std::exception& error) {
      logError(std::string(shared::ERROR_INTERNAL), std::string("扫描目录失败：") + error.what(), {});
    }
    lock.lock();
  }
}

void FileManager::runScan(const std::string& root) {
  bool limited = false;
  bool available = false;
  std::vector<api::StoredImageInfo> infos;
  if (!root.empty()) infos = storage_->scan(root, MAX_IMAGES, limited, available);
  // 文件夹在前、名称不分大小写排序，父文件夹排在它里面的内容之前
  struct Keyed {
    api::StoredImageInfo info;
    std::vector<std::string> names;
    std::vector<bool> directoryAt;
  };
  std::vector<Keyed> keyed;
  keyed.reserve(infos.size());
  for (api::StoredImageInfo& info : infos) {
    Keyed item;
    std::size_t start = 0;
    while (true) {
      const std::size_t slash = info.path.find('/', start);
      item.names.push_back(info.path.substr(start, slash == std::string::npos ? std::string::npos : slash - start));
      item.directoryAt.push_back(slash != std::string::npos || info.directory);
      if (slash == std::string::npos) break;
      start = slash + 1;
    }
    item.info = std::move(info);
    keyed.push_back(std::move(item));
  }
  std::sort(keyed.begin(), keyed.end(), [](const Keyed& a, const Keyed& b) {
    const std::size_t common = std::min(a.names.size(), b.names.size());
    for (std::size_t i = 0; i < common; ++i) {
      if (a.names[i] == b.names[i]) continue;
      if (a.directoryAt[i] != b.directoryAt[i]) return a.directoryAt[i];
      const std::string left = lowerAscii(a.names[i]);
      const std::string right = lowerAscii(b.names[i]);
      return left != right ? left < right : a.names[i] < b.names[i];
    }
    return a.names.size() < b.names.size();
  });

  std::vector<std::string> changedImages;
  std::vector<std::pair<std::string, std::pair<std::uint64_t, std::int64_t>>> pending;
  {
    std::lock_guard lock(stateMutex_);
    if (root != root_) return;
    std::map<std::string, std::pair<std::uint64_t, std::int64_t>> newStamps;
    std::vector<api::ImageNode> nodes;
    nodes.reserve(keyed.size());
    for (const Keyed& item : keyed) {
      api::ImageNode node;
      node.path = item.info.path;
      node.name = item.names.back();
      node.directory = item.info.directory;
      node.size = static_cast<std::int64_t>(item.info.size);
      if (item.info.directory) {
        node.kind = "folder";
        node.scanned = true;
      } else if (const auto kind = api::kindOfExtension(extensionOf(item.info.path)); kind && *kind != api::FileKind::Floppy) {
        // 卡带、音频、图片不用识别文件系统，直接给出种类与名称
        node.kind = api::kindId(*kind);
        node.scanned = true;
        node.formatName = fileLabel(*kind, extensionOf(item.info.path));
      } else {
        node.kind = api::kindId(api::FileKind::Floppy);
        const std::pair<std::uint64_t, std::int64_t> stamp{item.info.size, item.info.stamp};
        newStamps[node.path] = stamp;
        if (const auto old = stamps_.find(node.path); old != stamps_.end() && old->second != stamp) changedImages.push_back(node.path);
        const auto cached = formats_.find(node.path);
        if (cached != formats_.end() && cached->second.size == stamp.first && cached->second.stamp == stamp.second) {
          node.scanned = true;
          node.format = cached->second.format;
          node.formatName = cached->second.formatName;
          node.writable = cached->second.writable;
        } else {
          pending.emplace_back(node.path, stamp);
        }
      }
      nodes.push_back(std::move(node));
    }
    for (auto it = formats_.begin(); it != formats_.end();) it = newStamps.count(it->first) != 0 ? std::next(it) : formats_.erase(it);
    stamps_ = std::move(newStamps);
    nodes_ = std::move(nodes);
    available_ = available;
    limited_ = limited;
    scanning_ = !pending.empty();
  }
  publishTreeChanged();
  for (const std::string& image : changedImages) publishImageChanged(image);

  // 逐个识别还不认识的镜像：读出来按内容认格式
  std::size_t sincePublish = 0;
  bool finished = true;
  for (const auto& [path, stamp] : pending) {
    {
      std::lock_guard lock(signalMutex_);
      if (stopping_ || scanRequested_) {
        finished = false;
        break;
      }
    }
    CachedFormat format;
    format.size = stamp.first;
    format.stamp = stamp.second;
    format.format = "unknown";
    format.formatName = "未识别";
    try {
      std::vector<std::uint8_t> bytes = storage_->read(root, path);
      const IdentifiedImage identified = identifyImage(bytes, extensionOf(path), decoder_.get());
      format.format = identified.formatId;
      format.formatName = identified.formatName;
      format.writable = identified.fileSystem != nullptr && identified.fileSystem->traits().writable;
    } catch (const shared::CoreError&) {
      // 读不出来（被占用、被删除）：按未识别，下一次变化时再试
    }
    {
      std::lock_guard lock(stateMutex_);
      if (root != root_) return;
      formats_[path] = format;
      const auto node = std::find_if(nodes_.begin(), nodes_.end(), [&path](const api::ImageNode& item) { return item.path == path; });
      if (node != nodes_.end()) {
        node->scanned = true;
        node->format = format.format;
        node->formatName = format.formatName;
        node->writable = format.writable;
      }
    }
    if (++sincePublish >= PUBLISH_BATCH) {
      sincePublish = 0;
      publishTreeChanged();
    }
  }
  if (!pending.empty()) {
    if (finished) {
      std::lock_guard lock(stateMutex_);
      if (root == root_) scanning_ = false;
    }
    publishTreeChanged();
  }
}

// ---------------------------------------------------------------- 读

std::string FileManager::currentRoot() {
  std::lock_guard lock(stateMutex_);
  if (root_.empty()) fail(api::ERROR_NO_ROOT, "还没有设置目录");
  return root_;
}

FileManager::Source FileManager::loadSource(const std::string& root, const std::string& image) {
  const std::string full = storage_->absolutePath(root, image);
  Source source;
  try {
    runtime_->runOnSimulation([&] {
      for (std::size_t index = 0; index < drives_.size(); ++index) {
        const hw::FloppyDrive& drive = *drives_[index];
        const bool holds = drive.inserted() || drive.mediaSuspended();
        if (!holds || drive.source().empty() || !storage_->samePath(drive.source(), full)) continue;
        source.drive = static_cast<std::int64_t>(index);
        source.bytes = drive.image();
        source.writeProtected = drive.writeProtected();
        source.suspended = drive.mediaSuspended();
        return;
      }
    });
  } catch (const shared::CoreError& error) {
    // 运行服务已经停止：软驱里没有可看的盘片，按文件读
    if (error.code() != shared::ERROR_UNAVAILABLE) throw;
  }
  if (source.drive < 0) source.bytes = storage_->read(root, image);
  return source;
}

std::unique_ptr<api::DiskFileSystem> FileManager::openFileSystem(std::vector<std::uint8_t>& bytes, const std::string& image) {
  IdentifiedImage identified = identifyImage(bytes, extensionOf(image), decoder_.get());
  if (identified.fileSystem == nullptr) fail(api::ERROR_UNKNOWN_FORMAT, "不认识这个镜像的格式（" + identified.formatName + "），无法查看");
  return std::move(identified.fileSystem);
}

api::FilemanagerListResult FileManager::list(const api::FilemanagerListParams& params) {
  const std::string root = currentRoot();
  Source source = loadSource(root, params.image);
  const std::unique_ptr<api::DiskFileSystem> fileSystem = openFileSystem(source.bytes, params.image);
  api::FilemanagerListResult result;
  const api::FileSystemTraits& traits = fileSystem->traits();
  result.fileSystem = api::FileSystemInfo{traits.formatId, traits.formatName, traits.writable, traits.subdirectories};
  for (const api::DiskEntry& entry : fileSystem->list(params.directory)) {
    result.entries.push_back(api::EntryInfo{entry.name, entry.directory, entry.type, entry.size, entry.sizeIsAllocated, entry.modified, entry.locked});
  }
  if (source.drive >= 0) result.drive = source.drive;
  if (!traits.writable) result.readOnlyReason = "位流格式（.nib、.woz）的镜像只能查看，不能修改";
  else if (source.drive >= 0 && source.suspended) result.readOnlyReason = "软驱里的盘片被取出，暂时不能修改";
  else if (source.drive >= 0 && source.writeProtected) result.readOnlyReason = "软驱里的盘片已写保护，不能修改";
  {
    std::lock_guard lock(stateMutex_);
    const auto steps = undo_.find(params.image);
    result.undoDepth = steps == undo_.end() ? 0 : static_cast<std::int64_t>(steps->second.size());
  }
  return result;
}

api::FilemanagerReadEntriesResult FileManager::readEntries(const api::FilemanagerReadEntriesParams& params) {
  const std::string root = currentRoot();
  Source source = loadSource(root, params.image);
  const std::unique_ptr<api::DiskFileSystem> fileSystem = openFileSystem(source.bytes, params.image);
  api::FilemanagerReadEntriesResult result;
  std::size_t total = 0;
  for (const api::EntryPath& entry : params.entries) collectExport(*fileSystem, entry, {}, result.items, total, 0);
  return result;
}

// ---------------------------------------------------------------- 写

void FileManager::writeChanges(const std::string& root, const std::string& image, const Source& source, const std::vector<Change>& changes, bool restore) {
  const auto bytesOf = [restore](const Change& change) { return std::span<const std::uint8_t>(restore ? change.before : change.after); };
  if (source.drive < 0) {
    for (const Change& change : changes) storage_->writeAt(root, image, change.offset, bytesOf(change));
    return;
  }
  const std::string full = storage_->absolutePath(root, image);
  bool replaced = false;
  runtime_->runOnSimulation([&] {
    hw::FloppyDrive& drive = *drives_[static_cast<std::size_t>(source.drive)];
    // 读出镜像之后盘片被换了或取出：不把改动写进别的盘片
    if (!drive.inserted() || drive.source().empty() || !storage_->samePath(drive.source(), full)) {
      replaced = true;
      return;
    }
    for (const Change& change : changes) drive.writeFromHost(change.offset, bytesOf(change));
  });
  if (replaced) fail(api::ERROR_SAVE_FAILED, "软驱里的盘片已经换了，改动没有生效");
}

void FileManager::apply(const std::string& image, const std::string& description, const std::function<void(api::DiskFileSystem&)>& operation) {
  std::lock_guard edit(editMutex_);
  const std::string root = currentRoot();
  Source source = loadSource(root, image);
  if (source.drive >= 0 && source.suspended) fail(api::ERROR_READ_ONLY, "软驱里的盘片被取出，暂时不能修改");
  if (source.drive >= 0 && source.writeProtected) fail(api::ERROR_READ_ONLY, "软驱里的盘片已写保护，不能修改");
  const std::vector<std::uint8_t> before = source.bytes;
  std::vector<std::uint8_t> work = before;
  {
    const std::unique_ptr<api::DiskFileSystem> fileSystem = openFileSystem(work, image);
    if (!fileSystem->traits().writable) fail(api::ERROR_READ_ONLY, "位流格式（.nib、.woz）的镜像只能查看，不能修改");
    operation(*fileSystem);
  }
  std::vector<std::size_t> ends;
  const std::vector<std::size_t> starts = differingStarts(before, work, ends);
  if (starts.empty()) return;
  std::vector<Change> changes;
  UndoStep step;
  step.description = description;
  for (std::size_t i = 0; i < starts.size(); ++i) {
    Change change;
    change.offset = starts[i];
    change.before.assign(before.begin() + static_cast<std::ptrdiff_t>(starts[i]), before.begin() + static_cast<std::ptrdiff_t>(ends[i]));
    change.after.assign(work.begin() + static_cast<std::ptrdiff_t>(starts[i]), work.begin() + static_cast<std::ptrdiff_t>(ends[i]));
    step.bytes += change.before.size() + change.after.size();
    changes.push_back(std::move(change));
  }
  writeChanges(root, image, source, changes, false);
  step.changes = std::move(changes);
  {
    std::lock_guard lock(stateMutex_);
    std::vector<UndoStep>& steps = undo_[image];
    steps.push_back(std::move(step));
    std::size_t bytes = 0;
    for (const UndoStep& item : steps) bytes += item.bytes;
    // 超出上限时丢弃最早的几步，但至少留下刚做的这一步
    while (steps.size() > 1 && (steps.size() > MAX_UNDO_STEPS || bytes > MAX_UNDO_BYTES)) {
      bytes -= steps.front().bytes;
      steps.erase(steps.begin());
    }
  }
  publishImageChanged(image);
}

api::FilemanagerRenameResult FileManager::rename(const api::FilemanagerRenameParams& params) {
  apply(params.image, "重命名 " + params.entry.back(), [&params](api::DiskFileSystem& fileSystem) { fileSystem.rename(params.entry, params.name); });
  return {};
}

api::FilemanagerRemoveResult FileManager::remove(const api::FilemanagerRemoveParams& params) {
  std::vector<api::EntryPath> entries = params.entries;
  // 里面的先删：选中了文件夹和它里面的文件时，不会因为先删了文件夹而找不到文件
  std::stable_sort(entries.begin(), entries.end(), [](const api::EntryPath& a, const api::EntryPath& b) { return a.size() > b.size(); });
  const std::string description = entries.size() == 1 ? "删除 " + entries.front().back() : "删除 " + std::to_string(entries.size()) + " 个条目";
  apply(params.image, description, [&entries](api::DiskFileSystem& fileSystem) {
    for (const api::EntryPath& entry : entries) {
      try {
        fileSystem.remove(entry);
      } catch (const shared::CoreError& error) {
        if (error.code() != api::ERROR_ENTRY_NOT_FOUND) throw;
      }
    }
  });
  return {};
}

api::FilemanagerMoveResult FileManager::move(const api::FilemanagerMoveParams& params) {
  const std::string description = params.entries.size() == 1 ? "移动 " + params.entries.front().back() : "移动 " + std::to_string(params.entries.size()) + " 个条目";
  apply(params.image, description, [&params](api::DiskFileSystem& fileSystem) {
    if (!fileSystem.traits().subdirectories) fail(api::ERROR_UNSUPPORTED, fileSystem.traits().formatName + " 磁盘没有文件夹，不能移动文件");
    for (const api::EntryPath& entry : params.entries) fileSystem.move(entry, params.target);
  });
  return {};
}

api::FilemanagerCopyResult FileManager::copy(const api::FilemanagerCopyParams& params) {
  // 先从来源镜像读出全部内容（只读，不占编辑锁），再一次性写进目标镜像
  std::vector<api::HostItem> items;
  {
    const std::string root = currentRoot();
    Source source = loadSource(root, params.image);
    const std::unique_ptr<api::DiskFileSystem> fileSystem = openFileSystem(source.bytes, params.image);
    std::size_t total = 0;
    for (const api::EntryPath& entry : params.entries) items.push_back(readImageItem(*fileSystem, entry, total, 0));
  }
  const std::string description = params.entries.size() == 1 ? "复制 " + params.entries.front().back() : "复制 " + std::to_string(params.entries.size()) + " 个条目";
  apply(params.targetImage, description, [&](api::DiskFileSystem& fileSystem) {
    for (const api::HostItem& item : items) importItem(fileSystem, params.target, item);
  });
  return {};
}

api::FilemanagerMakeDirectoryResult FileManager::makeDirectory(const api::FilemanagerMakeDirectoryParams& params) {
  apply(params.image, "新建文件夹 " + params.name, [&params](api::DiskFileSystem& fileSystem) { fileSystem.makeDirectory(params.directory, params.name); });
  return {};
}

api::FilemanagerImportFilesResult FileManager::importFiles(const api::FilemanagerImportFilesParams& params) {
  api::FilemanagerImportFilesResult result;
  std::vector<api::HostItem> items;
  std::vector<std::size_t> itemOutcome;
  // 先读出拖入的内容：读不了的当场记为失败，不影响其他的
  for (const std::string& path : params.paths) {
    try {
      items.push_back(storage_->readHostItem(path));
      itemOutcome.push_back(result.outcomes.size());
      result.outcomes.push_back(api::ImportOutcome{items.back().name, true, {}});
    } catch (const shared::CoreError& error) {
      result.outcomes.push_back(api::ImportOutcome{baseNameOf(path), false, error.what()});
    }
  }
  if (items.empty()) return result;
  const std::string description = items.size() == 1 ? "导入 " + items.front().name : "导入 " + std::to_string(items.size()) + " 个文件";
  apply(params.image, description, [&](api::DiskFileSystem& fileSystem) {
    for (std::size_t i = 0; i < items.size(); ++i) {
      try {
        importItem(fileSystem, params.directory, items[i]);
      } catch (const shared::CoreError& error) {
        api::ImportOutcome& outcome = result.outcomes[itemOutcome[i]];
        outcome.ok = false;
        outcome.message = error.what();
      }
    }
  });
  return result;
}

api::FilemanagerUndoResult FileManager::undo(const api::FilemanagerUndoParams& params) {
  std::lock_guard edit(editMutex_);
  const std::string root = currentRoot();
  UndoStep step;
  {
    std::lock_guard lock(stateMutex_);
    const auto steps = undo_.find(params.image);
    if (steps == undo_.end() || steps->second.empty()) fail(api::ERROR_NOTHING_TO_UNDO, "没有可以撤销的操作");
    step = steps->second.back();
  }
  Source source = loadSource(root, params.image);
  if (source.drive >= 0 && source.suspended) fail(api::ERROR_READ_ONLY, "软驱里的盘片被取出，暂时不能修改");
  if (source.drive >= 0 && source.writeProtected) fail(api::ERROR_READ_ONLY, "软驱里的盘片已写保护，不能修改");
  // 被撤销的位置必须还是这一步改出来的样子，否则会盖掉后来别人改的内容
  for (const Change& change : step.changes) {
    const bool inRange = change.offset + change.after.size() <= source.bytes.size();
    if (!inRange || !std::equal(change.after.begin(), change.after.end(), source.bytes.begin() + static_cast<std::ptrdiff_t>(change.offset))) {
      fail(api::ERROR_UNDO_BLOCKED, "镜像在这之后被别的程序或机器改动过，不能撤销");
    }
  }
  writeChanges(root, params.image, source, step.changes, true);
  {
    std::lock_guard lock(stateMutex_);
    const auto steps = undo_.find(params.image);
    if (steps != undo_.end() && !steps->second.empty()) steps->second.pop_back();
  }
  publishImageChanged(params.image);
  return api::FilemanagerUndoResult{step.description};
}

// ---------------------------------------------------------------- 新建与路径

api::FilemanagerCreateImageResult FileManager::createImage(const api::FilemanagerCreateImageParams& params) {
  const std::string root = currentRoot();
  const std::vector<BlankImageKind>& kinds = blankImageKinds();
  const auto kind = std::find_if(kinds.begin(), kinds.end(), [&params](const BlankImageKind& item) { return item.id == params.kind; });
  if (kind == kinds.end()) fail(api::ERROR_UNSUPPORTED, "没有这种空白镜像：" + params.kind);
  const std::string path = storage_->create(root, params.directory, BLANK_IMAGE_NAME, kind->extension, createBlankImage(kind->id));
  requestScan();
  return api::FilemanagerCreateImageResult{path};
}

api::FilemanagerBlankKindsResult FileManager::blankKinds() {
  api::FilemanagerBlankKindsResult result;
  for (const BlankImageKind& kind : blankImageKinds()) result.kinds.push_back(api::BlankKind{kind.id, kind.title, kind.extension});
  return result;
}

// ---------------------------------------------------------------- 目录里的文件与文件夹

void FileManager::requireNotInDrive(const std::string& root, const std::string& relative) {
  bool inUse = false;
  try {
    runtime_->runOnSimulation([&] {
      for (const std::shared_ptr<hw::FloppyDrive>& drive : drives_) {
        const bool holds = drive->inserted() || drive->mediaSuspended();
        if (holds && !drive->source().empty() && storage_->covers(root, relative, drive->source())) {
          inUse = true;
          return;
        }
      }
    });
  } catch (const shared::CoreError& error) {
    // 运行服务已经停止：软驱里没有盘片
    if (error.code() != shared::ERROR_UNAVAILABLE) throw;
  }
  if (inUse) fail(api::ERROR_IN_USE, "镜像正插在软驱里，先从软驱里弹出它再操作");
}

void FileManager::forgetUndo(const std::string& path) {
  std::lock_guard lock(stateMutex_);
  for (auto it = undo_.begin(); it != undo_.end();) it = (it->first == path || it->first.rfind(path + "/", 0) == 0) ? undo_.erase(it) : std::next(it);
}

api::FilemanagerRenameItemResult FileManager::renameItem(const api::FilemanagerRenameItemParams& params) {
  std::lock_guard edit(editMutex_);
  const std::string root = currentRoot();
  requireNotInDrive(root, params.path);
  const std::string path = storage_->renameItem(root, params.path, params.name);
  if (path != params.path) forgetUndo(params.path);
  requestScan();
  return api::FilemanagerRenameItemResult{path};
}

api::FilemanagerRemoveItemsResult FileManager::removeItems(const api::FilemanagerRemoveItemsParams& params) {
  std::lock_guard edit(editMutex_);
  const std::string root = currentRoot();
  for (const std::string& path : params.paths) requireNotInDrive(root, path);
  storage_->removeItems(root, params.paths);
  for (const std::string& path : params.paths) forgetUndo(path);
  requestScan();
  return {};
}

api::FilemanagerMoveItemsResult FileManager::moveItems(const api::FilemanagerMoveItemsParams& params) {
  std::lock_guard edit(editMutex_);
  const std::string root = currentRoot();
  for (const std::string& path : params.paths) requireNotInDrive(root, path);
  api::FilemanagerMoveItemsResult result;
  result.paths = storage_->moveItems(root, params.paths, params.target);
  for (const std::string& path : params.paths) forgetUndo(path);
  requestScan();
  return result;
}

api::FilemanagerCopyItemsResult FileManager::copyItems(const api::FilemanagerCopyItemsParams& params) {
  std::lock_guard edit(editMutex_);
  api::FilemanagerCopyItemsResult result;
  result.paths = storage_->copyItems(currentRoot(), params.paths, params.target);
  requestScan();
  return result;
}

api::FilemanagerMakeFolderResult FileManager::makeFolder(const api::FilemanagerMakeFolderParams& params) {
  std::lock_guard edit(editMutex_);
  const std::string path = storage_->makeFolder(currentRoot(), params.directory, params.name);
  requestScan();
  return api::FilemanagerMakeFolderResult{path};
}

api::FilemanagerImagePathResult FileManager::imagePath(const api::FilemanagerImagePathParams& params) { return api::FilemanagerImagePathResult{storage_->absolutePath(currentRoot(), params.image)}; }

// ---------------------------------------------------------------- 事件与日志

void FileManager::publishImageChanged(const std::string& image) { events_->publish(api::FILEMANAGER_IMAGE_CHANGED_EVENT, nlohmann::json(api::FilemanagerImageChangedData{image})); }

void FileManager::publishTreeChanged() { events_->publish(api::FILEMANAGER_TREE_CHANGED_EVENT, nlohmann::json(api::FilemanagerTreeChangedData{})); }

void FileManager::logError(const std::string& code, const std::string& message, const std::string& image) {
  if (errors_ == nullptr) return;
  errors_->write({"filemanager", code, message, image.empty() ? nlohmann::json::object() : nlohmann::json{{"image", image}}});
}

}  // namespace ember::logic::filemanager
