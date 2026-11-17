/*
 * ------------------------------------------------------------------------
 * 名称：machines.cpp
 * 说明：机型模块的登记：创建 BIOS 的磁盘存取与机型服务，把随安装包带的默认 BIOS 装进还没有 BIOS 的机型，登记 machines.* 请求。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-17
 * 备注：取用 runtime 模块的运行服务，须排在它之后登记；BIOS 保存在用户数据目录的 bios/<机型>/ 下。
 *       默认 BIOS 随安装包放在 <bundledBiosPath>/<机型>/<文件>，核心每次启动时检查：机型已经有 BIOS（用户自己添加的，或以前装进的）就不动，没有才装进，所以用户添加的 BIOS 不会被覆盖；
 *       装进用的是与用户添加相同的 setBios（读取、校验、复制），单个文件不合格只写一条错误日志，不影响启动与其他机型。
 * ------------------------------------------------------------------------
 */

#include "bootstrap/modules/machines.hpp"

#include <algorithm>
#include <exception>
#include <filesystem>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "contracts/machines/machine-service.hpp"
#include "contracts/runtime/runtime-service.hpp"
#include "infra/machines/disk-bios-storage.hpp"
#include "logic/machines/machine-manager.hpp"
#include "platform/logging/error-log.hpp"
#include "requests/machines/machines-requests.hpp"

namespace ember::bootstrap {

namespace {

/** 目录里按文件名排在最前面的普通文件；目录不存在或没有文件时为空。 */
std::optional<std::filesystem::path> firstFileIn(const std::filesystem::path& directory) {
  std::error_code error;
  std::vector<std::filesystem::path> files;
  for (std::filesystem::directory_iterator it(directory, error), end; !error && it != end; it.increment(error)) {
    if (it->is_regular_file(error)) files.push_back(it->path());
  }
  if (files.empty()) return std::nullopt;
  std::sort(files.begin(), files.end());
  return files.front();
}

/** 把默认 BIOS 装进已支持、需要 BIOS 而还没有 BIOS 的机型。 */
void installDefaultBios(contracts::machines::MachineService& service, const std::filesystem::path& bundled, platform::ErrorLog& errors) {
  if (bundled.empty()) return;
  for (const auto& machine : service.list().machines) {
    if (!machine.needsBios || !machine.supported || machine.bios.has_value()) continue;
    const auto file = firstFileIn(bundled / std::filesystem::path(std::u8string(machine.id.begin(), machine.id.end())));
    if (!file) continue;
    try {
      const auto text = file->u8string();
      service.setBios({machine.id, std::string(text.begin(), text.end())});
    } catch (const std::exception& failure) {
      errors.write({"machines", "machines.defaultBiosFailed", std::string("无法装入默认 BIOS：") + failure.what(), {{"machine", machine.id}}});
    }
  }
}

}  // namespace

void registerMachines(ModuleRegistry& registry) {
  auto storage = std::make_shared<infra::machines::DiskBiosStorage>(registry.userDataPath / "bios");
  auto service = std::make_shared<logic::machines::MachineManager>(storage, registry.services.require<contracts::runtime::RuntimeService>(), registry.events);
  installDefaultBios(*service, registry.bundledBiosPath, *registry.errors);
  registry.services.provide<contracts::machines::MachineService>(service);
  requests::machines::addMachineRequests(registry.requests, service);
}

}  // namespace ember::bootstrap
