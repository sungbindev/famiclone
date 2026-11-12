// ------------------------------------------------------------------------
// 名称：dependency-file.mjs
// 说明：解析 cc65、ca65 用 --create-dep 生成的依赖文件（make 格式）。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-10
// 备注：路径里的空格用反斜杠转义；Windows 路径里的反斜杠分隔符保持原样。
// ------------------------------------------------------------------------

/**
 * 取出依赖文件里第一条规则的依赖项。
 * @param {string} text 依赖文件内容
 * @returns {string[]} 依赖文件路径；没有规则时为空数组
 */
export function parseDependencyFile(text) {
  const joined = text.replace(/\\\r?\n/g, ' ');
  const firstRule = joined.split(/\r?\n/).find((line) => line.trim() !== '');
  if (!firstRule) return [];
  // 规则的冒号后面一定是空白；盘符的冒号后面是路径分隔符，不会被误判
  const separator = firstRule.search(/:(\s|$)/);
  if (separator < 0) return [];
  const dependencies = firstRule.slice(separator + 1).match(/(?:\\ |\S)+/g) ?? [];
  return dependencies.map((item) => item.replaceAll('\\ ', ' '));
}
