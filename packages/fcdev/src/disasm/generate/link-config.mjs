// ------------------------------------------------------------------------
// 名称：link-config.mjs
// 说明：由布局和片生成 ld65 的链接配置：每个布局段一个存储区、一个输出文件，段里的片按地址顺序摆放。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-21
// 备注：换页窗口里同一个 CPU 地址的多个 bank 各有自己的存储区与输出文件（seg<段号>.bin），互不冲突；
//       存储区的大小恰好是段的长度，输出文件逐字节对应映像里的这一段。
// ------------------------------------------------------------------------

/**
 * 段对应的输出文件名。
 * @param {number} segmentId 段号
 * @returns {string} 文件名
 */
export function segmentOutputName(segmentId) {
  return `seg${segmentId}.bin`;
}

/**
 * 生成链接配置。
 * @param {import('../model/project.mjs').Project} project 工程
 * @param {import('./pieces.mjs').Piece[]} pieces 片，按段号与偏移排序
 * @returns {string} 配置内容
 */
export function renderLinkConfig(project, pieces) {
  const hex = (value) => `$${value.toString(16).toUpperCase()}`;
  const memory = [];
  const segments = [];
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
