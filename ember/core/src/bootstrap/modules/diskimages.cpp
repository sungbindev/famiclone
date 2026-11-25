/*
 * ------------------------------------------------------------------------
 * 名称：diskimages.cpp
 * 说明：磁盘镜像管理模块的登记：创建镜像目录的磁盘存取、位流格式解码器与磁盘镜像管理服务，登记 diskimages.* 请求。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-25
 * 备注：取用 runtime 模块的运行服务与软驱（与介质服务共用同一个驱动器），须排在它们之后登记；核心停止时停下目录监视与后台扫描线程。
 * ------------------------------------------------------------------------
 */

#include "bootstrap/modules/diskimages.hpp"

#include <memory>
#include <vector>

#include "bootstrap/apple-bitstream-decoder.hpp"
#include "contracts/hardware/floppy-drive.hpp"
#include "contracts/runtime/runtime-service.hpp"
#include "infra/diskimages/disk-image-storage.hpp"
#include "logic/diskimages/disk-image-manager.hpp"
#include "platform/files/os-file-watch.hpp"
#include "platform/files/trash-bin.hpp"
#include "requests/diskimages/diskimages-requests.hpp"

namespace ember::bootstrap {

void registerDiskimages(ModuleRegistry& registry) {
  auto service = std::make_shared<logic::diskimages::DiskImageManager>(
      std::make_shared<infra::diskimages::DiskImageStorageOnDisk>(platform::createOsTrashBin()), platform::createOsFileWatchFactory(), registry.services.require<contracts::runtime::RuntimeService>(),
      std::vector<std::shared_ptr<contracts::hardware::FloppyDrive>>{registry.services.require<contracts::hardware::FloppyDrive>()}, std::make_shared<AppleBitstreamDecoder>(), registry.events,
      registry.errors);
  registry.services.provide<contracts::diskimages::DiskImageService>(service);
  requests::diskimages::addDiskimagesRequests(registry.requests, service);
  registry.stopHandlers.push_back([service] { service->shutdown(); });
}

}  // namespace ember::bootstrap
