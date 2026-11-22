// ------------------------------------------------------------------------
// 名称：code-scan.ts
// 说明：按工程里的 code 区域逐条解码指令，得到“哪些地址是指令的开头”，供自动命名与源码生成使用。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-21
// 备注：每个 code 区域从它的第一个字节开始顺序解码，遇到不能解码的字节（非官方操作码、超出区域末
//       尾）就停止，剩下的字节当作数据处理。
// ------------------------------------------------------------------------
import { decode } from '../cpu/index';
import { locateOffset } from '../model/index';
import type { DecodedInstruction } from '../cpu/decode';
import type { Project } from '../model/project';

export interface ScannedInstruction {
  /** 文件偏移 */
  offset: number;
  /** 所在的段 */
  segmentId: number;
  /** CPU 地址 */
  address: number;
  /** 长度 */
  size: number;
  /** 解码结果 */
  instruction: DecodedInstruction;
}

export interface CodeScan {
  /** 全部指令，按文件偏移排序 */
  list: ScannedInstruction[];
  /** 文件偏移到以它开头的指令 */
  byOffset: Map<number, ScannedInstruction>;
  /** 包含某个偏移的指令；不在任何指令里时为 null */
  containing: (offset: number) => ScannedInstruction|null;
}

/**
 * 扫描工程的 code 区域。
 * @param project 工程
 * @param bytes 映像的全部字节
 * @returns 扫描结果
 */
export function scanCode(project: Project, bytes: Uint8Array): CodeScan {
  const list: ScannedInstruction[] = [];
  for (const region of project.regions.regions) {
    if (region.type !== 'code') continue;
    const { segment } = locateOffset(project.layout, region.offset)!;
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
  const containing = (offset: number): ScannedInstruction | null => {
    let low = 0;
    let high = list.length - 1;
    while (low <= high) {
      const middle = (low + high) >> 1;
      const item = list[middle]!;
      if (offset < item.offset) high = middle - 1;
      else if (offset >= item.offset + item.size) low = middle + 1;
      else return item;
    }
    return null;
  };
  return { list, byOffset, containing };
}
