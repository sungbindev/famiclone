/*
 * ------------------------------------------------------------------------
 * 名称：machine-service.hpp
 * 说明：machines 模块的服务接口：机型清单、添加与移除 BIOS、整机机型开机。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-15
 * 备注：数据类型由 schema/machines.schema.json 生成到 generated/；请求层只经本接口使用 machines 模块。
 * ------------------------------------------------------------------------
 */

#pragma once

#include "contracts/machines/generated/machines.hpp"

namespace ember::contracts::machines {

/** 机型服务。 */
class MachineService {
 public:
  virtual ~MachineService() = default;

  /** 机型清单，顺序固定为 fc、sb97、sb2000、bbk10、bbk20。 */
  [[nodiscard]] virtual MachinesListResult list() const = 0;

  /**
   * 读取并检查 BIOS 文件，保存为机型的 BIOS，替换已有的；成功后发 machines.changed。
   * @throws shared::CoreError machines.* 的错误，见 schema/machines.schema.json。
   */
  virtual MachinesSetBiosResult setBios(const MachinesSetBiosParams& params) = 0;

  /**
   * 删除机型的 BIOS；没有 BIOS 时没有效果；有 BIOS 被删除时发 machines.changed。
   * @throws shared::CoreError machines.notFound、machines.biosNotNeeded、machines.biosAccessFailed。
   */
  virtual MachinesRemoveBiosResult removeBios(const MachinesRemoveBiosParams& params) = 0;

  /**
   * 整机机型开机：读入已保存的 BIOS，装配主机，替换已有的卡带与主机；失败时原有的保持不变。
   * @throws shared::CoreError machines.* 的错误，以及 runtime.hostUnsupported。
   */
  virtual MachinesBootResult boot(const MachinesBootParams& params) = 0;
};

}  // namespace ember::contracts::machines
