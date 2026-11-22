// ------------------------------------------------------------------------
// 名称：routines.ts
// 说明：例程的范围、上下文与调用图：例程是“过程”符号开始的、到下一个过程符号为止的连续指令。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-21
// 备注：入口与出口寄存器是按指令顺序线性扫描得到的推测（不跟踪分支）：先被读的寄存器算可能的入口，
//       被写过的算可能的出口；读写的地址只统计指令里直接写出的地址，不含间接寻址指向的内容。
// ------------------------------------------------------------------------
import { FcdevError } from '../../errors/fcdev-error';
import { findXrefs } from '../analysis/index';
import { formatHex, memoryAccess } from '../cpu/index';
import { addressToOffset, expectSegment } from '../model/index';
import { renderListing } from './listing';
import type { Project } from '../model/project';
import type { ProjectSymbol } from '../model/symbols';
import type { CodeScan, ScannedInstruction } from '../analysis/code-scan';
import type { DecodedInstruction } from '../cpu/decode';
import type { SymbolIndex } from './symbol-index';
import type { XrefIndex } from '../analysis/xrefs';

/** 每种助记符读写的寄存器（不含变址寻址带来的读） */
const REGISTER_EFFECTS: Readonly<Record<string, {reads?: string[], writes?: string[]}>> = {
  lda: { writes: ['a'] }, ldx: { writes: ['x'] }, ldy: { writes: ['y'] },
  sta: { reads: ['a'] }, stx: { reads: ['x'] }, sty: { reads: ['y'] },
  tax: { reads: ['a'], writes: ['x'] }, tay: { reads: ['a'], writes: ['y'] }, txa: { reads: ['x'], writes: ['a'] }, tya: { reads: ['y'], writes: ['a'] },
  tsx: { writes: ['x'] }, txs: { reads: ['x'] },
  adc: { reads: ['a'], writes: ['a'] }, sbc: { reads: ['a'], writes: ['a'] }, and: { reads: ['a'], writes: ['a'] }, ora: { reads: ['a'], writes: ['a'] }, eor: { reads: ['a'], writes: ['a'] },
  cmp: { reads: ['a'] }, cpx: { reads: ['x'] }, cpy: { reads: ['y'] }, bit: { reads: ['a'] },
  inx: { reads: ['x'], writes: ['x'] }, dex: { reads: ['x'], writes: ['x'] }, iny: { reads: ['y'], writes: ['y'] }, dey: { reads: ['y'], writes: ['y'] },
  pla: { writes: ['a'] }, pha: { reads: ['a'] },
};

/** 累加器寻址的移位指令读写 A */
const ACCUMULATOR_SHIFTS = new Set(['asl', 'lsr', 'rol', 'ror']);

/** 变址读 X 或 Y 的寻址方式 */
const INDEX_READS: Readonly<Record<string, string>> = { zeroPageX: 'x', absoluteX: 'x', indexedIndirect: 'x', zeroPageY: 'y', absoluteY: 'y', indirectIndexed: 'y' };

export interface Routine {
  /** 过程符号的名字 */
  name: string;
  /** 段号 */
  segmentId: number;
  /** 起始 CPU 地址 */
  address: number;
  /** 结束 CPU 地址（不含） */
  end: number;
  /** 指令条数 */
  instructions: number;
}

/**
 * 列出工程里的例程。
 * @param project 工程
 * @param scan 指令扫描结果
 * @returns 例程，按段号与地址排序
 */
export function listRoutines(project: Project, scan: CodeScan): Routine[] {
  const procedures = project.symbols.filter((symbol): symbol is ProjectSymbol & { segmentId: number } => symbol.kind === 'procedure' && symbol.segmentId !== null).toSorted((a, b) => a.segmentId - b.segmentId || a.address - b.address);
  return procedures.map((symbol, position) => {
    const segment = expectSegment(project.layout, symbol.segmentId);
    const next = procedures[position + 1];
    const limitAddress = next !== undefined && next.segmentId === symbol.segmentId ? next.address : segment.address + segment.length;
    const limitOffset = segment.fileOffset + (limitAddress - segment.address);
    let offset = addressToOffset(segment, symbol.address);
    let instructions = 0;
    while (offset < limitOffset) {
      const item = scan.byOffset.get(offset);
      if (item === undefined || offset + item.size > limitOffset) break;
      offset += item.size;
      instructions += 1;
    }
    return { name: symbol.name, segmentId: symbol.segmentId, address: symbol.address, end: segment.address + (offset - segment.fileOffset), instructions };
  });
}

/**
 * 找出包含某个位置的例程。
 * @param routines 例程
 * @param segmentId 段号
 * @param address CPU 地址
 * @returns 例程；不在任何例程里时为 null
 */
export function enclosingRoutine(routines: Routine[], segmentId: number, address: number): Routine|null {
  return routines.find((routine) => routine.segmentId === segmentId && address >= routine.address && address < Math.max(routine.end, routine.address + 1)) ?? null;
}

/**
 * 按名字或地址找例程。
 * @param routines 例程
 * @param reference 名字，或地址（换页窗口里加段号）
 * @returns 例程
 * @throws FcdevError 找不到，或地址在多个 bank 里都有例程
 */
export function findRoutine(routines: Routine[], { name, address, segmentId }: {name?: string, address?: number, segmentId?: number}): Routine {
  if (name !== undefined) {
    const found = routines.find((routine) => routine.name === name);
    if (found === undefined) throw new FcdevError(`没有名为 ${name} 的例程`, { code: 'DISASM_ROUTINE_NOT_FOUND', hint: '例程是种类为 procedure 的符号；先追踪并自动命名，或用 apply 把符号定义成 procedure' });
    return found;
  }
  if (address === undefined) throw new FcdevError('要给例程名或地址', { code: 'DISASM_PARAM_INVALID' });
  const matches = routines.filter((routine) => address >= routine.address && address < Math.max(routine.end, routine.address + 1) && (segmentId === undefined || routine.segmentId === segmentId));
  if (matches.length === 0) throw new FcdevError(`${formatHex(address, 4)} 不在任何例程里`, { code: 'DISASM_ROUTINE_NOT_FOUND', hint: '例程是种类为 procedure 的符号开始的连续指令' });
  if (matches.length > 1) throw new FcdevError(`${formatHex(address, 4)} 在多个 bank 的例程里（段 ${matches.map((routine) => routine.segmentId).join('、')}）`, { code: 'DISASM_ROUTINE_AMBIGUOUS', hint: '给出段号' });
  return matches[0]!;
}

/**
 * 按 a、x、y 的顺序列出集合里的寄存器。
 * @param set 寄存器集合
 * @returns 寄存器名
 */
function order(set: ReadonlySet<string>): string[] {
  return ['a', 'x', 'y'].filter((register) => set.has(register));
}

/**
 * 推测例程的入口与出口寄存器。
 * @param instructions 例程里按顺序的指令
 * @returns 可能的入口寄存器（先读后写）与出口寄存器（被写过）
 */
export function registerUsage(instructions: Array<DecodedInstruction>): {inputs: string[], outputs: string[]} {
  const touched = new Set<string>();
  const inputs = new Set<string>();
  const outputs = new Set<string>();
  for (const instruction of instructions) {
    const effect = REGISTER_EFFECTS[instruction.mnemonic] ?? {};
    const reads = [...(effect.reads ?? [])];
    const writes = [...(effect.writes ?? [])];
    if (instruction.mode === 'accumulator' && ACCUMULATOR_SHIFTS.has(instruction.mnemonic)) {
      reads.push('a');
      writes.push('a');
    }
    const indexRead = INDEX_READS[instruction.mode];
    if (indexRead !== undefined) reads.push(indexRead);
    for (const register of reads) if (!touched.has(register)) inputs.add(register);
    for (const register of reads) touched.add(register);
    for (const register of writes) {
      touched.add(register);
      outputs.add(register);
    }
  }
  return { inputs: order(inputs), outputs: order(outputs) };
}

/**
 * 描述一个例程的上下文。
 * @param input 工程、映像、符号索引、指令扫描、交叉引用、全部例程、要描述的例程与清单的最大行数
 * @returns 例程的范围、清单、调用者、被调用者、读写的地址与寄存器推测
 */
export function describeRoutine({ project, bytes, index, scan, xrefs, routines, routine, maxLines }: {project: Project, bytes: Uint8Array, index: SymbolIndex, scan: CodeScan, xrefs: XrefIndex, routines: Routine[], routine: Routine, maxLines?: number}): Record<string, unknown> {
  const segment = expectSegment(project.layout, routine.segmentId);
  const startOffset = addressToOffset(segment, routine.address);
  const endOffset = segment.fileOffset + (routine.end - segment.address);
  const items: ScannedInstruction[] = [];
  for (let offset = startOffset; offset < endOffset;) {
    const item = scan.byOffset.get(offset);
    if (item === undefined) break;
    items.push(item);
    offset += item.size;
  }

  const listing = renderListing({ project, bytes, index, segmentId: routine.segmentId, address: routine.address, length: Math.max(1, routine.end - routine.address), maxLines, scan });
  const callers = findXrefs(xrefs, routine.address)
    .filter((ref) => ref.kind === 'call' || ref.kind === 'jump' || ref.kind === 'branch' || ref.kind === 'table' || ref.kind === 'address')
    .map((ref) => ({ kind: ref.kind, address: ref.address, segmentId: ref.segmentId, from: enclosingRoutine(routines, ref.segmentId, ref.address)?.name ?? null }));

  const callees = new Map<number, {address: number, name: string | null, kinds: Set<string>}>();
  const memory = new Map<number, {address: number, name: string | null, read: number, write: number}>();
  for (const item of items) {
    const decoded = item.instruction;
    if ((decoded.flow === 'call' || decoded.flow === 'jump') && decoded.target !== null && (decoded.target < routine.address || decoded.target >= routine.end)) {
      const key = decoded.target;
      if (!callees.has(key)) callees.set(key, { address: key, name: index.nameFor(key, routine.segmentId), kinds: new Set() });
      callees.get(key)!.kinds.add(decoded.flow);
    }
    if (decoded.reference !== null && decoded.flow !== 'call' && decoded.flow !== 'jump') {
      const access = memoryAccess(decoded.mnemonic, decoded.mode);
      if (access === null) continue;
      if (!memory.has(decoded.reference)) memory.set(decoded.reference, { address: decoded.reference, name: index.nameFor(decoded.reference, routine.segmentId), read: 0, write: 0 });
      const entry = memory.get(decoded.reference)!;
      if (access === 'read' || access === 'readWrite') entry.read += 1;
      if (access === 'write' || access === 'readWrite') entry.write += 1;
    }
  }
  return {
    name: routine.name,
    segmentId: routine.segmentId,
    address: routine.address,
    end: routine.end,
    instructions: items.length,
    registers: registerUsage(items.map((item) => item.instruction)),
    callers,
    callees: [...callees.values()].map((callee) => ({ address: callee.address, name: callee.name, kinds: [...callee.kinds] })),
    memory: [...memory.values()].toSorted((a, b) => a.address - b.address),
    listing: listing.text,
    truncated: listing.truncated,
  };
}

/**
 * 建立例程之间的调用图并写成树。
 * @param input 全部例程、交叉引用、根例程、深度与方向（callees 看它调用了谁，callers 看谁调用了它）
 * @returns 树与节点数
 */
export function buildCallGraph({ routines, xrefs, root, depth, direction }: {routines: Routine[], xrefs: XrefIndex, root: Routine, depth: number, direction: 'callees'|'callers'}): {root: string, depth: number, direction: string, nodes: number, text: string} {
  const key = (routine: Routine) => `${routine.segmentId}:${routine.address}`;
  const byKey = new Map(routines.map((routine) => [key(routine), routine]));
  const callees = new Map<string, Set<string>>();
  const callers = new Map<string, Set<string>>();
  const link = (from: string, to: string) => {
    if (!callees.has(from)) callees.set(from, new Set());
    if (!callers.has(to)) callers.set(to, new Set());
    callees.get(from)!.add(to);
    callers.get(to)!.add(from);
  };
  for (const routine of routines) {
    for (const ref of findXrefs(xrefs, routine.address)) {
      if (ref.kind !== 'call' && ref.kind !== 'jump') continue;
      if (ref.targetSegmentId !== null && ref.targetSegmentId !== routine.segmentId) continue;
      const from = enclosingRoutine(routines, ref.segmentId, ref.address);
      if (from !== null && key(from) !== key(routine)) link(key(from), key(routine));
    }
  }
  const edges = direction === 'callers' ? callers : callees;
  const lines: string[] = [];
  const expanded = new Set<string>();
  let nodes = 0;
  const visit = (current: string, level: number) => {
    const routine = byKey.get(current)!;
    const repeated = expanded.has(current);
    nodes += 1;
    lines.push(`${'  '.repeat(level)}${routine.name} (${formatHex(routine.address, 4)})${repeated ? ' …已在上面展开' : ''}`);
    if (repeated || level >= depth) return;
    expanded.add(current);
    const next = [...(edges.get(current) ?? [])].toSorted((a, b) => byKey.get(a)!.address - byKey.get(b)!.address);
    for (const child of next) visit(child, level + 1);
  };
  visit(key(root), 0);
  return { root: root.name, depth, direction, nodes, text: lines.join('\n') };
}
