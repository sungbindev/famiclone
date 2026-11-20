// ------------------------------------------------------------------------
// 名称：code-scan.mjs
// 说明：按工程里的 code 区域逐条解码指令，得到“哪些地址是指令的开头”，供自动命名与源码生成使用。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-21
// 备注：每个 code 区域从它的第一个字节开始顺序解码，遇到不能解码的字节（非官方操作码、超出区域末尾）就停止，剩下的字节当作数据处理。
// ------------------------------------------------------------------------
import { decode } from '../cpu/index.mjs';
import { locateOffset } from '../model/index.mjs';

/**
 * @typedef {object} ScannedInstruction
 * @property {number} offset 文件偏移
 * @property {number} segmentId 所在的段
 * @property {number} address CPU 地址
 * @property {number} size 长度
 * @property {import('../cpu/decode.mjs').DecodedInstruction} instruction 解码结果
 */

/**
 * @typedef {object} CodeScan
 * @property {ScannedInstruction[]} list 全部指令，按文件偏移排序
 * @property {Map<number, ScannedInstruction>} byOffset 文件偏移到以它开头的指令
 * @property {(offset: number) => ScannedInstruction|null} containing 包含某个偏移的指令；不在任何指令里时为 null
 */

/**
 * 扫描工程的 code 区域。
 * @param {import('../model/project.mjs').Project} project 工程
 * @param {Uint8Array} bytes 映像的全部字节
 * @returns {CodeScan} 扫描结果
 */
export function scanCode(project, bytes) {
  const list = [];
  for (const region of project.regions.regions) {
    if (region.type !== 'code') continue;
    const { segment } = locateOffset(project.layout, region.offset);
    const end = region.offset + region.length;
    let offset = region.offset;
    while (offset < end) {
      const address = segment.address + (offset - segment.fileOffset);
      const instruction = decode(bytes, offset, address);
      if (!instruction.ok || offset + instruction.size > end) break;
      list.push({ offset, segmentId: segment.id, address, size: instruction.size, instruction });
      offset += instruction.size;
    }
  }
  const byOffset = new Map(list.map((item) => [item.offset, item]));
  const containing = (offset) => {
    let low = 0;
    let high = list.length - 1;
    while (low <= high) {
      const middle = (low + high) >> 1;
      const item = list[middle];
      if (offset < item.offset) high = middle - 1;
      else if (offset >= item.offset + item.size) low = middle + 1;
      else return item;
    }
    return null;
  };
  return { list, byOffset, containing };
}
