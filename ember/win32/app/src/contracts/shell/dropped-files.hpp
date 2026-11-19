/*
 * ------------------------------------------------------------------------
 * 名称：dropped-files.hpp
 * 说明：拖入窗口的文件服务：路径只记在内存里，界面只能拿到编号与显示名；主进程请求按编号取回路径。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-19
 * 备注：编号从 1 递增，只在本次运行内有效。
 * ------------------------------------------------------------------------
 */

#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace ember::win32::contracts {

/** 登记后的一个拖入文件：编号与显示名。 */
struct DroppedFile {
  std::int64_t id{0};
  std::string name;
};

/** 拖入窗口的文件服务。 */
class DroppedFilesService {
 public:
  virtual ~DroppedFilesService() = default;

  /** 登记拖入的文件（UTF-8 路径），返回各文件的编号与显示名。 */
  [[nodiscard]] virtual std::vector<DroppedFile> registerFiles(const std::vector<std::string>& paths) = 0;

  /** 按编号取路径；编号没登记过或已失效时为空。 */
  [[nodiscard]] virtual std::optional<std::string> resolve(std::int64_t id) const = 0;
};

}  // namespace ember::win32::contracts
