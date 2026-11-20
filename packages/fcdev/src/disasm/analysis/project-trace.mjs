// ------------------------------------------------------------------------
// 名称：project-trace.mjs
// 说明：对工程做静态追踪：从工程的入口出发追踪，把结果写回区域表与“未解析”列表。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-21
// 备注：每次从头追踪；上一次追踪生成的区域（来源 trace、table）先清除，再写入新的；来源是 manual 等的区域是用户标注的，不被盖住：
//       追踪出的区域与它们重叠时，那一块不写入。
// ------------------------------------------------------------------------
import { cloneProject, setRegion } from '../model/index.mjs';
import { buildRegions } from './build-regions.mjs';
import { createAnalysis, traceCode } from './tracer.mjs';

/**
 * 追踪工程。
 * @param {import('../model/project.mjs').Project} project 工程，不会被修改
 * @param {Uint8Array} bytes 映像的全部字节
 * @returns {{project: import('../model/project.mjs').Project, analysis: import('./tracer.mjs').Analysis, summary: import('./tracer.mjs').TraceSummary}} 新工程、分析结果与本次追踪的数量
 */
export function traceProject(project, bytes) {
  const next = cloneProject(project);
  const analysis = createAnalysis(next.layout);
  const summary = traceCode(analysis, { bytes, layout: next.layout }, next.entries);
  next.regions.regions = next.regions.regions.filter((region) => region.source !== 'trace' && region.source !== 'table');
  const manual = next.regions.regions;
  for (const region of buildRegions(next.layout, analysis).regions) {
    const end = region.offset + region.length;
    if (manual.some((item) => item.offset < end && region.offset < item.offset + item.length)) continue;
    setRegion(next.regions, region);
  }
  next.unresolved = analysis.unresolved.map((item) => ({ ...item }));
  return { project: next, analysis, summary };
}
