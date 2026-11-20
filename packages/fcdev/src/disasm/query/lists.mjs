// ------------------------------------------------------------------------
// 名称：lists.mjs
// 说明：按类别列出工程里待处理或需要查看的项：入口、例程、未解析项、没有识别的字节、自动命名的符号、没有说明的例程。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-21
// 备注：每个类别返回一个数组，由调用者分页；“没有识别的字节”按段里连续的 unknown 区域合并。
// ------------------------------------------------------------------------
import { FcdevError } from '../../shared/index.mjs';
import { listRegions } from '../model/index.mjs';
import { listRoutines } from './routines.mjs';

/** 可用的类别 */
export const LIST_CATEGORIES = Object.freeze(['entries', 'routines', 'unresolved', 'unknown', 'auto-symbols', 'uncommented-routines']);

/**
 * 列出一个类别的全部项。
 * @param {{project: import('../model/project.mjs').Project, scan: import('../analysis/code-scan.mjs').CodeScan, category: string}} input 工程、指令扫描结果与类别
 * @returns {Array<Record<string, unknown>>} 项，按段与地址排序
 * @throws {FcdevError} 类别不认识
 */
export function listItems({ project, scan, category }) {
  switch (category) {
    case 'entries':
      return project.entries.map((entry) => ({ segmentId: entry.segmentId, address: entry.address, source: entry.source, description: entry.description }));
    case 'routines': {
      const commented = commentedAddresses(project);
      return listRoutines(project, scan).map((routine) => ({ ...routine, size: routine.end - routine.address, commented: commented.has(`${routine.segmentId}:${routine.address}`) }));
    }
    case 'unresolved':
      return project.unresolved.map((item) => ({ ...item }));
    case 'unknown': {
      const items = [];
      for (const segment of [...project.layout.segments].sort((a, b) => a.fileOffset - b.fileOffset)) {
        for (const region of listRegions(project.regions, segment.fileOffset, segment.length)) {
          if (region.type !== 'unknown') continue;
          items.push({ segmentId: segment.id, address: segment.address + (region.offset - segment.fileOffset), length: region.length, offset: region.offset });
        }
      }
      return items;
    }
    case 'auto-symbols':
      return project.symbols.filter((symbol) => symbol.auto).sort((a, b) => (a.segmentId ?? -1) - (b.segmentId ?? -1) || a.address - b.address).map((symbol) => ({ name: symbol.name, kind: symbol.kind, segmentId: symbol.segmentId, address: symbol.address }));
    case 'uncommented-routines': {
      const commented = commentedAddresses(project);
      return listRoutines(project, scan).filter((routine) => !commented.has(`${routine.segmentId}:${routine.address}`)).map((routine) => ({ ...routine, size: routine.end - routine.address }));
    }
    default:
      throw new FcdevError(`未知的类别：${String(category)}`, { code: 'DISASM_PARAM_INVALID', hint: `可用的类别：${LIST_CATEGORIES.join('、')}` });
  }
}

/**
 * 有块注释的位置。
 * @param {import('../model/project.mjs').Project} project 工程
 * @returns {Set<string>} 段号:地址
 */
function commentedAddresses(project) {
  return new Set(project.comments.filter((comment) => comment.kind === 'block').map((comment) => `${comment.segmentId}:${comment.address}`));
}
