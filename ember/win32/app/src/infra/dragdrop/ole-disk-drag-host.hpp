/*
 * ------------------------------------------------------------------------
 * 名称：ole-disk-drag-host.hpp
 * 说明：把磁盘镜像里的条目拖到 Windows 里的能力的实现：用系统的 OLE 拖放，把读出的文件内容作为虚拟文件交给资源管理器等拖放目标。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-25
 * 备注：无。
 * ------------------------------------------------------------------------
 */

#pragma once

#include "contracts/diskimages/diskimages-model.hpp"

namespace ember::win32::infra {

/** 用 OLE 拖放实现的磁盘镜像条目拖出。 */
class OleDiskDragHost final : public contracts::DiskImagesDragHost {
 public:
  void startDrag(const std::vector<contracts::DiskExportItem>& items, std::uint64_t token) override;
};

}  // namespace ember::win32::infra
