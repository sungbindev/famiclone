// ------------------------------------------------------------------------
// 名称：link-config.ts
// 说明：由布局和片生成 ld65 的链接配置：每个布局段一个存储区，段里的片按地址顺序摆放。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-23
// 备注：换页窗口里同一个 CPU 地址的多个 bank 各有自己的存储区，互不冲突；存储区的大小恰好是段的长
//       度。各段按文件偏移排序后在映像里首尾相接时，所有存储区都写到同一个输出文件（%O），按偏移
//       顺序拼接，产物就是映像本身，可以直接用 fcdev 的 rom 目标构建；否则每段一个输出文件
//       （seg<段号>.bin），逐字节对应映像里的这一段。
// ------------------------------------------------------------------------
import type { Layout, Segment } from '../model/layout';
import type { Project } from '../model/project';
import type { Piece } from './pieces';

/** 段拼成一个输出文件时，回读用的输出文件名（ld65 的 -o 参数）。 */
export const JOINED_OUTPUT_NAME = 'rom.bin';

/**
 * 段是否在映像里首尾相接：按文件偏移排序后，每一段紧接着上一段的末尾（第一段可以不从 0 开始，如
 * 跳过 iNES 文件头）。
 * @param layout 布局
 * @returns 首尾相接时为 true
 */
export function isJoinedLayout(layout: Layout): boolean {
  const sorted = layout.segments.toSorted((a, b) => a.fileOffset - b.fileOffset);
  return sorted.every((segment, index) => index === 0 || segment.fileOffset === sorted[index - 1]!.fileOffset + sorted[index - 1]!.length);
}

/**
 * 段的输出文件与段在文件里的起始位置。
 * @param layout 布局
 * @param segment 段
 * @returns 输出文件名（拼成一个文件时是回读用的名字）与起始位置
 */
export function segmentOutput(layout: Layout, segment: Segment): {file: string, offset: number} {
  if (!isJoinedLayout(layout)) return { file: `seg${segment.id}.bin`, offset: 0 };
  const first = Math.min(...layout.segments.map((item) => item.fileOffset));
  return { file: JOINED_OUTPUT_NAME, offset: segment.fileOffset - first };
}

/**
 * 数值写成 $ 开头的十六进制。
 * @param value 数值
 * @returns 如 $8000
 */
function hex(value: number): string {
  return `$${value.toString(16).toUpperCase()}`;
}

/**
 * 生成链接配置。
 * @param project 工程
 * @param pieces 片，按段号与偏移排序
 * @returns 配置内容
 */
export function renderLinkConfig(project: Project, pieces: Piece[]): string {
  const memory: string[] = [];
  const segments: string[] = [];
  const joined = isJoinedLayout(project.layout);
  const ordered = joined ? project.layout.segments.toSorted((a, b) => a.fileOffset - b.fileOffset) : project.layout.segments;
  for (const segment of ordered) {
    const bank = segment.bank === null ? '' : `，bank ${segment.bank}`;
    const file = joined ? '%O' : `"${segmentOutput(project.layout, segment).file}"`;
    memory.push(`  # 段 ${segment.id}${bank}：映像文件偏移 ${hex(segment.fileOffset)} 起，${segment.length} 字节${segment.writable ? '，运行时可写' : ''}`);
    memory.push(`  SEG${segment.id}: file = ${file}, start = ${hex(segment.address)}, size = ${hex(segment.length)}, fill = yes;`);
    for (const piece of pieces.filter((item) => item.segmentId === segment.id)) {
      segments.push(`  ${piece.name}: load = SEG${segment.id}, type = ro;`);
    }
  }
  const note = joined ? '\n# 各段按映像文件偏移顺序首尾相接，写到同一个输出文件，产物就是映像本身。' : '';
  return `# 链接配置：由 fcdev 的反汇编工程按布局生成，不要手改。${note}\nMEMORY {\n${memory.join('\n')}\n}\nSEGMENTS {\n${segments.join('\n')}\n}\n`;
}
