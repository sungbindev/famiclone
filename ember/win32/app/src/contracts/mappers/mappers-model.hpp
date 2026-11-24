/*
 * ------------------------------------------------------------------------
 * 名称：mappers-model.hpp
 * 说明：mapper 插件模块给界面层的模型：对话框里要显示的统计、插件表格各行、选中插件的清单摘要，以及用户的操作（搜索、筛选来源、选中、安装、卸载、启用与停用）。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-21
 * 备注：文字与可用状态由逻辑层算好，界面只显示；操作的结果与错误用外壳通知给出，模型不显示结果；规则见 docs/05-界面.md 第 4.4 节。
 * ------------------------------------------------------------------------
 */

#pragma once

#include <functional>
#include <string>
#include <vector>

namespace ember::win32::contracts {

/** 数据的读取状态。 */
enum class MappersLoad {
  /** 正在读取（核心还没有就绪或核心正在重启）。 */
  Loading,
  Ready,
  /** 读取失败，说明在 message 里。 */
  Failed
};

/** 选中插件的清单详情的读取状态。 */
enum class MapperDetailLoad {
  /** 没有选中插件。 */
  None,
  Loading,
  Ready,
  Failed
};

/** 来源筛选的取值。 */
inline constexpr const char* kMapperSourceAll = "all";
inline constexpr const char* kMapperSourceBundled = "bundled";
inline constexpr const char* kMapperSourceUser = "user";
inline constexpr const char* kMapperSourceDevelopment = "development";

/** 表格里的一行插件。 */
struct MapperRow {
  std::string id;
  std::string name;
  /** 名称下面的说明，没有时为空。 */
  std::string description;
  std::string version;
  /** 匹配条件的一行文字。 */
  std::string match;
  /** 主机型号的一行文字。 */
  std::string hosts;
  /** 来源的显示名：预装、用户安装、开发目录。 */
  std::string source;
  bool enabled{false};
  /** 预装插件：只能停用，不能卸载。 */
  bool bundled{false};
  /** 开发目录里的插件：只能停用，不能卸载，要从开发目录里删除它的目录。 */
  bool development{false};
  bool operator==(const MapperRow&) const = default;
};

/** 清单摘要里的一项：标签与若干行文字。 */
struct MapperFact {
  std::string label;
  std::vector<std::string> lines;
  bool operator==(const MapperFact&) const = default;
};

/** mapper 插件对话框此刻要显示的内容。 */
struct MappersView {
  MappersLoad load{MappersLoad::Loading};
  /** 读取失败的说明。 */
  std::string message;
  /** 统计文字（“共 N 个插件（已启用 K 个），支持 M 种 mapper”）；还没读到时为空。 */
  std::string stats;
  /** 按搜索词与来源筛选后的插件，按标识排序。 */
  std::vector<MapperRow> rows;
  /** 当前的搜索词与来源筛选。 */
  std::string search;
  std::string source{kMapperSourceAll};
  /** 选中的插件标识；没有选中或插件已不在列表里时为空。 */
  std::string selectedId;
  /** 选中的插件（不受筛选影响，筛选掉了仍能看到它的详情）。 */
  MapperRow selected;
  MapperDetailLoad detailLoad{MapperDetailLoad::None};
  /** 清单详情读取失败的说明。 */
  std::string detailMessage;
  std::vector<MapperFact> facts;
  bool operator==(const MappersView&) const = default;
};

/** mapper 插件对话框的模型。 */
class MappersModel {
 public:
  virtual ~MappersModel() = default;

  [[nodiscard]] virtual MappersView view() const = 0;

  /** 设置搜索词（按名称、标识、发布者、说明筛选，不分大小写、忽略全部空白）。 */
  virtual void setSearch(const std::string& text) = 0;
  /** 设置来源筛选：all、bundled、user、development。 */
  virtual void setSource(const std::string& source) = 0;
  /** 选中或取消选中（空标识表示取消）；已选中的再选一次就是取消。 */
  virtual void toggleSelect(const std::string& id) = 0;
  /** 弹出系统文件选择窗口安装插件包；成功后新插件被选中，结果与错误用通知给出，用户取消时什么都不发生。 */
  virtual void install() = 0;
  /** 询问确认后卸载用户安装的插件。 */
  virtual void uninstall(const std::string& id) = 0;
  /** 启用或停用；成功没有通知，失败有。 */
  virtual void setEnabled(const std::string& id, bool enabled) = 0;

  /** 订阅内容变化；返回取消订阅的函数。 */
  [[nodiscard]] virtual std::function<void()> subscribe(std::function<void()> handler) = 0;
};

}  // namespace ember::win32::contracts
