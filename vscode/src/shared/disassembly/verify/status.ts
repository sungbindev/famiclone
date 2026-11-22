// ------------------------------------------------------------------------
// 名称：status.ts
// 说明：进度报告：各类型区域的字节占比、自动命名的符号数、没有注释的过程数、未解析的间接跳转与跨
//       bank 转移、最近一次回读的结果。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-21
// 备注：自动命名的符号、没有说明的过程、没有处理的未解析项都算“未完成”；回读结果在工程改动之后算
//       过期。
// ------------------------------------------------------------------------
import { isImageStale, isVerificationStale, regionStats } from '../model/index';
import type { Project } from '../model/project';

export interface Status {
  /** 映像；stale 为 null 表示没有给映像字节，没有检查 */
  image: {path: string, size: number, stale: boolean|null};
  /** 各类型区域的字节数与占布局内字节的百分比 */
  regions: Record<string, {bytes: number, percent: number}>;
  /** 符号总数、自动命名数、已命名数 */
  symbols: {total: number, auto: number, named: number};
  /** 没有块注释的过程数 */
  proceduresWithoutComment: number;
  /** 未解析的间接跳转与跨 bank 转移 */
  unresolved: {indirectJumps: number, crossBank: number};
  /** 最近一次回读 */
  verification: {state: 'none'|'ok'|'failed'|'stale', at: string|null, firstDifference: Record<string, unknown>|null};
}

/**
 * 生成进度报告。
 * @param project 工程
 * @param [bytes] 映像字节；给出时检查映像是否变了
 * @returns 报告
 */
export function buildStatus(project: Project, bytes?: Uint8Array): Status {
  const stats = regionStats(project.regions);
  const mapped = project.layout.segments.reduce((sum, segment) => sum + segment.length, 0);
  const regions = Object.fromEntries(Object.entries(stats).map(([type, count]) => [type, { bytes: count, percent: mapped === 0 ? 0 : Math.round((count / mapped) * 1000) / 10 }]));
  const auto = project.symbols.filter((symbol) => symbol.auto).length;
  const commented = new Set(project.comments.filter((comment) => comment.kind === 'block').map((comment) => `${comment.segmentId}:${comment.address}`));
  const procedures = project.symbols.filter((symbol) => symbol.kind === 'procedure' && symbol.segmentId !== null);
  let verification: Status['verification'] = { state: 'none', at: null, firstDifference: null };
  if (project.verification !== null) {
    const state: Status['verification']['state'] = isVerificationStale(project) ? 'stale' : project.verification.ok ? 'ok' : 'failed';
    verification = { state, at: project.verification.at, firstDifference: project.verification.firstDifference ?? null };
  }
  return {
    image: { path: project.image.path, size: project.image.size, stale: bytes === undefined ? null : isImageStale(project, bytes) },
    regions,
    symbols: { total: project.symbols.length, auto, named: project.symbols.length - auto },
    proceduresWithoutComment: procedures.filter((symbol) => !commented.has(`${symbol.segmentId}:${symbol.address}`)).length,
    unresolved: {
      indirectJumps: project.unresolved.filter((item) => item.kind === 'indirect-jump').length,
      crossBank: project.unresolved.filter((item) => item.kind === 'cross-bank').length,
    },
    verification,
  };
}
