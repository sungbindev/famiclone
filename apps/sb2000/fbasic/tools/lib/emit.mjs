// ------------------------------------------------------------------------
// 名称：emit.mjs
// 说明：把分析模型里的条目写成 ca65 源码行：标签、指令、数据、外部符号与指向条目中间的地址等式；lift.mjs（原样还原）与 port.mjs（分页移植）共用。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-16
// 备注：钩子 hooks.codeOperand(item, target) 可以改写 jsr/jmp 的操作数，hooks.tableWord(address, value) 可以改写地址表里的字，
//       hooks.instruction(item) 可以整条替换一条指令的文字（返回空串表示删掉这条指令，只留标签），hooks.mapAddress(address, item) 可以改写指令里的非代码地址（零页与 RAM），返回 undefined 表示不改。
// ------------------------------------------------------------------------
import { OPCODES } from '../../../../../packages/fcdev/src/disasm/opcode-table.mjs';
import { BASE, collectTargets, dataElements, hex2, hex4 } from './image-model.mjs';
import { addressingMode, formatInstruction } from './opcode-modes.mjs';

/**
 * 写一串文字字节：可见的 ASCII 连成引号字符串，其余写数值；遇到 0 结束一行。
 * @param {number[]} values 字节
 * @returns {string[]} 若干行 .byte
 */
function textLines(values) {
  const result = [];
  let pieces = [];
  let quoted = '';
  const flushQuoted = () => { if (quoted !== '') { pieces.push(`"${quoted}"`); quoted = ''; } };
  const flushLine = () => { flushQuoted(); if (pieces.length > 0) { result.push(`        .byte   ${pieces.join(', ')}`); pieces = []; } };
  for (const value of values) {
    if (value >= 0x20 && value < 0x7f && value !== 0x22) {
      quoted += String.fromCharCode(value);
      if (quoted.length >= 40) flushQuoted();
    } else {
      flushQuoted();
      pieces.push(`$${hex2(value)}`);
      if (value === 0 || pieces.length >= 12) flushLine();
    }
  }
  flushLine();
  return result;
}

/**
 * 建立写源码用的上下文。
 * @param {object} model loadImageModel 的结果
 * @param {{codeOperand?: Function, tableWord?: Function}} hooks 改写钩子
 * @returns {object} labelName、targets、equates、labeled、externals、emitItem(index)、preludeLines()
 */
export function createEmitter(model, hooks = {}) {
  const { bytes, inImage, items, symbols, immediates, itemIndexAt } = model;
  const targets = collectTargets(model);
  const splitPoints = new Map();
  const equates = [];
  const labeled = new Set();
  for (const address of [...targets.keys()].sort((left, right) => left - right)) {
    const index = itemIndexAt(address);
    const item = items[index];
    if (item.start === address) { labeled.add(address); continue; }
    const splittable = item.type === 'data' && ['fill', 'bytes', 'text'].includes(item.range.kind);
    const boundary = item.type === 'data' && !splittable && dataElements(item.range).some((element) => element.start === address);
    if (splittable || boundary) {
      if (!splitPoints.has(index)) splitPoints.set(index, new Set());
      splitPoints.get(index).add(address);
      labeled.add(address);
    } else {
      labeled.add(item.start);
      equates.push({ address, head: item.start });
    }
  }

  const labelName = (address) => symbols.get(address)?.name ?? `L${hex4(address)}`;
  const externals = new Map();
  const externalName = (address, width) => {
    const symbol = symbols.get(address);
    if (symbol) { externals.set(address, symbol); return symbol.name; }
    return `$${address.toString(16).toUpperCase().padStart(width, '0')}`;
  };
  const addressExpression = (address, width = 4) => {
    if (typeof address === 'string') return address;
    return inImage(address) ? labelName(address) : externalName(address, width);
  };

  /** 写一条指令。 */
  function instructionLine(item) {
    const offset = item.start - BASE;
    const mode = addressingMode(item.opcode);
    let operand = '';
    let forceAbsolute = false;
    if (mode === 'imm') {
      const mapping = immediates.get(item.start);
      if (mapping) {
        const base = labelName(mapping.target);
        operand = `${mapping.half === 'lo' ? '<' : '>'}${mapping.offset ? `(${base}+${mapping.offset})` : base}`;
      } else operand = `$${hex2(bytes[offset + 1])}`;
    } else if (mode === 'zp' || mode === 'zpx' || mode === 'zpy' || mode === 'izx' || mode === 'izy') {
      operand = addressExpression(hooks.mapAddress?.(bytes[offset + 1], item) ?? bytes[offset + 1], 2);
    } else if (mode === 'abs' || mode === 'abx' || mode === 'aby' || mode === 'ind') {
      const original = bytes.readUInt16LE(offset + 1);
      const value = inImage(original) ? original : (hooks.mapAddress?.(original, item) ?? original);
      operand = addressExpression(value);
      const mnemonic = OPCODES[item.opcode][0];
      if ((mnemonic === 'jsr' || mnemonic === 'jmp') && mode === 'abs' && inImage(value)) operand = hooks.codeOperand?.(item, value) ?? operand;
      forceAbsolute = typeof value === 'number' && value < 0x100;
    } else if (mode === 'rel') {
      operand = addressExpression((item.start + 2 + bytes.readInt8(offset + 1)) & 0xffff);
    }
    return formatInstruction(item.opcode, operand, forceAbsolute);
  }

  /** 写一段数据（已经按标签拆好的一块）。 */
  function emitData(range, start, end, out) {
    const slice = (from, to) => [...bytes.subarray(from - BASE, to - BASE + 1)];
    if (range.kind === 'fill') {
      out.push(`        .res    ${end - start + 1}, $${hex2(bytes[start - BASE])}`);
    } else if (range.kind === 'bytes') {
      const values = slice(start, end);
      for (let at = 0; at < values.length; at += 16) out.push(`        .byte   ${values.slice(at, at + 16).map((value) => `$${hex2(value)}`).join(',')}`);
    } else if (range.kind === 'text') {
      out.push(...textLines(slice(start, end)));
    } else {
      const elements = dataElements(range).filter((element) => element.start >= start && element.start <= end);
      let pending = [];
      let pendingKind = null;
      const flush = () => {
        if (pending.length === 0) return;
        const directive = pendingKind === 'b' ? '.byte' : pendingKind === 'a' ? '.addr' : '.word';
        for (let at = 0; at < pending.length; at += 8) out.push(`        ${directive.padEnd(8)}${pending.slice(at, at + 8).join(',')}`);
        pending = [];
      };
      for (const element of elements) {
        if (element.kind !== pendingKind) { flush(); pendingKind = element.kind; }
        if (element.kind === 'b') pending.push(`$${hex2(bytes[element.start - BASE])}`);
        else {
          const value = bytes.readUInt16LE(element.start - BASE);
          pending.push(element.kind === 'a' ? (hooks.tableWord?.(element.start, value) ?? addressExpression(value)) : `$${hex4(value)}`);
        }
      }
      flush();
    }
  }

  /**
   * 写第 index 个条目的全部源码行。
   * @param {number} index 条目下标
   * @returns {string[]} 源码行
   */
  function emitItem(index) {
    const item = items[index];
    const out = [];
    if (item.type === 'data') {
      const cuts = [...(splitPoints.get(index) ?? [])].sort((left, right) => left - right);
      const edges = [item.start, ...cuts, item.start + item.size];
      for (let part = 0; part + 1 < edges.length; part += 1) {
        if (labeled.has(edges[part])) out.push(`${labelName(edges[part])}:`);
        emitData(item.range, edges[part], edges[part + 1] - 1, out);
      }
      return out;
    }
    const label = labeled.has(item.start) ? `${labelName(item.start)}:` : '';
    const patched = item.type === 'code' ? hooks.instruction?.(item) : undefined;
    if (patched === '') { if (label !== '') out.push(label); return out; }
    const text = patched ?? (item.type === 'skip' ? `.byte   $${hex2(bytes[item.start - BASE])}` : instructionLine(item));
    out.push(label === '' ? `        ${text}` : `${label.padEnd(8)}${text}`);
    return out;
  }

  /** 外部符号与指向条目中间的地址等式（要在全部条目写完之后调用）。 */
  function preludeLines() {
    const prelude = [];
    for (const [address, symbol] of [...externals].sort((left, right) => left[0] - right[0])) {
      prelude.push(`${symbol.name.padEnd(24)}:= $${address.toString(16).toUpperCase().padStart(address < 0x100 ? 2 : 4, '0')}${symbol.note ? `  ; ${symbol.note}` : ''}`);
    }
    if (equates.length > 0) {
      prelude.push('', '; 指向指令或数据中间的地址');
      for (const equate of [...equates].sort((left, right) => left.address - right.address)) {
        prelude.push(`${labelName(equate.address).padEnd(24)}:= ${labelName(equate.head)}+${equate.address - equate.head}`);
      }
    }
    return prelude;
  }

  return { targets, labelName, labeled, equates, externals, emitItem, preludeLines };
}
