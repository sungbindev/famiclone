// ------------------------------------------------------------------------
// 名称：result-text.ts
// 说明：把 fcdev 操作的结果写成给模型看的文字，并限制长度。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-21
// 备注：结果里的 text（清单、十六进制视图、调用图）与 listing 字段单独成块原样输出，
//       其余字段写成缩进的 JSON；
//       超过上限时截断并说明怎么缩小范围，截断处不拆开字符。
// ------------------------------------------------------------------------

/** 单次结果的最大字符数。 */
export const MAX_RESULT_CHARS = 20000;

/** 原样输出的多行文字字段。 */
const TEXT_FIELDS = ['text', 'listing'] as const;

/**
 * 把结果写成文字。
 * @param result fcdev 操作的结果。
 * @param maxChars 最大字符数，默认 MAX_RESULT_CHARS。
 * @returns 文字；超长时已截断。
 */
export function formatResult(result: unknown, maxChars: number = MAX_RESULT_CHARS): string {
  const blocks: string[] = [];
  if (typeof result === 'object' && result !== null && !Array.isArray(result)) {
    const rest: Record<string, unknown> = { ...(result as Record<string, unknown>) };
    const texts: string[] = [];
    for (const field of TEXT_FIELDS) {
      const value = rest[field];
      if (typeof value === 'string') {
        texts.push(value);
        delete rest[field];
      }
    }
    if (Object.keys(rest).length > 0 || texts.length === 0) blocks.push(JSON.stringify(rest, null, 2));
    blocks.push(...texts);
  } else {
    blocks.push(JSON.stringify(result, null, 2) ?? String(result));
  }
  return truncate(blocks.join('\n\n'), maxChars);
}

/**
 * 截断过长的文字。
 * @param text 文字。
 * @param maxChars 最大字符数。
 * @returns 不超长时原样返回；超长时保留开头并在末尾说明。
 */
export function truncate(text: string, maxChars: number): string {
  if (text.length <= maxChars) return text;
  let end = maxChars;
  const last = text.charCodeAt(end - 1);
  if (last >= 0xd800 && last <= 0xdbff) end -= 1;
  return `${text.slice(0, end)}\n\n…（输出共 ${text.length} 个字符，已截断；请缩小地址范围，或用 limit 与 cursor 分页）`;
}
