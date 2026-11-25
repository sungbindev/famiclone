/*
 * ------------------------------------------------------------------------
 * 名称：diskimages-handlers.cpp
 * 说明：diskimages 模块的外壳请求处理的实现。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-25
 * 备注：无。
 * ------------------------------------------------------------------------
 */

#include "entry/diskimages/diskimages-handlers.hpp"

#include <vector>

#include "entry/main-requests/param-reader.hpp"

namespace ember::win32::entry {

namespace {

constexpr const char* kDroppedNotFound = "diskimages.droppedNotFound";
constexpr const char* kRevealUnavailable = "diskimages.revealUnavailable";
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

}  // namespace

MainRequestHandlers createDiskimagesHandlers(DiskimagesDependencies dependencies) {
  MainRequestHandlers handlers;

  handlers["diskimages.openRoot"] = [dependencies](const nlohmann::json& params, contracts::CoreReplyHandler done) {
    const ParamReader reader(params, {});
    const auto& saved = dependencies.shellState.get().diskImagesRoot;
    if (!saved) {
      done(contracts::CoreReply::success({{"root", nullptr}}));
      return;
    }
    const std::string root = *saved;
    dependencies.core.request("diskimages.setRoot", nlohmann::json{{"path", root}}, [done, root](contracts::CoreReply reply) {
      done(reply.ok ? contracts::CoreReply::success({{"root", root}}) : std::move(reply));
    });
  };

  handlers["diskimages.pickRoot"] = [dependencies](const nlohmann::json& params, contracts::CoreReplyHandler done) {
    const ParamReader reader(params, {});
    const auto picked = dependencies.dialog.pickDirectory({"选择磁盘镜像目录", "diskImages"});
    if (!picked) {
      done(contracts::CoreReply::success({{"canceled", true}}));
      return;
    }
    const std::string root = *picked;
    dependencies.core.request("diskimages.setRoot", nlohmann::json{{"path", root}}, [dependencies, done, root](contracts::CoreReply reply) {
      if (!reply.ok) {
        done(std::move(reply));
        return;
      }
      dependencies.shellState.updateDiskImagesRoot(root);
      done(contracts::CoreReply::success({{"canceled", false}, {"root", root}}));
    });
  };

  handlers["diskimages.importDropped"] = [dependencies](const nlohmann::json& params, contracts::CoreReplyHandler done) {
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
    dependencies.core.request("diskimages.importFiles", nlohmann::json{{"image", image}, {"directory", directory}, {"paths", paths}}, std::move(done));
  };

  handlers["diskimages.loadImage"] = [dependencies](const nlohmann::json& params, contracts::CoreReplyHandler done) {
    const ParamReader reader(params, {"image", "drive"});
    const std::string image = reader.requiredString("image");
    const std::int64_t drive = reader.requiredInteger("drive", 0, 3);
    dependencies.core.request("diskimages.imagePath", nlohmann::json{{"image", image}}, [dependencies, done, drive](contracts::CoreReply located) {
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

  handlers["diskimages.revealImage"] = [dependencies](const nlohmann::json& params, contracts::CoreReplyHandler done) {
    const ParamReader reader(params, {"image"});
    const std::string image = reader.requiredString("image");
    dependencies.core.request("diskimages.imagePath", nlohmann::json{{"image", image}}, [dependencies, done](contracts::CoreReply located) {
      if (!located.ok) {
        done(std::move(located));
        return;
      }
      const std::string error = dependencies.directories.reveal(located.result.value("path", std::string()));
      done(error.empty() ? contracts::CoreReply::success(nlohmann::json::object()) : contracts::CoreReply::failure(kRevealUnavailable, "无法在资源管理器里显示镜像文件：" + error));
    });
  };

  return handlers;
}

}  // namespace ember::win32::entry
