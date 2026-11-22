// ------------------------------------------------------------------------
// 名称：pieces.ts
// 说明：把布局里的段按模块切成“片”：每片是一个段里属于同一个模块的连续字节，对应源码里的一个 ca65
//       段。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-21
// 备注：没有归入任何模块的字节属于默认模块 main；同一个模块相邻的范围合并成一片。片按文件偏移排列、
//       首尾相接、盖住整个段，链接配置据此把各片按地址顺序摆进段的存储区。ca65 的分支不能跨段，所以
//       片的边界也是分支能到达的边界。
// ------------------------------------------------------------------------
import type { Project } from '../model/project';

/** 没有归入任何模块的字节所在的模块 */
export const DEFAULT_MODULE = 'main';

export interface Piece {
  /** ca65 段名，如 SEG0_0010（段号加片在段内的偏移） */
  name: string;
  /** 所在的段 */
  segmentId: number;
  /** 所属模块 */
  module: string;
  /** 第一个字节的文件偏移 */
  start: number;
  /** 最后一个字节之后的文件偏移 */
  end: number;
}

/**
 * 把全部段切成片。
 * @param project 工程
 * @returns 片，按段号与偏移排序
 */
export function buildPieces(project: Project): Piece[] {
  const pieces: Piece[] = [];
  for (const segment of project.layout.segments) {
    const segmentEnd = segment.fileOffset + segment.length;
    const ranges: Array<{module: string, start: number, end: number}> = [];
    for (const module of project.modules) {
      for (const range of module.ranges) {
        if (range.segmentId !== segment.id) continue;
        const start = segment.fileOffset + (range.address - segment.address);
        ranges.push({ module: module.name, start, end: start + range.length });
      }
    }
    const cuts = new Set([segment.fileOffset, segmentEnd]);
    for (const range of ranges) {
      cuts.add(range.start);
      cuts.add(range.end);
    }
    const sorted = [...cuts].toSorted((a, b) => a - b);
    for (let index = 0; index + 1 < sorted.length; index += 1) {
      const start = sorted[index]!;
      const end = sorted[index + 1]!;
      const module = ranges.find((range) => range.start <= start && start < range.end)?.module ?? DEFAULT_MODULE;
      const last = pieces.at(-1);
      if (last !== undefined && last.segmentId === segment.id && last.module === module && last.end === start) {
        last.end = end;
      } else {
        pieces.push({ name: `SEG${segment.id}_${(start - segment.fileOffset).toString(16).toUpperCase().padStart(4, '0')}`, segmentId: segment.id, module, start, end });
      }
    }
  }
  return pieces;
}
