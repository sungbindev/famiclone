// ------------------------------------------------------------------------
// 名称：symbols.mjs
// 说明：工程里的符号、注释、操作数提示、模块的定义与检查。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-21
// 备注：符号在布局内的由“段号加地址”定位，不在任何段里的（硬件寄存器、RAM 变量）段号为 null；注释、提示与模块只能放在布局内。
//       符号名必须是合法的 ca65 标识符，且不能与指令助记符、寄存器名相同。
// ------------------------------------------------------------------------
import { FcdevError } from '../../shared/index.mjs';
import { OPCODE_TABLE } from '../cpu/index.mjs';
import { findSegmentsAt, getSegment } from './layout.mjs';

/** 符号种类：过程、标签、变量、常量、数据 */
export const SYMBOL_KINDS = Object.freeze(['procedure', 'label', 'variable', 'constant', 'data']);

/** 注释种类：块注释写在地址所在行的上面，行尾注释写在那一行的末尾 */
export const COMMENT_KINDS = Object.freeze(['block', 'inline']);

/** 操作数提示的写法：符号（可带偏移）、低字节、高字节、十进制、二进制、字符 */
export const HINT_FORMS = Object.freeze(['symbol', 'low', 'high', 'decimal', 'binary', 'char']);

/** 不能当符号名的字：全部助记符与寄存器名 */
const RESERVED_NAMES = new Set([...OPCODE_TABLE.filter((entry) => entry !== null).map((entry) => entry.mnemonic), 'a', 'x', 'y', 's', 'z']);

/** 合法的 ca65 标识符 */
const IDENTIFIER = /^[A-Za-z_][A-Za-z0-9_]*$/;

/** 模块名同时是源文件名，只用字母、数字、下划线与连字符 */
const MODULE_NAME = /^[A-Za-z0-9_-]+$/;

/**
 * 抛出带错误码的错误。
 * @param {string} code 错误码
 * @param {string} message 说明
 * @returns {never}
 */
function fail(code, message) {
  throw new FcdevError(message, { code });
}

/**
 * 检查符号名。
 * @param {string} name 符号名
 * @throws {FcdevError} 不是合法标识符，或与助记符、寄存器名相同
 */
export function assertSymbolName(name) {
  if (typeof name !== 'string' || !IDENTIFIER.test(name)) fail('DISASM_SYMBOL_INVALID', `符号名不合法：${String(name)}（只能用字母、数字、下划线，不能以数字开头）`);
  if (RESERVED_NAMES.has(name.toLowerCase())) fail('DISASM_SYMBOL_INVALID', `符号名不能与指令助记符或寄存器名相同：${name}`);
}

/**
 * 确定地址所在的段号。
 * @param {import('./layout.mjs').Layout} layout 布局
 * @param {number} address CPU 地址
 * @param {number|null} [segmentId] 调用者给出的段号；省略时按地址推断，布局里没有这个地址时为 null
 * @param {string} [code] 出错时的错误码
 * @returns {number|null} 段号
 * @throws {FcdevError} 给出的段号不存在或不覆盖地址，换页窗口里没给段号
 */
export function resolveSegmentId(layout, address, segmentId, code = 'DISASM_ADDRESS_INVALID') {
  if (!Number.isInteger(address) || address < 0 || address > 0xffff) fail(code, `地址不合法：${String(address)}`);
  const candidates = findSegmentsAt(layout, address);
  if (segmentId === null) {
    if (candidates.length > 0) fail(code, `地址 $${address.toString(16).toUpperCase()} 在布局里，要给段号`);
    return null;
  }
  if (segmentId !== undefined) {
    const segment = getSegment(layout, segmentId);
    if (segment === undefined || address < segment.address || address >= segment.address + segment.length) {
      fail(code, `段 ${segmentId} 不存在或不包含地址 $${address.toString(16).toUpperCase()}`);
    }
    return segmentId;
  }
  if (candidates.length === 0) return null;
  if (candidates.length > 1) fail(code, `地址 $${address.toString(16).toUpperCase()} 在多个 bank 里都有映射（段 ${candidates.map((segment) => segment.id).join('、')}），要给段号`);
  return candidates[0].id;
}

/**
 * 检查并规范化符号。
 * @param {import('./layout.mjs').Layout} layout 布局
 * @param {{name: string, address: number, segmentId?: number|null, kind?: string, description?: string, module?: string|null, auto?: boolean}} input 符号
 * @returns {Readonly<{name: string, address: number, segmentId: number|null, kind: string, description: string, module: string|null, auto: boolean}>} 符号
 * @throws {FcdevError} 名称、地址、种类或模块名不合法
 */
export function normalizeSymbol(layout, input) {
  assertSymbolName(input.name);
  const segmentId = resolveSegmentId(layout, input.address, input.segmentId, 'DISASM_SYMBOL_INVALID');
  const kind = input.kind ?? (segmentId === null ? 'variable' : 'label');
  if (!SYMBOL_KINDS.includes(kind)) fail('DISASM_SYMBOL_INVALID', `未知的符号种类：${kind}`);
  const description = input.description ?? '';
  const module = input.module ?? null;
  if (typeof description !== 'string') fail('DISASM_SYMBOL_INVALID', '符号的说明必须是字符串');
  if (module !== null && (typeof module !== 'string' || !MODULE_NAME.test(module))) fail('DISASM_SYMBOL_INVALID', `模块名不合法：${String(module)}`);
  return Object.freeze({ name: input.name, address: input.address, segmentId, kind, description, module, auto: input.auto === true });
}

/**
 * 检查并规范化注释。
 * @param {import('./layout.mjs').Layout} layout 布局
 * @param {{address: number, segmentId?: number, kind: string, text: string}} input 注释
 * @returns {Readonly<{segmentId: number, address: number, kind: string, text: string}>} 注释
 * @throws {FcdevError} 地址不在布局里、种类不合法、行尾注释含换行、文字为空
 */
export function normalizeComment(layout, input) {
  const segmentId = resolveSegmentId(layout, input.address, input.segmentId, 'DISASM_COMMENT_INVALID');
  if (segmentId === null) fail('DISASM_COMMENT_INVALID', `注释的地址 $${input.address.toString(16).toUpperCase()} 不在布局里`);
  if (!COMMENT_KINDS.includes(input.kind)) fail('DISASM_COMMENT_INVALID', `未知的注释种类：${String(input.kind)}`);
  const text = typeof input.text === 'string' ? input.text.replace(/\r\n?/g, '\n').replace(/[\u0000-\u0009\u000b-\u001f]/g, ' ').trimEnd() : '';
  if (text === '') fail('DISASM_COMMENT_INVALID', '注释的文字不能为空');
  if (input.kind === 'inline' && text.includes('\n')) fail('DISASM_COMMENT_INVALID', '行尾注释不能换行');
  return Object.freeze({ segmentId, address: input.address, kind: input.kind, text });
}

/**
 * 检查并规范化操作数提示。
 * @param {import('./layout.mjs').Layout} layout 布局
 * @param {{address: number, segmentId?: number, form: string, symbol?: string, offset?: number}} input 提示
 * @returns {Readonly<{segmentId: number, address: number, form: string, symbol: string|null, offset: number}>} 提示
 * @throws {FcdevError} 地址不在布局里、写法不合法、symbol 的名字不合法
 */
export function normalizeHint(layout, input) {
  const segmentId = resolveSegmentId(layout, input.address, input.segmentId, 'DISASM_HINT_INVALID');
  if (segmentId === null) fail('DISASM_HINT_INVALID', `提示的地址 $${input.address.toString(16).toUpperCase()} 不在布局里`);
  if (!HINT_FORMS.includes(input.form)) fail('DISASM_HINT_INVALID', `未知的操作数写法：${String(input.form)}`);
  const symbol = input.symbol ?? null;
  const offset = input.offset ?? 0;
  if (symbol !== null) assertSymbolName(symbol);
  if (!Number.isInteger(offset) || Math.abs(offset) > 0xffff) fail('DISASM_HINT_INVALID', '偏移必须是整数');
  if ((input.form === 'low' || input.form === 'high') && symbol === null) fail('DISASM_HINT_INVALID', `${input.form} 要指明符号`);
  return Object.freeze({ segmentId, address: input.address, form: input.form, symbol, offset });
}

/**
 * 检查并规范化模块。
 * @param {import('./layout.mjs').Layout} layout 布局
 * @param {{name: string, description?: string, ranges?: Array<{segmentId?: number, address: number, length: number}>}} input 模块
 * @returns {{name: string, description: string, ranges: Array<{segmentId: number, address: number, length: number}>}} 模块
 * @throws {FcdevError} 名称不合法，范围不在段内或重叠
 */
export function normalizeModule(layout, input) {
  if (typeof input.name !== 'string' || !MODULE_NAME.test(input.name)) fail('DISASM_MODULE_INVALID', `模块名不合法：${String(input.name)}（只用字母、数字、下划线与连字符）`);
  const ranges = [];
  for (const range of input.ranges ?? []) {
    const segmentId = resolveSegmentId(layout, range.address, range.segmentId, 'DISASM_MODULE_INVALID');
    const segment = segmentId === null ? undefined : getSegment(layout, segmentId);
    if (segment === undefined) fail('DISASM_MODULE_INVALID', `模块 ${input.name} 的范围 $${range.address.toString(16).toUpperCase()} 不在布局里`);
    if (!Number.isInteger(range.length) || range.length < 1 || range.address + range.length > segment.address + segment.length) {
      fail('DISASM_MODULE_INVALID', `模块 ${input.name} 的范围（$${range.address.toString(16).toUpperCase()}，长度 ${range.length}）超出段 ${segmentId}`);
    }
    ranges.push({ segmentId, address: range.address, length: range.length });
  }
  return { name: input.name, description: input.description ?? '', ranges };
}

/**
 * 检查各模块的范围互不重叠。
 * @param {Array<{name: string, ranges: Array<{segmentId: number, address: number, length: number}>}>} modules 模块
 * @throws {FcdevError} 模块重名，或两个范围重叠
 */
export function assertModulesDisjoint(modules) {
  const names = new Set();
  const all = [];
  for (const module of modules) {
    if (names.has(module.name)) fail('DISASM_MODULE_INVALID', `模块 ${module.name} 重复`);
    names.add(module.name);
    for (const range of module.ranges) all.push({ ...range, module: module.name });
  }
  for (let first = 0; first < all.length; first += 1) {
    for (let second = first + 1; second < all.length; second += 1) {
      const a = all[first];
      const b = all[second];
      if (a.segmentId === b.segmentId && a.address < b.address + b.length && b.address < a.address + a.length) {
        fail('DISASM_MODULE_INVALID', `模块 ${a.module} 与 ${b.module} 的范围在段 ${a.segmentId} 里重叠（$${Math.max(a.address, b.address).toString(16).toUpperCase()} 附近）`);
      }
    }
  }
}
