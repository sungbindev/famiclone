/*
 * ------------------------------------------------------------------------
 * 名称：filemanager-handlers.cpp
 * 说明：filemanager 模块的外壳请求处理的实现。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-25
 * 备注：无。
 * ------------------------------------------------------------------------
 */

#include "entry/filemanager/filemanager-handlers.hpp"

#include <functional>
#include <vector>

#include "entry/main-requests/cartridge-loading.hpp"
#include "entry/main-requests/param-reader.hpp"

namespace ember::win32::entry {

namespace {

constexpr const char* kDroppedNotFound = "filemanager.droppedNotFound";
constexpr const char* kRevealUnavailable = "filemanager.revealUnavailable";
constexpr const char* kOpenUnavailable = "filemanager.openUnavailable";
/** 一次最多拖入的文件与文件夹个数，与核心请求的上限一致。 */
constexpr std::size_t kMaxDropped = 1000;
/** 镜像里文件夹路径的最大层数、一层名称的最大长度，与核心请求的上限一致。 */
constexpr std::size_t kMaxDirectoryDepth = 64;
constexpr std::size_t kMaxNameLength = 255;

/** 读“镜像里的文件夹路径”：逐级名称的数组，可以是空数组（根目录）。 */
std::vector<std::string> readDirectory(const nlohmann::json& params) {
  const auto found = params.find("directory");
  if (found == params.end() || !found->is_array() || found->size() > kMaxDirectoryDepth) throw ParamError("directory 必须是名称数组");
  std::vector<std::string> result;
  for (const nlohmann::json& part : *found) {
    if (!part.is_string() || part.get<std::string>().empty() || part.get<std::string>().size() > kMaxNameLength) throw ParamError("directory 里有不合格的名称");
    result.push_back(part.get<std::string>());
  }
  return result;
}

/** 读拖入文件的编号数组。 */
std::vector<std::int64_t> readDroppedIds(const nlohmann::json& params) {
  const auto found = params.find("droppedIds");
  if (found == params.end() || !found->is_array() || found->empty() || found->size() > kMaxDropped) throw ParamError("droppedIds 必须是 1 到 1000 个编号");
  std::vector<std::int64_t> result;
  for (const nlohmann::json& id : *found) {
    if (!id.is_number_integer() || id.get<std::int64_t>() < 1) throw ParamError("droppedIds 里有不合格的编号");
    result.push_back(id.get<std::int64_t>());
  }
  return result;
}

/** 把目录里文件的相对路径交给核心解析成完整路径；解析失败时原样回复错误。 */
void withFullPath(const FileManagerDependencies& dependencies, const std::string& file, const contracts::CoreReplyHandler& done, std::function<void(const std::string& path)> use) {
  dependencies.core.request("filemanager.imagePath", nlohmann::json{{"image", file}}, [done, use = std::move(use)](contracts::CoreReply located) {
    if (!located.ok) {
      done(std::move(located));
      return;
    }
    use(located.result.value("path", std::string()));
  });
}

}  // namespace

MainRequestHandlers createFileManagerHandlers(FileManagerDependencies dependencies) {
  MainRequestHandlers handlers;

  handlers["filemanager.openRoot"] = [dependencies](const nlohmann::json& params, contracts::CoreReplyHandler done) {
    const ParamReader reader(params, {});
    const auto& saved = dependencies.shellState.get().fileManagerRoot;
    if (!saved) {
      done(contracts::CoreReply::success({{"root", nullptr}}));
      return;
    }
    const std::string root = *saved;
    dependencies.core.request("filemanager.setRoot", nlohmann::json{{"path", root}}, [done, root](contracts::CoreReply reply) {
      done(reply.ok ? contracts::CoreReply::success({{"root", root}}) : std::move(reply));
    });
  };

  handlers["filemanager.pickRoot"] = [dependencies](const nlohmann::json& params, contracts::CoreReplyHandler done) {
    const ParamReader reader(params, {});
    const auto picked = dependencies.dialog.pickDirectory({"选择文件管理的目录", "fileManager"});
    if (!picked) {
      done(contracts::CoreReply::success({{"canceled", true}}));
      return;
    }
    const std::string root = *picked;
    dependencies.core.request("filemanager.setRoot", nlohmann::json{{"path", root}}, [dependencies, done, root](contracts::CoreReply reply) {
      if (!reply.ok) {
        done(std::move(reply));
        return;
      }
      dependencies.shellState.updateFileManagerRoot(root);
      done(contracts::CoreReply::success({{"canceled", false}, {"root", root}}));
    });
  };

  handlers["filemanager.importDropped"] = [dependencies](const nlohmann::json& params, contracts::CoreReplyHandler done) {
    const ParamReader reader(params, {"image", "directory", "droppedIds"});
    const std::string image = reader.requiredString("image");
    const std::vector<std::string> directory = readDirectory(params);
    std::vector<std::string> paths;
    for (const std::int64_t id : readDroppedIds(params)) {
      const auto path = dependencies.dropped.resolve(id);
      if (!path) {
        done(contracts::CoreReply::failure(kDroppedNotFound, "拖入的文件已失效，请重新拖入"));
        return;
      }
      paths.push_back(*path);
    }
    dependencies.core.request("filemanager.importFiles", nlohmann::json{{"image", image}, {"directory", directory}, {"paths", paths}}, std::move(done));
  };

  handlers["filemanager.loadImage"] = [dependencies](const nlohmann::json& params, contracts::CoreReplyHandler done) {
    const ParamReader reader(params, {"image", "drive"});
    const std::string image = reader.requiredString("image");
    const std::int64_t drive = reader.requiredInteger("drive", 0, 3);
    dependencies.core.request("filemanager.imagePath", nlohmann::json{{"image", image}}, [dependencies, done, drive](contracts::CoreReply located) {
      if (!located.ok) {
        done(std::move(located));
        return;
      }
      const std::string path = located.result.value("path", std::string());
      dependencies.core.request("media.floppyInsert", nlohmann::json{{"drive", drive}, {"path", path}}, [done](contracts::CoreReply reply) {
        done(reply.ok ? contracts::CoreReply::success(nlohmann::json::object()) : std::move(reply));
      });
    });
  };

  handlers["filemanager.revealImage"] = [dependencies](const nlohmann::json& params, contracts::CoreReplyHandler done) {
    const ParamReader reader(params, {"image"});
    const std::string image = reader.requiredString("image");
    dependencies.core.request("filemanager.imagePath", nlohmann::json{{"image", image}}, [dependencies, done](contracts::CoreReply located) {
      if (!located.ok) {
        done(std::move(located));
        return;
      }
      const std::string error = dependencies.directories.reveal(located.result.value("path", std::string()));
      done(error.empty() ? contracts::CoreReply::success(nlohmann::json::object()) : contracts::CoreReply::failure(kRevealUnavailable, "无法在资源管理器里显示文件：" + error));
    });
  };

  handlers["filemanager.runCartridge"] = [dependencies](const nlohmann::json& params, contracts::CoreReplyHandler done) {
    const ParamReader reader(params, {"file"});
    withFullPath(dependencies, reader.requiredString("file"), done, [dependencies, done](const std::string& path) {
      loadAndRememberCartridge(dependencies.core, dependencies.recent, path, [done](contracts::CoreReply reply) { done(reply.ok ? contracts::CoreReply::success(nlohmann::json::object()) : std::move(reply)); });
    });
  };

  handlers["filemanager.loadTape"] = [dependencies](const nlohmann::json& params, contracts::CoreReplyHandler done) {
    const ParamReader reader(params, {"file"});
    withFullPath(dependencies, reader.requiredString("file"), done, [dependencies, done](const std::string& path) {
      dependencies.core.request("media.tapeLoad", nlohmann::json{{"path", path}}, [done](contracts::CoreReply reply) { done(reply.ok ? contracts::CoreReply::success(nlohmann::json::object()) : std::move(reply)); });
    });
  };

  handlers["filemanager.openFile"] = [dependencies](const nlohmann::json& params, contracts::CoreReplyHandler done) {
    const ParamReader reader(params, {"file"});
    withFullPath(dependencies, reader.requiredString("file"), done, [dependencies, done](const std::string& path) {
      const std::string error = dependencies.directories.openFile(path);
      done(error.empty() ? contracts::CoreReply::success(nlohmann::json::object()) : contracts::CoreReply::failure(kOpenUnavailable, "无法用系统的程序打开文件：" + error));
    });
  };

  return handlers;
}

}  // namespace ember::win32::entry
