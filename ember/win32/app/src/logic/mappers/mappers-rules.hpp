/*
 * ------------------------------------------------------------------------
 * 名称：mappers-rules.hpp
 * 说明：mapper 插件对话框的纯规则：从核心回复读出插件摘要与清单详情，来源与主机型号的显示名、匹配条件的文字、按搜索词与来源筛选（忽略空白）、插件与 mapper 数量的统计、清单摘要的各项文字，错误说明的读取。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-21
 * 备注：不依赖界面与请求，便于单独测试；文字规则见 docs/05-界面.md 第 4.4 节。
 * ------------------------------------------------------------------------
 */

#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

#include <nlohmann/json.hpp>

#include "contracts/core-gateway/core-gateway.hpp"
#include "contracts/mappers/mappers-model.hpp"

namespace ember::win32::logic {

/** 一项匹配条件。 */
struct MapperRule {
  std::int64_t mapper{0};
  std::optional<std::int64_t> variant;
  std::optional<std::int64_t> prgSize;
  std::optional<std::int64_t> chrSize;
  std::optional<std::string> crc32Prg;
  std::optional<std::string> crc32All;
};

/** 插件摘要（mappers.list 的一项）。 */
struct MapperSummary {
  std::string id;
  std::string name;
  std::string version;
  std::string publisher;
  std::string description;
  std::vector<std::string> host;
  std::vector<std::string> devices;
  std::vector<MapperRule> match;
  /** bundled 或 user。 */
  std::string source;
  bool enabled{false};
};

/** 清单里的一个存储区。 */
struct MapperStorage {
  std::string id;
  /** rom 或 ram。 */
  std::string kind;
  /** rom 的数据来源：file.prg 或 file.chr。 */
  std::string from;
  std::int64_t size{0};
  bool battery{false};
};

/** 清单里的一段寄存器范围。 */
struct MapperRegister {
  std::string bus;
  std::string range;
  bool read{false};
  bool write{false};
};

/** 清单里声明的一项脚本状态。 */
struct MapperState {
  std::string name;
  std::int64_t bits{0};
  std::string description;
};

/** 插件详情（mappers.info 的结果）。 */
struct MapperDetail {
  MapperSummary summary;
  std::vector<MapperStorage> storage;
  std::vector<MapperRegister> registers;
  std::vector<MapperState> state;
};

/** 从 mappers.list 的结果读出全部插件；格式不对时没有值，格式不对的单项跳过。 */
[[nodiscard]] std::optional<std::vector<MapperSummary>> parseMapperList(const nlohmann::json& result);
/** 从一个摘要的 JSON 读出插件；格式不对时没有值。 */
[[nodiscard]] std::optional<MapperSummary> parseMapperSummary(const nlohmann::json& value);
/** 从 mappers.info 的结果读出插件详情；格式不对时没有值。 */
[[nodiscard]] std::optional<MapperDetail> parseMapperDetail(const nlohmann::json& result);

/** 来源的显示名。 */
[[nodiscard]] std::string sourceLabel(const std::string& source);
/** 主机型号的显示名；不认识的型号显示标识。 */
[[nodiscard]] std::string hostLabel(const std::string& host);
/** 一项匹配条件的文字：mapper 号加写出的其他条件。 */
[[nodiscard]] std::string ruleText(const MapperRule& rule);
/** 全部匹配条件在表格里的一行文字。 */
[[nodiscard]] std::string matchesText(const std::vector<MapperRule>& rules);
/** 主机型号在表格里的一行文字。 */
[[nodiscard]] std::string hostsText(const std::vector<std::string>& hosts);

/**
 * 按来源与搜索词筛选插件：搜索词不分大小写、忽略全部空白，匹配名称、标识、发布者与说明。
 * @param source all、bundled 或 user。
 */
[[nodiscard]] std::vector<MapperSummary> filterMappers(const std::vector<MapperSummary>& plugins, const std::string& source, const std::string& search);
/** 统计在工具栏里显示的一行文字：全部已安装的插件，不受筛选影响。 */
[[nodiscard]] std::string statsText(const std::vector<MapperSummary>& plugins);
/** 表格里的一行。 */
[[nodiscard]] contracts::MapperRow toRow(const MapperSummary& plugin);
/** 清单摘要的各项，没有内容的项不列出。 */
[[nodiscard]] std::vector<contracts::MapperFact> detailFacts(const MapperDetail& detail);

/** 从请求失败的原因里取出给用户看的说明；没有说明时为 fallback。 */
[[nodiscard]] std::string mapperErrorText(const contracts::CoreError& error, const std::string& fallback);

}  // namespace ember::win32::logic
