// ------------------------------------------------------------------------
// 名称：analysis-operations.ts
// 说明：分析类操作：trace（静态追踪）、list（分类列表）、view（带符号的清单）、xrefs（交叉引用）、
//       routine（例程上下文）、callgraph（调用图）、detect（识别候选）、reference（指令与符号查
//       询）。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-21
// 备注：除 trace 外都不修改工程；列表类结果带 limit 与 nextCursor，cursor 是下一页第一项的序号
//       （find 的 cursor 是文件偏移，见各自说明）。
// ------------------------------------------------------------------------
import type { DisasmEnvironment } from '../../../contracts/disassembly/disasm-environment';
import { FcdevError } from '../../../shared/errors/fcdev-error';
import { buildXrefs, detectCandidates, ensureAutoSymbols, findXrefs, scanCode, traceProject } from '../../../shared/disassembly/analysis/index';
import { OPCODE_TABLE, formatHex } from '../../../shared/disassembly/cpu/index';
import { applyEdits, resolveSegmentId } from '../../../shared/disassembly/model/index';
import type { EditItem } from '../../../shared/disassembly/model/index';
import { indexByValue, parseSymbolSet } from '../../../shared/disassembly/profile/index';
import { LIST_CATEGORIES, buildCallGraph, describeRoutine, enclosingRoutine, findRoutine, listItems, listRoutines, normalizePage, renderListing } from '../../../shared/disassembly/query/index';
import { analyze, loadContext, saveContext } from './context';
import { invalidParam, optionalAddress, optionalArray, optionalBoolean, optionalEnum, optionalInteger, optionalString, projectDirectory, requireString, toAddress } from '../../../shared/disassembly/operations/params';
import type { Routine } from '../../../shared/disassembly/query/routines';
import type { DetectOptions } from '../../../shared/disassembly/analysis/detect';
import type { AddressingMode, Opcode } from '../../../shared/disassembly/cpu/opcodes';

/** 警告最多返回的条数 */
const MAX_WARNINGS = 20;

/** 各寻址方式的写法 */
const MODE_SYNTAX: Readonly<Record<AddressingMode, string>> = {
  implied: '', accumulator: 'a', immediate: '#$nn', zeroPage: '$nn', zeroPageX: '$nn,x', zeroPageY: '$nn,y', absolute: '$nnnn', absoluteX: '$nnnn,x', absoluteY: '$nnnn,y',
  indirect: '($nnnn)', indexedIndirect: '($nn,x)', indirectIndexed: '($nn),y', relative: '$目标地址',
};

/**
 * 按序号分页。
 * @param items 全部项
 * @param params 参数里的 limit 与 cursor
 * @returns 一页与下一页游标
 */
function paginate(items: unknown[], params: Record<string, unknown>): {total: number, items: unknown[], nextCursor: string|null} {
  const { limit, cursor } = normalizePage({ limit: optionalInteger(params, 'limit', { min: 1 }), cursor: optionalString(params, 'cursor') });
  const page = items.slice(cursor, cursor + limit);
  return { total: items.length, items: page, nextCursor: cursor + limit < items.length ? String(cursor + limit) : null };
}

/**
 * 由例程名或地址参数找例程。
 * @param params routine（名字）或 address（加 segmentId）
 * @param routines 全部例程
 * @returns 例程
 */
function routineFromParams(params: Record<string, unknown>, routines: Routine[]): Routine {
  const name = optionalString(params, 'routine');
  const address = optionalAddress(params, 'address');
  if (name === undefined && address === undefined) invalidParam('要给 routine（例程名）或 address');
  return findRoutine(routines, { name, address, segmentId: optionalInteger(params, 'segmentId') });
}

/**
 * 静态追踪：必要时先加入手工入口，追踪后更新区域并给新发现的地址自动命名。
 * @param environment 运行环境
 * @param params projectDir；可选 entries（要加的入口）、autoName（默认 true）
 * @returns 指令数与新增数、例程数、新符号数、警告与未解析项
 */
export function trace({ files }: DisasmEnvironment, params: Record<string, unknown>): Record<string, unknown> {
  const context = loadContext(files, projectDirectory(params));
  const { bytes } = context;
  let project = context.project;
  const entries = optionalArray(params, 'entries') ?? [];
  const known = new Set(project.entries.map((entry) => `${entry.segmentId}:${entry.address}`));
  const edits: EditItem[] = [];
  for (const item of entries) {
    const record = item !== null && typeof item === 'object' ? (item as { address?: unknown; segmentId?: number }) : undefined;
    const address = toAddress(record === undefined ? item : record.address, 'entries');
    const segmentId = record?.segmentId;
    const resolved = resolveSegmentId(project.layout, address, segmentId, 'DISASM_PARAM_INVALID');
    if (resolved !== null && known.has(`${resolved}:${address}`)) continue;
    edits.push({ op: 'addEntry', address, segmentId, source: 'manual', description: '手工指定' });
    if (resolved !== null) known.add(`${resolved}:${address}`);
  }
  if (edits.length > 0) project = applyEdits(project, edits).project;

  const before = scanCode(project, bytes).list.length;
  const traced = traceProject(project, bytes);
  let next = traced.project;
  let newSymbols = 0;
  if (optionalBoolean(params, 'autoName', true)) {
    const named = ensureAutoSymbols(next, bytes, { externalNames: indexByValue(context.constants) });
    next = named.project;
    newSymbols = named.added;
  }
  saveContext(files, context, next);
  const scan = scanCode(next, bytes);
  const { analysis } = traced;
  return {
    entriesAdded: edits.length,
    instructions: scan.list.length,
    added: scan.list.length - before,
    routines: listRoutines(next, scan).length,
    newSymbols,
    tables: analysis.tables.length,
    outsideTargets: analysis.outside.length,
    unresolved: { indirectJumps: next.unresolved.filter((item) => item.kind === 'indirect-jump').length, crossBank: next.unresolved.filter((item) => item.kind === 'cross-bank').length },
    warningCount: analysis.warnings.length,
    warnings: analysis.warnings.slice(0, MAX_WARNINGS).map((warning) => warning.message),
  };
}

/**
 * 按类别列出。
 * @param environment 运行环境
 * @param params projectDir、category（entries、routines、unresolved、unknown、auto-symbols、
 *   uncommented-routines）；可选 limit、cursor
 * @returns 一页项与下一页游标
 */
export function list({ files }: DisasmEnvironment, params: Record<string, unknown>): Record<string, unknown> {
  const context = loadContext(files, projectDirectory(params));
  const category = optionalEnum(params, 'category', [...LIST_CATEGORIES]);
  if (category === undefined) invalidParam(`要给 category：${LIST_CATEGORIES.join('、')}`);
  const items = listItems({ project: context.project, scan: scanCode(context.project, context.bytes), category });
  return { category, ...paginate(items, params) };
}

/**
 * 带符号与注释的清单。
 * @param environment 运行环境
 * @param params projectDir；routine，或 address（加 segmentId）；可选 length、maxLines
 * @returns 清单文字与范围
 */
export function view({ files }: DisasmEnvironment, params: Record<string, unknown>): Record<string, unknown> {
  const context = loadContext(files, projectDirectory(params));
  const { project, bytes, index } = context;
  const scan = scanCode(project, bytes);
  const maxLines = optionalInteger(params, 'maxLines', { min: 1, max: 2000 });
  const routineName = optionalString(params, 'routine');
  if (routineName !== undefined) {
    const found = findRoutine(listRoutines(project, scan), { name: routineName });
    const listing = renderListing({ project, bytes, index, segmentId: found.segmentId, address: found.address, length: Math.max(1, found.end - found.address), maxLines, scan });
    return { routine: found.name, ...listing };
  }
  const address = optionalAddress(params, 'address');
  if (address === undefined) invalidParam('要给 routine 或 address');
  const segmentId = resolveSegmentId(project.layout, address, optionalInteger(params, 'segmentId'), 'DISASM_PARAM_INVALID');
  if (segmentId === null) invalidParam(`${formatHex(address, 4)} 不在布局里`, '先用 layout 查看布局');
  return { ...renderListing({ project, bytes, index, segmentId, address, length: optionalInteger(params, 'length', { min: 1, max: 4096 }), maxLines, scan }) };
}

/**
 * 交叉引用。
 * @param environment 运行环境
 * @param params projectDir；symbol，或 address（加 segmentId）；可选 kind、limit、cursor
 * @returns 目标与一页引用
 */
export function xrefs({ files }: DisasmEnvironment, params: Record<string, unknown>): Record<string, unknown> {
  const context = loadContext(files, projectDirectory(params));
  const { project, bytes, index } = context;
  const symbolName = optionalString(params, 'symbol');
  let address = optionalAddress(params, 'address');
  if (symbolName !== undefined) {
    const symbol = index.byName.get(symbolName);
    if (symbol === undefined) throw new FcdevError(`没有符号 ${symbolName}`, { code: 'DISASM_SYMBOL_NOT_FOUND' });
    address = symbol.address;
  }
  if (address === undefined) invalidParam('要给 symbol 或 address');
  const analysis = analyze(context);
  const found = findXrefs(buildXrefs({ bytes, layout: project.layout, analysis }), address, { kind: optionalString(params, 'kind') });
  const routines = listRoutines(project, scanCode(project, bytes));
  const refs = found.map((ref) => ({
    kind: ref.kind,
    address: ref.address,
    segmentId: ref.segmentId,
    mnemonic: ref.mnemonic ?? null,
    indexed: ref.indexed ?? false,
    likely: ref.likely ?? false,
    routine: enclosingRoutine(routines, ref.segmentId, ref.address)?.name ?? null,
  }));
  return { target: { address, name: index.nameFor(address) }, ...paginate(refs, params) };
}

/**
 * 一个例程的上下文。
 * @param environment 运行环境
 * @param params projectDir；routine，或 address（加 segmentId）；可选 maxLines
 * @returns 范围、寄存器推测、调用者、被调用者、读写的地址与清单
 */
export function routine({ files }: DisasmEnvironment, params: Record<string, unknown>): Record<string, unknown> {
  const context = loadContext(files, projectDirectory(params));
  const { project, bytes, index } = context;
  const scan = scanCode(project, bytes);
  const routines = listRoutines(project, scan);
  const chosen = routineFromParams(params, routines);
  const xrefIndex = buildXrefs({ bytes, layout: project.layout, analysis: analyze(context) });
  return describeRoutine({ project, bytes, index, scan, xrefs: xrefIndex, routines, routine: chosen, maxLines: optionalInteger(params, 'maxLines', { min: 1, max: 2000 }) });
}

/**
 * 调用图。
 * @param environment 运行环境
 * @param params projectDir；routine，或 address（加 segmentId）；可选 depth（1 到 8，默认 2）、
 *   direction（callees 或 callers）
 * @returns 树文字与节点数
 */
export function callgraph({ files }: DisasmEnvironment, params: Record<string, unknown>): Record<string, unknown> {
  const context = loadContext(files, projectDirectory(params));
  const { project, bytes } = context;
  const routines = listRoutines(project, scanCode(project, bytes));
  const root = routineFromParams(params, routines);
  const xrefIndex = buildXrefs({ bytes, layout: project.layout, analysis: analyze(context) });
  return buildCallGraph({ routines, xrefs: xrefIndex, root, depth: optionalInteger(params, 'depth', { min: 1, max: 8 }) ?? 2, direction: optionalEnum(params, 'direction', ['callees', 'callers'], 'callees') as 'callees' | 'callers' });
}

/**
 * 识别候选（填充、地址表、字符串、图形），不修改工程。
 * @param environment 运行环境
 * @param params projectDir；可选 types、minPadding、minString、minTableEntries、graphicsPorts、
 *   limit、cursor
 * @returns 一页候选与下一页游标
 */
export function detect({ files }: DisasmEnvironment, params: Record<string, unknown>): Record<string, unknown> {
  const context = loadContext(files, projectDirectory(params));
  const { project, bytes } = context;
  const options: DetectOptions = {};
  for (const name of ['minPadding', 'minString', 'minTableEntries'] as const) {
    const value = optionalInteger(params, name, { min: 1 });
    if (value !== undefined) options[name] = value;
  }
  const ports = optionalArray(params, 'graphicsPorts');
  if (ports !== undefined) options.graphicsPorts = ports.map((port) => toAddress(port, 'graphicsPorts'));
  else if (project.profile !== null && project.profile.graphicsPorts.length > 0) options.graphicsPorts = project.profile.graphicsPorts;
  const result = detectCandidates({ bytes, layout: project.layout, analysis: analyze(context), regions: project.regions, options });
  const types = optionalArray(params, 'types');
  const all = [...result.padding, ...result.tables, ...result.strings, ...result.graphics]
    .filter((candidate) => types === undefined || types.includes(candidate.type))
    .toSorted((a, b) => a.offset - b.offset)
    .map((candidate) => ({ type: candidate.type, segmentId: candidate.segmentId, address: candidate.address, length: candidate.length, confidence: candidate.confidence, evidence: candidate.evidence, params: candidate.params }));
  return paginate(all, params);
}

/**
 * 查询指令与符号：指令给出每种寻址方式的操作码、长度与写法；符号在工程的符号与符号集的常量里搜索。
 * @param environment 运行环境
 * @param params query（助记符、符号名片段或地址）；可选 projectDir、limit、cursor
 * @returns 指令（没有匹配时为 null）与一页符号
 */
export function reference({ files }: DisasmEnvironment, params: Record<string, unknown>): Record<string, unknown> {
  const query = requireString(params, 'query').trim();
  const mnemonic = query.toLowerCase();
  const forms = OPCODE_TABLE.filter((entry): entry is Readonly<Opcode> => entry !== null && entry.mnemonic === mnemonic).map((entry) => ({
    opcode: formatHex(entry.opcode, 2), mode: entry.mode, size: entry.size, flow: entry.flow, syntax: `${entry.mnemonic} ${MODE_SYNTAX[entry.mode]}`.trim(),
  }));
  const instruction = forms.length === 0 ? null : { mnemonic, forms };
  if (params.projectDir === undefined) return { instruction, symbols: { total: 0, items: [], nextCursor: null } };

  const context = loadContext(files, projectDirectory(params));
  const asAddress = /^(?:\$|0[xX])[0-9A-Fa-f]{1,4}$/.test(query) ? toAddress(query, 'query') : undefined;
  const needle = query.toLowerCase();
  const matches: Array<Record<string, unknown>> = [];
  for (const symbol of context.project.symbols) {
    if (symbol.name.toLowerCase().includes(needle) || symbol.address === asAddress) matches.push({ source: 'project', name: symbol.name, address: symbol.address, segmentId: symbol.segmentId, kind: symbol.kind, description: symbol.description });
  }
  for (const [name, text] of context.symbolSetTexts) {
    for (const constant of parseSymbolSet(text)) {
      if (!constant.name.toLowerCase().includes(needle) && constant.value !== asAddress) continue;
      matches.push({ source: name, name: constant.name, address: constant.value, segmentId: null, kind: 'constant', description: '' });
    }
  }
  return { instruction, symbols: paginate(matches, params) };
}
