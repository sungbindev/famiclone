/*
 * ------------------------------------------------------------------------
 * 名称：filemanager-rules.hpp
 * 说明：媒体库的规则：读取核心的目录树、镜像里文件列表、空白镜像种类与导出内容，把大小与格式转成界面要显示的文字，以及自动取新建文件夹的名称。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-25
 * 备注：读取时逐项检查，内容不合格就整体返回空值；纯函数，不访问核心与界面。
 * ------------------------------------------------------------------------
 */

#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

#include <nlohmann/json.hpp>

#include "contracts/filemanager/filemanager-model.hpp"

namespace ember::win32::logic {

/** 镜像文件大小的文字：1.44 MB、720 KB。 */
[[nodiscard]] std::string imageSizeText(std::uint64_t bytes);

/** 文件大小的文字：带千位分隔，占用空间而不是准确字节数时前面加“≈”。 */
[[nodiscard]] std::string entrySizeText(std::uint64_t bytes, bool allocated);

/** 树里一个镜像行尾的说明：文件系统格式与大小；还在识别时是“识别中…”。 */
[[nodiscard]] std::string treeDetailText(bool scanned, const std::string& formatName, std::uint64_t size);

/** filemanager.tree 的结果。 */
struct ParsedTree {
  std::string root;
  bool available{false};
  bool scanning{false};
  bool limited{false};
  std::vector<contracts::DiskTreeNode> nodes;
};

/** 解析 filemanager.tree 的结果；内容不合格时为空。 */
[[nodiscard]] std::optional<ParsedTree> parseTree(const nlohmann::json& result);

/** filemanager.list 的结果（只含镜像里文件列表与文件系统的部分，选中的镜像与文件夹由控制器填）。 */
struct ParsedList {
  std::string format;
  std::string formatName;
  bool writable{false};
  bool subdirectories{false};
  int drive{-1};
  std::string readOnlyReason;
  int undoDepth{0};
  std::vector<contracts::DiskEntryRow> entries;
};

/** 解析 filemanager.list 的结果；内容不合格时为空。 */
[[nodiscard]] std::optional<ParsedList> parseList(const nlohmann::json& result);

/** 解析 filemanager.blankKinds 的结果；内容不合格时为空。 */
[[nodiscard]] std::optional<std::vector<contracts::DiskBlankKind>> parseBlankKinds(const nlohmann::json& result);

/** 解析 filemanager.readEntries 的结果（Base64 解码文件内容）；内容不合格时为空。 */
[[nodiscard]] std::optional<std::vector<contracts::DiskExportItem>> parseExport(const nlohmann::json& result);

/** 核心错误给用户看的文字：核心已经给了说明就用它，否则用错误码。 */
[[nodiscard]] std::string diskErrorText(const std::string& code, const std::string& message);

/** 新建文件夹的名称：基础名不重名就用基础名，否则在后面加序号。 */
[[nodiscard]] std::string uniqueFolderName(const std::vector<contracts::DiskEntryRow>& entries);

}  // namespace ember::win32::logic
