// ------------------------------------------------------------------------
// 名称：editing-operations.ts
// 说明：编辑类操作：apply（批量编辑，整批生效或整批不改）与 symbols（导入符号集，列出与搜索符号）。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-21
// 备注：导入符号集时先检查：文件里有常量，名字不与工程里已有的符号冲突；导入后需要重新生成源码。
// ------------------------------------------------------------------------
import * as path from 'node:path';
import type { DisasmEnvironment } from '../../../contracts/disassembly/disasm-environment';
import type { FileSystem } from '../../../contracts/files/file-system';
import { FcdevError } from '../../../shared/errors/fcdev-error';
import { EDIT_OPERATIONS, applyEdits, cloneProject } from '../../../shared/disassembly/model/index';
import { parseSymbolSet } from '../../../shared/disassembly/profile/index';
import { normalizePage } from '../../../shared/disassembly/query/index';
import { buildStatus } from '../../../shared/disassembly/verify/status';
import { toProjectPath } from '../workspace/index';
import { loadContext, saveContext } from './context';
import { invalidParam, normalizeEdits, optionalEnum, optionalInteger, optionalString, projectDirectory, requireString } from '../../../shared/disassembly/operations/params';
import type { Context } from './context';

/** 每个符号集名字最多列出几个冲突 */
const MAX_CONFLICTS = 5;

/**
 * 批量编辑。
 * @param environment 运行环境
 * @param params projectDir、edits（编辑项数组，每项的 op 见 EDIT_OPERATIONS）
 * @returns 成功的项数与进度概况
 */
export function apply({ files }: DisasmEnvironment, params: Record<string, unknown>): Record<string, unknown> {
  const context = loadContext(files, projectDirectory(params));
  const edits = params.edits;
  if (!Array.isArray(edits) || edits.length === 0) invalidParam('edits 必须是非空数组', `每项的 op 取：${EDIT_OPERATIONS.join('、')}`);
  const { project, applied } = applyEdits(context.project, normalizeEdits(edits));
  saveContext(files, context, project);
  const { symbols: symbolCounts, proceduresWithoutComment, regions } = buildStatus(project, context.bytes);
  return { applied, symbols: symbolCounts, proceduresWithoutComment, unknownBytes: regions.unknown?.bytes ?? 0 };
}

/**
 * 导入符号集，或列出与搜索符号。
 * @param environment 运行环境
 * @param params projectDir、action（import、list、search，缺省为 list）；import 要给 file；search
 *   要给 query；list 与 search 可给 scope（project、sets、all）、kind、limit、cursor
 * @returns 导入结果，或一页符号
 */
export function symbols({ files }: DisasmEnvironment, params: Record<string, unknown>): Record<string, unknown> {
  const context = loadContext(files, projectDirectory(params));
  const action = optionalEnum(params, 'action', ['import', 'list', 'search'], 'list');
  if (action === 'import') return importSymbolSet(files, context, requireString(params, 'file'));

  const query = (optionalString(params, 'query') ?? '').toLowerCase();
  if (action === 'search' && query === '') invalidParam('search 要给 query');
  const scope = optionalEnum(params, 'scope', ['project', 'sets', 'all'], 'all');
  const kind = optionalString(params, 'kind');
  const items: Array<Record<string, unknown>> = [];
  if (scope !== 'sets') {
    for (const symbol of [...context.project.symbols].toSorted((a, b) => (a.segmentId ?? -1) - (b.segmentId ?? -1) || a.address - b.address)) {
      if ((kind !== undefined && symbol.kind !== kind) || !symbol.name.toLowerCase().includes(query)) continue;
      items.push({ source: 'project', name: symbol.name, address: symbol.address, segmentId: symbol.segmentId, kind: symbol.kind, auto: symbol.auto, module: symbol.module, description: symbol.description });
    }
  }
  if (scope !== 'project' && kind === undefined) {
    for (const [name, text] of context.symbolSetTexts) {
      for (const constant of parseSymbolSet(text)) {
        if (constant.name.toLowerCase().includes(query)) items.push({ source: name, name: constant.name, address: constant.value, segmentId: null, kind: 'constant' });
      }
    }
  }
  const { limit, cursor } = normalizePage({ limit: optionalInteger(params, 'limit', { min: 1 }), cursor: optionalString(params, 'cursor') });
  return { total: items.length, items: items.slice(cursor, cursor + limit), nextCursor: cursor + limit < items.length ? String(cursor + limit) : null };
}

/**
 * 把符号集文件登记进工程。
 * @param files 文件系统
 * @param context 上下文
 * @param file 符号集文件的路径
 * @returns 符号集名、常量个数与保存的路径
 */
function importSymbolSet(files: FileSystem, context: Context, file: string): Record<string, unknown> {
  const absolute = path.resolve(file);
  let text: string;
  try {
    text = files.readText(absolute);
  } catch (error) {
    throw new FcdevError(`读不到符号集 ${absolute}`, { code: 'DISASM_SYMBOL_SET_MISSING', cause: error });
  }
  const name = path.basename(absolute);
  if (!/^[A-Za-z0-9_-]+\.inc$/.test(name)) invalidParam(`符号集的文件名要写成 xxx.inc：${name}`);
  if (context.project.symbolSets.some((set) => set.name === name)) throw new FcdevError(`符号集 ${name} 已经导入`, { code: 'DISASM_SYMBOL_SET_DUPLICATE' });
  const constants = parseSymbolSet(text);
  if (constants.length === 0) throw new FcdevError(`${name} 里没有“名字 = 数值”形式的常量`, { code: 'DISASM_SYMBOL_SET_EMPTY' });
  const names = new Set(constants.map((constant) => constant.name));
  const clashes = context.project.symbols.filter((symbol) => names.has(symbol.name)).map((symbol) => symbol.name);
  if (clashes.length > 0) {
    throw new FcdevError(`工程里的符号与 ${name} 里的常量重名：${clashes.slice(0, MAX_CONFLICTS).join('、')}${clashes.length > MAX_CONFLICTS ? ' 等' : ''}`, {
      code: 'DISASM_SYMBOL_DUPLICATE',
      hint: '先用 apply 的 renameSymbol 或 removeSymbol 处理这些符号',
    });
  }
  const next = cloneProject(context.project);
  next.symbolSets = [...next.symbolSets, { name, path: toProjectPath(context.dir, absolute) }];
  saveContext(files, context, next);
  return { imported: name, constants: constants.length, path: toProjectPath(context.dir, absolute), note: '重新生成源码后才会用到这些名字；下次追踪的自动命名也会跳过它们已有名字的地址' };
}
