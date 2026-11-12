// ------------------------------------------------------------------------
// 名称：cpp-properties.ts
// 说明：C/C++ 扩展的配置文件 c_cpp_properties.json 的解析：取出各个配置的名字。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-11
// 备注：文件由 fcdev sync 改写，内容是标准 JSON；解析失败或格式不符时返回空数组。
// ------------------------------------------------------------------------

/** fcdev sync 写入的配置名，与 fcdev 命令行保持一致。 */
export const FCDEV_CONFIGURATION_NAME = 'fcdev';

/**
 * 取出配置文件里每个配置的名字。
 * @param text c_cpp_properties.json 的文本。
 * @returns 配置名，按文件里的顺序；解析失败或没有 configurations 数组时为空数组。
 */
export function parseConfigurationNames(text: string): string[] {
  let document: unknown;
  try {
    document = JSON.parse(text);
  } catch {
    return [];
  }
  const configurations = (document as { configurations?: unknown } | null)?.configurations;
  if (!Array.isArray(configurations)) return [];
  return configurations.flatMap((item: unknown) => {
    const name = (item as { name?: unknown } | null)?.name;
    return typeof name === 'string' ? [name] : [];
  });
}
