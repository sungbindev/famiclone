/*
 * ------------------------------------------------------------------------
 * 名称：diskimages-requests.cpp
 * 说明：diskimages 模块的请求登记实现：校验参数，调用磁盘镜像管理服务，返回结果。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-25
 * ------------------------------------------------------------------------
 */

#include "requests/diskimages/diskimages-requests.hpp"

namespace ember::requests::diskimages {

namespace contract = ::ember::contracts::diskimages;

void addDiskimagesRequests(platform::RequestRouter& router, std::shared_ptr<contract::DiskImageService> service) {
  router.add("diskimages", contract::DISKIMAGES_SET_ROOT_REQUEST, [service](const nlohmann::json& params, const platform::CancellationToken&) {
    return nlohmann::json(service->setRoot(contract::parseDiskimagesSetRootParams(params, "params")));
  });
  router.add("diskimages", contract::DISKIMAGES_TREE_REQUEST, [service](const nlohmann::json& params, const platform::CancellationToken&) {
    static_cast<void>(contract::parseDiskimagesTreeParams(params, "params"));
    return nlohmann::json(service->tree());
  });
  router.add("diskimages", contract::DISKIMAGES_LIST_REQUEST, [service](const nlohmann::json& params, const platform::CancellationToken&) {
    return nlohmann::json(service->list(contract::parseDiskimagesListParams(params, "params")));
  });
  router.add("diskimages", contract::DISKIMAGES_RENAME_REQUEST, [service](const nlohmann::json& params, const platform::CancellationToken&) {
    return nlohmann::json(service->rename(contract::parseDiskimagesRenameParams(params, "params")));
  });
  router.add("diskimages", contract::DISKIMAGES_REMOVE_REQUEST, [service](const nlohmann::json& params, const platform::CancellationToken&) {
    return nlohmann::json(service->remove(contract::parseDiskimagesRemoveParams(params, "params")));
  });
  router.add("diskimages", contract::DISKIMAGES_MOVE_REQUEST, [service](const nlohmann::json& params, const platform::CancellationToken&) {
    return nlohmann::json(service->move(contract::parseDiskimagesMoveParams(params, "params")));
  });
  router.add("diskimages", contract::DISKIMAGES_COPY_REQUEST, [service](const nlohmann::json& params, const platform::CancellationToken&) {
    return nlohmann::json(service->copy(contract::parseDiskimagesCopyParams(params, "params")));
  });
  router.add("diskimages", contract::DISKIMAGES_MAKE_DIRECTORY_REQUEST, [service](const nlohmann::json& params, const platform::CancellationToken&) {
    return nlohmann::json(service->makeDirectory(contract::parseDiskimagesMakeDirectoryParams(params, "params")));
  });
  router.add("diskimages", contract::DISKIMAGES_UNDO_REQUEST, [service](const nlohmann::json& params, const platform::CancellationToken&) {
    return nlohmann::json(service->undo(contract::parseDiskimagesUndoParams(params, "params")));
  });
  router.add("diskimages", contract::DISKIMAGES_READ_ENTRIES_REQUEST, [service](const nlohmann::json& params, const platform::CancellationToken&) {
    return nlohmann::json(service->readEntries(contract::parseDiskimagesReadEntriesParams(params, "params")));
  });
  router.add("diskimages", contract::DISKIMAGES_IMPORT_FILES_REQUEST, [service](const nlohmann::json& params, const platform::CancellationToken&) {
    return nlohmann::json(service->importFiles(contract::parseDiskimagesImportFilesParams(params, "params")));
  });
  router.add("diskimages", contract::DISKIMAGES_CREATE_IMAGE_REQUEST, [service](const nlohmann::json& params, const platform::CancellationToken&) {
    return nlohmann::json(service->createImage(contract::parseDiskimagesCreateImageParams(params, "params")));
  });
  router.add("diskimages", contract::DISKIMAGES_BLANK_KINDS_REQUEST, [service](const nlohmann::json& params, const platform::CancellationToken&) {
    static_cast<void>(contract::parseDiskimagesBlankKindsParams(params, "params"));
    return nlohmann::json(service->blankKinds());
  });
  router.add("diskimages", contract::DISKIMAGES_IMAGE_PATH_REQUEST, [service](const nlohmann::json& params, const platform::CancellationToken&) {
    return nlohmann::json(service->imagePath(contract::parseDiskimagesImagePathParams(params, "params")));
  });
}

}  // namespace ember::requests::diskimages
