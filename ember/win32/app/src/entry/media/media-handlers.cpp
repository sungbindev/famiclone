/*
 * ------------------------------------------------------------------------
 * 名称：media-handlers.cpp
 * 说明：media 模块的外壳请求处理的实现。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-19
 * 备注：无。
 * ------------------------------------------------------------------------
 */

#include "entry/media/media-handlers.hpp"

#include <vector>

#include "entry/main-requests/dropped-path.hpp"
#include "entry/main-requests/param-reader.hpp"

namespace ember::win32::entry {

namespace {

/** 软盘镜像常见的扩展名。 */
const std::vector<std::string> kFloppyExtensions{"img", "ima", "vfd", "dsk", "flp"};
/** 磁带可以用的声音文件扩展名（ffmpeg 能解码的常见格式）。 */
const std::vector<std::string> kSoundExtensions{"m4a", "mp3", "wav", "aac", "flac", "ogg", "opus", "mp4", "mka"};

constexpr const char* kDroppedNotFound = "media.droppedNotFound";

/** 选择文件或取拖入的文件：取消时 canceled 为 true，拖入的文件找不到时直接回复错误。 */
enum class PickOutcome { Picked, Canceled, DroppedMissing };

PickOutcome pick(contracts::FileDialog& dialog, const contracts::DroppedFilesService& dropped, const std::optional<std::int64_t>& droppedId, const std::vector<std::string>& extensions,
                 const contracts::FileDialogOptions& options, std::string& path) {
  if (droppedId) {
    const auto resolved = resolveDroppedPath(dropped, *droppedId, extensions);
    if (!resolved) return PickOutcome::DroppedMissing;
    path = *resolved;
    return PickOutcome::Picked;
  }
  const auto picked = dialog.pickFile(options);
  if (!picked) return PickOutcome::Canceled;
  path = *picked;
  return PickOutcome::Picked;
}

/** 核心回复里的状态包装成“已选择”的结果。 */
contracts::CoreReply withStatus(contracts::CoreReply reply) {
  if (!reply.ok) return reply;
  return contracts::CoreReply::success({{"canceled", false}, {"status", reply.result.value("status", nlohmann::json(nullptr))}});
}

}  // namespace

MainRequestHandlers createMediaHandlers(MediaDependencies dependencies) {
  MainRequestHandlers handlers;

  handlers["media.openFloppy"] = [dependencies](const nlohmann::json& params, contracts::CoreReplyHandler done) {
    const ParamReader reader(params, {"drive", "droppedId"});
    const std::int64_t drive = reader.requiredInteger("drive", 0, 3);
    const auto droppedId = reader.integer("droppedId", 1, INT64_MAX);
    std::string path;
    switch (pick(dependencies.dialog, dependencies.dropped, droppedId, kFloppyExtensions, {"载入软盘镜像", {{"软盘镜像", kFloppyExtensions}, {"所有文件", {"*"}}}, "floppy"}, path)) {
      case PickOutcome::DroppedMissing:
        done(contracts::CoreReply::failure(kDroppedNotFound, "拖入的文件已失效或不是软盘镜像，请重新拖入"));
        return;
      case PickOutcome::Canceled:
        done(contracts::CoreReply::success({{"canceled", true}}));
        return;
      case PickOutcome::Picked:
        break;
    }
    dependencies.core.request("media.floppyInsert", nlohmann::json{{"drive", drive}, {"path", path}}, [done](contracts::CoreReply reply) { done(withStatus(std::move(reply))); });
  };

  handlers["media.openTape"] = [dependencies](const nlohmann::json& params, contracts::CoreReplyHandler done) {
    const ParamReader reader(params, {"droppedId"});
    const auto droppedId = reader.integer("droppedId", 1, INT64_MAX);
    std::string path;
    switch (pick(dependencies.dialog, dependencies.dropped, droppedId, kSoundExtensions, {"载入磁带", {{"声音文件", kSoundExtensions}, {"所有文件", {"*"}}}, "tape"}, path)) {
      case PickOutcome::DroppedMissing:
        done(contracts::CoreReply::failure(kDroppedNotFound, "拖入的文件已失效或不是声音文件，请重新拖入"));
        return;
      case PickOutcome::Canceled:
        done(contracts::CoreReply::success({{"canceled", true}}));
        return;
      case PickOutcome::Picked:
        break;
    }
    dependencies.core.request("media.tapeLoad", nlohmann::json{{"path", path}}, [done](contracts::CoreReply reply) { done(withStatus(std::move(reply))); });
  };

  handlers["media.openTapeRecording"] = [dependencies](const nlohmann::json& params, contracts::CoreReplyHandler done) {
    const ParamReader reader(params, {});
    const auto path = dependencies.dialog.pickSaveFile({"保存录音", {{"WAV 声音文件", {"wav"}}}, "tapeRecording"});
    if (!path) {
      done(contracts::CoreReply::success({{"canceled", true}}));
      return;
    }
    dependencies.core.request("media.tapeRecord", nlohmann::json{{"path", *path}}, [done](contracts::CoreReply reply) { done(withStatus(std::move(reply))); });
  };

  return handlers;
}

}  // namespace ember::win32::entry
