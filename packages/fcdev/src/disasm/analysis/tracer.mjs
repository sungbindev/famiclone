// ------------------------------------------------------------------------
// 名称：tracer.mjs
// 说明：静态追踪：从入口出发沿控制流标出哪些字节是指令的操作码、哪些是操作数；追完后再从地址表里找新入口，直到没有新表。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-21
// 备注：处理规则：顺序执行继续，条件分支两路都追，JMP 只追目标，JSR 追目标并假定会返回，RTS、RTI、BRK、间接 JMP 结束这一路；
//       遇到非官方操作码、超出段尾或与已识别指令重叠时放弃这一路并记入警告。没被追到的字节保持 unknown。
//       换页窗口：同一个窗口里的转移留在当前 bank；从固定区转移到换页窗口、无法确定 bank 时登记为 cross-bank，不追下去。
//       目标不在任何段里（如跳到 RAM）记入 outside。分类按映像文件的偏移索引；分析结果可以多次追踪累积。
// ------------------------------------------------------------------------
import { decode } from '../cpu/index.mjs';
import { addressToOffset, getSegment } from '../model/index.mjs';
import { createAddressSpace } from './address-space.mjs';
import { BYTE_KIND } from './kinds.mjs';
import { findAddressTables, markWordTable } from './tables.mjs';

/**
 * @typedef {object} Analysis
 * @property {Uint8Array} kinds 每个字节的分类，按文件偏移索引，见 BYTE_KIND
 * @property {Map<number, {segmentId: number, address: number, size: number}>} instructions 已识别的指令：文件偏移到所在的段、CPU 地址与长度
 * @property {import('./tables.mjs').AddressTable[]} tables 已识别的地址表
 * @property {Array<{segmentId: number, address: number, code: string, message: string}>} warnings 放弃某一路的原因
 * @property {Array<UnresolvedItem>} unresolved 待处理的间接跳转与跨 bank 转移
 * @property {Array<{segmentId: number, address: number, target: number, flow: string}>} outside 目标不在任何段里的转移
 * @property {Set<string>} seenTables 已返回过的表的标识
 * @property {Set<string>} seenKeys 已登记过的 unresolved、outside 的标识
 */

/**
 * @typedef {object} UnresolvedItem
 * @property {'indirect-jump'|'cross-bank'} kind 种类：间接 JMP 没有找到地址表；转移的目标 bank 无法确定
 * @property {number} segmentId 转移指令所在的段
 * @property {number} address 转移指令的 CPU 地址
 * @property {number} [offset] 转移指令的文件偏移
 * @property {number} [pointer] indirect-jump：JMP (指针) 的指针地址
 * @property {number} [target] cross-bank：目标 CPU 地址
 * @property {number[]} [candidates] cross-bank：覆盖目标地址的段号
 */

/**
 * @typedef {object} TraceSummary
 * @property {number} instructions 本次新识别的指令条数
 * @property {number} tables 本次新识别的地址表数
 * @property {number} warnings 本次新增的警告数
 */

/**
 * 创建空的分析结果。
 * @param {import('../model/layout.mjs').Layout} layout 布局
 * @returns {Analysis} 分析结果
 */
export function createAnalysis(layout) {
  return {
    kinds: new Uint8Array(layout.imageSize),
    instructions: new Map(),
    tables: [],
    warnings: [],
    unresolved: [],
    outside: [],
    seenTables: new Set(),
    seenKeys: new Set(),
  };
}

/**
 * 十六进制写出地址。
 * @param {number} address 地址
 * @returns {string} 如 $C000
 */
function hex(address) {
  return `$${address.toString(16).toUpperCase().padStart(4, '0')}`;
}

/**
 * 从入口出发追踪代码，结果累积到 analysis 里。
 * @param {Analysis} analysis 分析结果，会被修改
 * @param {{bytes: Uint8Array, layout: import('../model/layout.mjs').Layout}} image 映像的全部字节与布局
 * @param {ReadonlyArray<{segmentId: number, address: number}>} entries 入口
 * @returns {TraceSummary} 本次追踪的新增数量
 */
export function traceCode(analysis, { bytes, layout }, entries) {
  const space = createAddressSpace(layout);
  const { kinds } = analysis;
  const before = { instructions: analysis.instructions.size, tables: analysis.tables.length, warnings: analysis.warnings.length };
  const pending = entries.map((entry) => ({ segmentId: entry.segmentId, address: entry.address }));

  const warn = (segmentId, address, code, message) => analysis.warnings.push({ segmentId, address, code, message: `${hex(address)}：${message}` });

  const recordOnce = (key, list, item) => {
    if (analysis.seenKeys.has(key)) return;
    analysis.seenKeys.add(key);
    list.push(item);
  };

  /** 把控制转移的目标放进待办；目标不在段里或 bank 不确定时登记 */
  const schedule = (segment, from, target, flow, offset) => {
    const resolved = space.resolve(target, segment.id);
    if (resolved.status === 'ok') {
      pending.push({ segmentId: resolved.segment.id, address: target });
    } else if (resolved.status === 'unmapped') {
      recordOnce(`o:${segment.id}:${from}:${target}`, analysis.outside, { segmentId: segment.id, address: from, target, flow });
    } else {
      recordOnce(`x:${segment.id}:${from}:${target}`, analysis.unresolved, {
        kind: 'cross-bank', segmentId: segment.id, address: from, offset, target, candidates: resolved.candidates.map((item) => item.id),
      });
    }
  };

  /** 沿着顺序执行一路走下去 */
  const walk = (start) => {
    let segment = getSegment(layout, start.segmentId);
    let address = start.address;
    for (;;) {
      if (address > 0xffff) return;
      if (address < segment.address || address >= segment.address + segment.length) {
        const next = space.resolve(address, segment.id);
        if (next.status !== 'ok') {
          warn(segment.id, address, 'segment-end', next.status === 'unmapped' ? '顺序执行到了段的末尾，后面没有映射，放弃这一路' : '顺序执行到了段的末尾，后面是换页窗口，无法确定 bank，放弃这一路');
          return;
        }
        segment = next.segment;
      }
      const offset = addressToOffset(segment, address);
      if (kinds[offset] === BYTE_KIND.OPCODE) return;
      const instruction = decode(bytes, offset, address);
      if (!instruction.ok) {
        if (instruction.reason === 'illegal') warn(segment.id, address, 'illegal', `操作码 $${instruction.opcode.toString(16).toUpperCase().padStart(2, '0')} 不是官方 6502 指令，放弃这一路`);
        else warn(segment.id, address, 'truncated', '指令超出映像末尾，放弃这一路');
        return;
      }
      if (offset + instruction.size > segment.fileOffset + segment.length) {
        warn(segment.id, address, 'segment-end', '指令超出段尾，放弃这一路');
        return;
      }
      // 操作码与操作数位置必须都是未知，不能压在别的指令或表上
      for (let index = 0; index < instruction.size; index += 1) {
        if (kinds[offset + index] !== BYTE_KIND.UNKNOWN) {
          warn(segment.id, address, 'overlap', '与已识别的指令重叠，放弃这一路');
          return;
        }
      }
      kinds[offset] = BYTE_KIND.OPCODE;
      for (let index = 1; index < instruction.size; index += 1) kinds[offset + index] = BYTE_KIND.OPERAND;
      analysis.instructions.set(offset, { segmentId: segment.id, address, size: instruction.size });

      if (instruction.flow === 'branch' || instruction.flow === 'jump' || instruction.flow === 'call') schedule(segment, address, instruction.target, instruction.flow, offset);
      if (instruction.flow === 'jump-indirect') {
        recordOnce(`i:${offset}`, analysis.unresolved, { kind: 'indirect-jump', segmentId: segment.id, address, offset, pointer: instruction.reference });
      }
      if (instruction.flow === 'jump' || instruction.flow === 'jump-indirect' || instruction.flow === 'return' || instruction.flow === 'stop') return;
      address += instruction.size;
    }
  };

  /** 找新的地址表，表里的目标成为新入口；用这些表分发的间接 JMP 不再算未解析 */
  const discoverTables = () => {
    const tables = findAddressTables(analysis, { bytes, space });
    for (const table of tables) {
      markWordTable(kinds, table);
      analysis.tables.push(table);
      const segment = getSegment(layout, table.segmentId);
      for (const target of table.targets) schedule(segment, table.address, target, 'table', table.offset);
      analysis.unresolved = analysis.unresolved.filter((item) => !(item.kind === 'indirect-jump' && table.jumpOffsets.includes(item.offset)));
    }
    return tables.length > 0;
  };

  while (pending.length > 0 || discoverTables()) walk(pending.pop());
  return {
    instructions: analysis.instructions.size - before.instructions,
    tables: analysis.tables.length - before.tables,
    warnings: analysis.warnings.length - before.warnings,
  };
}
