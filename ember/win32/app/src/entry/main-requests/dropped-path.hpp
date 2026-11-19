/*
 * ------------------------------------------------------------------------
 * 名称：dropped-path.hpp
 * 说明：按编号取拖入的文件路径：编号没有登记过、已失效或扩展名不是该请求接受的类型时，返回空值，由请求以自己的“找不到拖入的文件”错误拒绝。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-19
 * 备注：编号对应的路径由外壳在拖放时登记，界面不能指定路径；这里再按扩展名核对一次，界面传来的编号配错请求时不会把别的文件交给核心。
 * ------------------------------------------------------------------------
 */

#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

#include "contracts/shell/dropped-files.hpp"

namespace ember::win32::entry {

/**
 * 按编号取拖入的文件路径。
 * @param files 拖入的文件服务。
 * @param id 编号。
 * @param extensions 接受的扩展名，小写、不带点。
 * @returns 文件路径；编号无效或扩展名不符时为空。
 */
[[nodiscard]] std::optional<std::string> resolveDroppedPath(const contracts::DroppedFilesService& files, std::int64_t id, const std::vector<std::string>& extensions);

}  // namespace ember::win32::entry
