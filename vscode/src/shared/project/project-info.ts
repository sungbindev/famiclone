// ------------------------------------------------------------------------
// 名称：project-info.ts
// 说明：fcdev 项目信息的数据类型，以及从 `fcdev info --json` 的输出里解析它的纯函数。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-10
// 备注：解析时只取用到的字段并检查类型，输出格式不合法时返回 undefined。
// ------------------------------------------------------------------------

/** 一个 fcdev 项目的信息。 */
export interface ProjectInfo {
  /** 项目根目录，即 fcdev.json 所在目录。 */
  readonly root: string;
  /** 目标名，如 nes、sb2000。 */
  readonly targetName: string;
  /** C 头文件目录。 */
  readonly includeDirs: readonly string[];
  /** 汇编引入目录，`.include` 按这些目录查找文件。 */
  readonly asmIncludeDirs: readonly string[];
}

/** 判断值是不是字符串数组。 */
function isStringArray(value: unknown): value is string[] {
  return Array.isArray(value) && value.every((item) => typeof item === 'string');
}

/**
 * 解析 `fcdev info --json` 的输出。
 * @param output 命令的标准输出。
 * @returns 项目信息；不是合法的 JSON 或缺少字段时为 undefined。
 */
export function parseProjectInfo(output: string): ProjectInfo | undefined {
  let data: unknown;
  try {
    data = JSON.parse(output);
  } catch {
    return undefined;
  }
  if (typeof data !== 'object' || data === null) return undefined;
  const record = data as Record<string, unknown>;
  const target = record['target'];
  const targetName = typeof target === 'object' && target !== null ? (target as Record<string, unknown>)['name'] : undefined;
  const root = record['root'];
  const includeDirs = record['includeDirs'];
  const asmIncludeDirs = record['asmIncludeDirs'];
  if (typeof root !== 'string' || typeof targetName !== 'string' || !isStringArray(includeDirs) || !isStringArray(asmIncludeDirs)) {
    return undefined;
  }
  return { root, targetName, includeDirs, asmIncludeDirs };
}
