/*
 * ------------------------------------------------------------------------
 * 名称：media-requests.cpp
 * 说明：media 模块的请求登记实现：校验参数，调用介质服务，返回结果。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-15
 * ------------------------------------------------------------------------
 */

#include "requests/media/media-requests.hpp"

namespace ember::requests::media {

namespace contract = ::ember::contracts::media;

void addMediaRequests(platform::RequestRouter& router, std::shared_ptr<contract::MediaService> service) {
  router.add("media", contract::MEDIA_FLOPPY_INSERT_REQUEST, [service](const nlohmann::json& params, const platform::CancellationToken&) {
    return nlohmann::json(service->floppyInsert(contract::parseMediaFloppyInsertParams(params, "params")));
  });
  router.add("media", contract::MEDIA_FLOPPY_EJECT_REQUEST, [service](const nlohmann::json& params, const platform::CancellationToken&) {
    return nlohmann::json(service->floppyEject(contract::parseMediaFloppyEjectParams(params, "params")));
  });
  router.add("media", contract::MEDIA_FLOPPY_SAVE_REQUEST, [service](const nlohmann::json& params, const platform::CancellationToken&) {
    return nlohmann::json(service->floppySave(contract::parseMediaFloppySaveParams(params, "params")));
  });
  router.add("media", contract::MEDIA_FLOPPY_WRITE_PROTECT_REQUEST, [service](const nlohmann::json& params, const platform::CancellationToken&) {
    return nlohmann::json(service->floppyWriteProtect(contract::parseMediaFloppyWriteProtectParams(params, "params")));
  });
  router.add("media", contract::MEDIA_FLOPPY_STATUS_REQUEST, [service](const nlohmann::json& params, const platform::CancellationToken&) {
    static_cast<void>(contract::parseMediaFloppyStatusParams(params, "params"));
    return nlohmann::json(service->floppyStatus());
  });
  router.add("media", contract::MEDIA_FLOPPY_FAULT_REQUEST, [service](const nlohmann::json& params, const platform::CancellationToken&) {
    return nlohmann::json(service->floppyFault(contract::parseMediaFloppyFaultParams(params, "params")));
  });
  router.add("media", contract::MEDIA_PRINTER_STATUS_REQUEST, [service](const nlohmann::json& params, const platform::CancellationToken&) {
    static_cast<void>(contract::parseMediaPrinterStatusParams(params, "params"));
    return nlohmann::json(service->printerStatus());
  });
  router.add("media", contract::MEDIA_PRINTER_FLUSH_REQUEST, [service](const nlohmann::json& params, const platform::CancellationToken&) {
    static_cast<void>(contract::parseMediaPrinterFlushParams(params, "params"));
    return nlohmann::json(service->printerFlush());
  });
}

}  // namespace ember::requests::media
