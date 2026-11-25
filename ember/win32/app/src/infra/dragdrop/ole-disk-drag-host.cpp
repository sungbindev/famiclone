/*
 * ------------------------------------------------------------------------
 * 名称：ole-disk-drag-host.cpp
 * 说明：用 OLE 拖放实现的磁盘镜像条目拖出的实现。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-25
 * 备注：无。
 * ------------------------------------------------------------------------
 */

#include "infra/dragdrop/ole-disk-drag-host.hpp"

#include "platform/dragdrop/file-drag.hpp"

namespace ember::win32::infra {

void OleDiskDragHost::startDrag(const std::vector<contracts::DiskExportItem>& items, std::uint64_t token) {
  std::vector<platform::DragItem> dragged;
  dragged.reserve(items.size());
  for (const contracts::DiskExportItem& item : items) dragged.push_back(platform::DragItem{item.path, item.directory, item.content});
  platform::startFileDrag(dragged, token);
}

}  // namespace ember::win32::infra
