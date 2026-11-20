// ------------------------------------------------------------------------
// 名称：image-compare.ts
// 说明：比较两段字节，找出第一处不同的位置。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-21
// 备注：长度不同时，较短的一方结束的位置也算不同。
// ------------------------------------------------------------------------

/**
 * 找出第一个不同的位置。
 * @param expected 原映像。
 * @param actual 构建出的字节。
 * @returns 第一个不同的偏移；完全相同时为 null。
 */
export function firstDifference(expected: Uint8Array, actual: Uint8Array): number | null {
  const common = Math.min(expected.length, actual.length);
  for (let offset = 0; offset < common; offset += 1) {
    if (expected[offset] !== actual[offset]) return offset;
  }
  return expected.length === actual.length ? null : common;
}
