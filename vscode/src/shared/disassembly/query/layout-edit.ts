// ------------------------------------------------------------------------
// 名称：layout-edit.ts
// 说明：替换工程的布局：按新的段列表重建布局，入口、符号、注释、提示、模块里仍然有效的保留，其余丢
//       弃并统计。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-21
// 备注：区域表重建为空（追踪出的区域下次追踪再生成，手工标注的区域一并丢弃并计入统计）；最近一次回
//       读的结果与未解析项清空；有效的意思是：原来的段号仍然存在并覆盖这个地址，或者新布局里只有一
//       个段覆盖这个地址。
// ------------------------------------------------------------------------
import { FcdevError } from '../../errors/fcdev-error';
import { cloneProject, createEntry, createLayout, createRegionMap, normalizeComment, normalizeHint, normalizeModule, normalizeSymbol } from '../model/index';
import type { Project } from '../model/project';

/**
 * 依次尝试各种段号，返回第一个成功的结果。
 * @param candidates 段号候选
 * @param make 用段号创建新对象，不合法时抛 FcdevError
 * @returns 新对象；都不行时为 null
 * @template T
 */
function rebind<T>(candidates: Array<number|undefined>, make: (segmentId: number|undefined) => T): T|null {
  for (const segmentId of candidates) {
    try {
      return make(segmentId);
    } catch (error) {
      if (!(error instanceof FcdevError)) throw error;
    }
  }
  return null;
}

/**
 * 一个段号的候选顺序：原段号（有的话）优先，再按地址重新推断。
 * @param segmentId 原段号
 * @returns 按顺序尝试的段号，undefined 表示按地址推断
 */
function attempts(segmentId: number | null | undefined): Array<number | undefined> {
  return [...(segmentId === null || segmentId === undefined ? [] : [segmentId]), undefined];
}

/**
 * 替换布局。
 * @param project 工程，不会被修改
 * @param segments 新的段列表
 * @returns 新工程与各项丢弃的个数
 * @throws FcdevError 新布局无效
 */
export function replaceLayout(project: Project, segments: Array<Record<string, unknown>>): {project: Project, dropped: Record<string, number>} {
  const layout = createLayout(segments, { imageSize: project.layout.imageSize });
  const next = cloneProject(project);
  next.layout = layout;
  next.regions = createRegionMap(layout);
  next.unresolved = [];
  next.verification = null;
  next.format = { name: project.format.name, options: project.format.name === 'flat' ? { segments: layout.segments.map((segment) => ({ ...segment })) } : project.format.options };
  const dropped = { entries: 0, symbols: 0, comments: 0, hints: 0, modules: 0, regions: project.regions.regions.filter((region) => region.source === 'manual').length };

  next.entries = project.entries.flatMap((entry) => {
    const made = rebind(attempts(entry.segmentId), (segmentId) => {
      const found = segmentId ?? layout.segments.find((segment) => entry.address >= segment.address && entry.address < segment.address + segment.length)?.id;
      if (found === undefined) throw new FcdevError(`入口地址 ${entry.address} 不在新布局里`, { code: 'DISASM_ENTRY_INVALID' });
      return createEntry(layout, { segmentId: found, address: entry.address, source: entry.source, description: entry.description });
    });
    if (made === null) dropped.entries += 1;
    return made === null ? [] : [made];
  });
  next.symbols = project.symbols.flatMap((symbol) => {
    const made = rebind(attempts(symbol.segmentId), (segmentId) => normalizeSymbol(layout, { ...symbol, segmentId }));
    if (made === null) dropped.symbols += 1;
    return made === null ? [] : [made];
  });
  const keptNames = new Set(next.symbols.map((symbol) => symbol.name));
  next.comments = project.comments.flatMap((comment) => {
    const made = rebind(attempts(comment.segmentId), (segmentId) => normalizeComment(layout, { ...comment, segmentId }));
    if (made === null) dropped.comments += 1;
    return made === null ? [] : [made];
  });
  next.hints = project.hints.flatMap((hint) => {
    const made = hint.symbol !== null && !keptNames.has(hint.symbol) ? null : rebind(attempts(hint.segmentId), (segmentId) => normalizeHint(layout, { ...hint, segmentId }));
    if (made === null) dropped.hints += 1;
    return made === null ? [] : [made];
  });
  next.modules = project.modules.flatMap((module) => {
    const made = rebind([undefined], () => normalizeModule(layout, module));
    if (made === null) dropped.modules += 1;
    return made === null ? [] : [made];
  });
  return { project: next, dropped };
}
