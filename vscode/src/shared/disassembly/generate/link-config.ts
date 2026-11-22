// ------------------------------------------------------------------------
// 名称：link-config.ts
// 说明：由布局和片生成 ld65 的链接配置：每个布局段一个存储区、一个输出文件，段里的片按地址顺序摆
//       放。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-21
// 备注：换页窗口里同一个 CPU 地址的多个 bank 各有自己的存储区与输出文件（seg<段号>.bin），互不冲
//       突；存储区的大小恰好是段的长度，输出文件逐字节对应映像里的这一段。
// ------------------------------------------------------------------------
import type { Project } from '../model/project';
import type { Piece } from './pieces';

/**
 * 段对应的输出文件名。
 * @param segmentId 段号
 * @returns 文件名
 */
export function segmentOutputName(segmentId: number): string {
  return `seg${segmentId}.bin`;
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
  for (const segment of project.layout.segments) {
    const bank = segment.bank === null ? '' : `，bank ${segment.bank}`;
    memory.push(`  # 段 ${segment.id}${bank}：映像文件偏移 ${hex(segment.fileOffset)} 起，${segment.length} 字节${segment.writable ? '，运行时可写' : ''}`);
    memory.push(`  SEG${segment.id}: file = "${segmentOutputName(segment.id)}", start = ${hex(segment.address)}, size = ${hex(segment.length)}, fill = yes;`);
    for (const piece of pieces.filter((item) => item.segmentId === segment.id)) {
      segments.push(`  ${piece.name}: load = SEG${segment.id}, type = ro;`);
    }
  }
  return `# 链接配置：由 fcdev 的反汇编工程按布局生成，不要手改。\nMEMORY {\n${memory.join('\n')}\n}\nSEGMENTS {\n${segments.join('\n')}\n}\n`;
}
