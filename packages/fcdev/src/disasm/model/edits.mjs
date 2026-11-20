// ------------------------------------------------------------------------
// 名称：edits.mjs
// 说明：批量编辑工程：定义与重命名符号、标注区域、加注释、操作数提示、归入模块、增删入口；整批要么全部生效，要么都不生效。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-21
// 备注：在工程的副本上依次执行，任何一项出错就抛出带项号的错误，原工程不受影响；成功时返回新工程。
//       区域、注释、提示、入口用 CPU 地址（换页窗口里加段号）定位，内部换算成文件偏移。
// ------------------------------------------------------------------------
import { FcdevError } from '../../shared/index.mjs';
import { createEntry } from './entries.mjs';
import { addressToOffset, getSegment } from './layout.mjs';
import { cloneProject } from './project.mjs';
import { setRegion } from './regions.mjs';
import { assertModulesDisjoint, assertSymbolName, normalizeComment, normalizeHint, normalizeModule, normalizeSymbol, resolveSegmentId } from './symbols.mjs';

/**
 * 抛出编辑错误。
 * @param {string} message 说明
 * @returns {never}
 */
function fail(message) {
  throw new FcdevError(message, { code: 'DISASM_EDIT_INVALID' });
}

/**
 * 按名字找符号的下标。
 * @param {import('./project.mjs').Project} project 工程
 * @param {string} name 符号名
 * @returns {number} 下标，不存在时为 -1
 */
function symbolIndex(project, name) {
  return project.symbols.findIndex((symbol) => symbol.name === name);
}

/**
 * 检查符号名没有被占用。
 * @param {import('./project.mjs').Project} project 工程
 * @param {string} name 符号名
 */
function assertNameFree(project, name) {
  if (symbolIndex(project, name) >= 0) fail(`符号 ${name} 已经存在`);
}

/**
 * 把 (地址, 可选段号) 换算成段与文件偏移。
 * @param {import('./project.mjs').Project} project 工程
 * @param {{address: number, segmentId?: number}} where 位置
 * @returns {{segment: import('./layout.mjs').Segment, offset: number}} 段与文件偏移
 */
function locate(project, { address, segmentId }) {
  const id = resolveSegmentId(project.layout, address, segmentId, 'DISASM_EDIT_INVALID');
  if (id === null) fail(`地址 $${address.toString(16).toUpperCase()} 不在布局里`);
  const segment = getSegment(project.layout, id);
  return { segment, offset: addressToOffset(segment, address) };
}

/** 各种编辑操作：参数为（工程的副本，操作） */
const OPERATIONS = {
  /** 定义符号 */
  defineSymbol(project, op) {
    const symbol = normalizeSymbol(project.layout, op);
    assertNameFree(project, symbol.name);
    const clash = project.symbols.find((item) => item.address === symbol.address && item.segmentId === symbol.segmentId);
    if (clash !== undefined) fail(`地址 $${symbol.address.toString(16).toUpperCase()} 上已经有符号 ${clash.name}`);
    project.symbols.push(symbol);
  },

  /** 重命名符号，同时清除自动命名标记 */
  renameSymbol(project, { from, to }) {
    const index = symbolIndex(project, from);
    if (index < 0) fail(`符号 ${from} 不存在`);
    assertSymbolName(to);
    if (from !== to) assertNameFree(project, to);
    project.symbols[index] = normalizeSymbol(project.layout, { ...project.symbols[index], name: to, auto: false });
    for (let hintIndex = 0; hintIndex < project.hints.length; hintIndex += 1) {
      if (project.hints[hintIndex].symbol === from) project.hints[hintIndex] = normalizeHint(project.layout, { ...project.hints[hintIndex], symbol: to });
    }
  },

  /** 修改符号的种类、说明、所属模块 */
  updateSymbol(project, { name, kind, description, module }) {
    const index = symbolIndex(project, name);
    if (index < 0) fail(`符号 ${name} 不存在`);
    const current = project.symbols[index];
    project.symbols[index] = normalizeSymbol(project.layout, {
      ...current, kind: kind ?? current.kind, description: description ?? current.description, module: module === undefined ? current.module : module,
    });
  },

  /** 删除符号 */
  removeSymbol(project, { name }) {
    const index = symbolIndex(project, name);
    if (index < 0) fail(`符号 ${name} 不存在`);
    if (project.hints.some((hint) => hint.symbol === name)) fail(`符号 ${name} 还被操作数提示引用着`);
    project.symbols.splice(index, 1);
  },

  /** 标注区域 */
  setRegion(project, op) {
    const { offset } = locate(project, op);
    setRegion(project.regions, { offset, length: op.length, type: op.type, params: op.params, source: op.source ?? 'manual' });
  },

  /** 加注释，同一地址同一种类的旧注释被替换 */
  setComment(project, op) {
    const comment = normalizeComment(project.layout, op);
    project.comments = project.comments.filter((item) => !(item.segmentId === comment.segmentId && item.address === comment.address && item.kind === comment.kind));
    project.comments.push(comment);
  },

  /** 删除注释 */
  removeComment(project, op) {
    const segmentId = resolveSegmentId(project.layout, op.address, op.segmentId, 'DISASM_EDIT_INVALID');
    const before = project.comments.length;
    project.comments = project.comments.filter((item) => !(item.segmentId === segmentId && item.address === op.address && item.kind === op.kind));
    if (project.comments.length === before) fail(`$${op.address.toString(16).toUpperCase()} 上没有${op.kind === 'block' ? '块' : '行尾'}注释`);
  },

  /** 设置操作数提示，同一条指令的旧提示被替换 */
  setHint(project, op) {
    const hint = normalizeHint(project.layout, op);
    if (hint.symbol !== null && symbolIndex(project, hint.symbol) < 0) fail(`提示引用的符号 ${hint.symbol} 不存在`);
    project.hints = project.hints.filter((item) => !(item.segmentId === hint.segmentId && item.address === hint.address));
    project.hints.push(hint);
  },

  /** 删除操作数提示 */
  removeHint(project, op) {
    const segmentId = resolveSegmentId(project.layout, op.address, op.segmentId, 'DISASM_EDIT_INVALID');
    const before = project.hints.length;
    project.hints = project.hints.filter((item) => !(item.segmentId === segmentId && item.address === op.address));
    if (project.hints.length === before) fail(`$${op.address.toString(16).toUpperCase()} 上没有操作数提示`);
  },

  /** 把一段地址归入模块，模块不存在时创建 */
  assignModule(project, op) {
    const range = normalizeModule(project.layout, { name: op.name, ranges: [{ segmentId: op.segmentId, address: op.address, length: op.length }] }).ranges[0];
    const existing = project.modules.find((module) => module.name === op.name);
    const next = { name: op.name, description: op.description ?? existing?.description ?? '', ranges: [...(existing?.ranges ?? []), range] };
    const modules = [...project.modules.filter((module) => module.name !== op.name), next];
    assertModulesDisjoint(modules);
    project.modules = modules;
  },

  /** 删除模块（它的范围回到默认模块） */
  removeModule(project, { name }) {
    if (!project.modules.some((module) => module.name === name)) fail(`模块 ${name} 不存在`);
    project.modules = project.modules.filter((module) => module.name !== name);
    project.symbols = project.symbols.map((symbol) => (symbol.module === name ? normalizeSymbol(project.layout, { ...symbol, module: null }) : symbol));
  },

  /** 增加入口 */
  addEntry(project, op) {
    const segmentId = resolveSegmentId(project.layout, op.address, op.segmentId, 'DISASM_EDIT_INVALID');
    if (segmentId === null) fail(`入口地址 $${op.address.toString(16).toUpperCase()} 不在布局里`);
    const entry = createEntry(project.layout, { segmentId, address: op.address, source: op.source ?? 'manual', description: op.description });
    if (project.entries.some((item) => item.segmentId === entry.segmentId && item.address === entry.address)) fail(`入口 $${op.address.toString(16).toUpperCase()} 已经存在`);
    project.entries.push(entry);
  },

  /** 删除入口 */
  removeEntry(project, op) {
    const segmentId = resolveSegmentId(project.layout, op.address, op.segmentId, 'DISASM_EDIT_INVALID');
    const before = project.entries.length;
    project.entries = project.entries.filter((item) => !(item.segmentId === segmentId && item.address === op.address));
    if (project.entries.length === before) fail(`入口 $${op.address.toString(16).toUpperCase()} 不存在`);
  },
};

/** 支持的操作名 */
export const EDIT_OPERATIONS = Object.freeze(Object.keys(OPERATIONS));

/**
 * 批量编辑工程。
 * @param {import('./project.mjs').Project} project 工程，不会被修改
 * @param {Array<{op: string}>} edits 编辑项，每项的 op 是操作名，其余是该操作的参数
 * @returns {{project: import('./project.mjs').Project, applied: number}} 编辑后的新工程与成功的项数
 * @throws {FcdevError} 任何一项出错：错误里写明项号与操作，整批不生效
 */
export function applyEdits(project, edits) {
  const next = cloneProject(project);
  edits.forEach((edit, index) => {
    const operation = Object.hasOwn(OPERATIONS, edit?.op) ? OPERATIONS[edit.op] : undefined;
    if (operation === undefined) {
      throw new FcdevError(`第 ${index + 1} 项：未知的操作 ${String(edit?.op)}`, { code: 'DISASM_EDIT_INVALID', hint: `可用的操作：${EDIT_OPERATIONS.join('、')}` });
    }
    try {
      operation(next, edit);
    } catch (error) {
      if (!(error instanceof FcdevError)) throw error;
      throw new FcdevError(`第 ${index + 1} 项（${edit.op}）：${error.message}`, { code: error.code, hint: error.hint, cause: error });
    }
  });
  return { project: next, applied: edits.length };
}
