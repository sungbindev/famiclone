/*
 * ------------------------------------------------------------------------
 * 名称：help-request.hpp
 * 说明：帮助文档站点的请求规则：把请求目标（路径加查询）整理成站点里的相对路径，判断各文件的内容类型，给出站点页面用的安全响应头。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-21
 * 备注：站点是只读的静态页面，请求地址一律不可信：解码失败、含反斜杠、冒号、空字符、上级或当前目录段的地址都按“不存在”处理；
 *       以 / 结尾的地址读该目录的 index.html。
 * ------------------------------------------------------------------------
 */

#pragma once

#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace ember::win32::shared {

/** 可以直接打开的页面名对应的站点路径（相对站点根目录）；不认识的页面名没有值。 */
[[nodiscard]] std::optional<std::string> helpPagePath(std::string_view page);

/**
 * 把请求目标整理成站点里的相对路径（用 / 分隔，没有开头的 /）。
 * @param target 请求行里的目标，如 /manual/changelog.html?x=1。
 * @returns 合格时是相对路径；地址不合格时没有值。
 */
[[nodiscard]] std::optional<std::string> normalizeHelpPath(std::string_view target);

/** 按扩展名给出内容类型；不认识的扩展名是 application/octet-stream。 */
[[nodiscard]] std::string helpContentType(std::string_view path);

/** 站点页面的响应头：禁止猜测类型与内容安全策略（只允许本站的资源，内联脚本与样式仅限站点生成器的输出）。 */
[[nodiscard]] std::vector<std::pair<std::string, std::string>> helpSecurityHeaders();

}  // namespace ember::win32::shared
