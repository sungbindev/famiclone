// ------------------------------------------------------------------------
// 名称：flat.mjs
// 说明：平铺二进制的解析器：整个文件放在一个 CPU 地址起点上，或由调用者给出完整的布局。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-21
// 备注：平铺文件没有中断向量等自带信息，入口由调用者在工程里登记；给出的布局没有覆盖到的字节记入 unmapped。
// ------------------------------------------------------------------------
import { FcdevError } from '../../shared/index.mjs';
import { createLayout, findUncovered } from '../model/index.mjs';

/**
 * @typedef {object} FlatOptions
 * @property {number} [address] 第一个字节的 CPU 地址，整个文件放成一个段；没有 segments 时必须给出
 * @property {Array<Partial<import('../model/layout.mjs').Segment>>} [segments] 完整的段列表；给出时忽略 address
 */

/**
 * 解析平铺二进制。
 * @param {Buffer} buffer 文件内容
 * @param {FlatOptions} options 加载地址或段列表
 * @returns {import('./index.mjs').ParsedImage} 解析结果
 * @throws {FcdevError} 文件为空、没有给出地址与段、布局无效
 */
export function parseFlat(buffer, options = {}) {
  if (buffer.length === 0) throw new FcdevError('输入文件是空的', { code: 'DISASM_INPUT_INVALID' });
  const { address, segments } = options;
  if (segments === undefined && address === undefined) {
    throw new FcdevError('平铺二进制要给出加载地址或段列表', { code: 'DISASM_INPUT_INVALID', hint: '加载地址如 0x8000；有换页时给出每个段的文件偏移、长度、地址和 bank' });
  }
  const layout = createLayout(segments ?? [{ id: 0, fileOffset: 0, length: buffer.length, address }], { imageSize: buffer.length });
  return {
    format: 'flat',
    layout,
    vectors: null,
    unmapped: findUncovered(layout).map((range) => ({ ...range, description: '没有划入任何段' })),
    description: segments === undefined ? `平铺二进制，加载地址 $${address.toString(16).toUpperCase()}` : `平铺二进制，${layout.segments.length} 个段`,
  };
}
