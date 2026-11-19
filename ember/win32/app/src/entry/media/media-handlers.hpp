/*
 * ------------------------------------------------------------------------
 * 名称：media-handlers.hpp
 * 说明：media 模块里外壳自己处理的请求：openFloppy（载入软盘镜像）、openTape（载入磁带）、openTapeRecording（选择录音文件并开始录音）。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-19
 * 备注：对应 Electron 外壳的 entry/ipc/media；给出 droppedId 时不弹窗口，用拖入窗口的那个文件（按扩展名核对）；用户取消选择时结果的 canceled 为 true，不是错误。
 * ------------------------------------------------------------------------
 */

#pragma once

#include "contracts/core-gateway/core-gateway.hpp"
#include "contracts/shell/dropped-files.hpp"
#include "contracts/shell/file-dialog.hpp"
#include "entry/main-requests/main-request-handler.hpp"

namespace ember::win32::entry {

/** media 请求处理所需的依赖。 */
struct MediaDependencies {
  contracts::CoreGateway& core;
  contracts::FileDialog& dialog;
  contracts::DroppedFilesService& dropped;
};

/** 创建 media 模块的处理函数。 */
[[nodiscard]] MainRequestHandlers createMediaHandlers(MediaDependencies dependencies);

}  // namespace ember::win32::entry
