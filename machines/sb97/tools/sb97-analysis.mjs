// ------------------------------------------------------------------------
// 名称：sb97-analysis.mjs
// 说明：SB97 BIOS 的静态分析：在一个视图（哪一页在哪个地址）里从入口出发顺着控制流找出代码，再统计对 $2000-$7FFF 的访问。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-17
// 备注：只认官方指令，遇到非官方指令、BRK（多半是 $00 的数据）、与已分析指令重叠的字节就停下并记入 rejected；
//       控制流只跟 JSR、JMP 绝对、条件分支；JMP 间接与 RTS 之后不再往下走，间接跳转的目标要另给入口（见 sb97-pages.mjs）；
//       常量追踪只在同一段连续指令里跟踪 A、X、Y 的立即数，遇到跳转目标、JSR、非连续地址就清空。
// ------------------------------------------------------------------------
import { decodeInstruction, endsFlow, isBranch } from '../../sb2000/disassembly/tools/disasm.mjs';
import { WINDOW_END, WINDOW_START } from './sb97-memory.mjs';

/** 窗口里的字节数（$8000-$FFFF） */
const SPAN = WINDOW_END - WINDOW_START + 1;

/** 字节还没有被当作代码 */
export const KIND_UNKNOWN = 0;
/** 字节是一条指令的第一个字节 */
export const KIND_OPCODE = 1;
/** 字节是一条指令的操作数 */
export const KIND_OPERAND = 2;

/**
 * @typedef {object} Seed
 * @property {number} address 入口地址
 * @property {string} [name] 入口的名字，列表里显示
 */

/**
 * @typedef {object} Analysis
 * @property {import('./sb97-memory.mjs').MemoryView} view 分析用的视图
 * @property {Uint8Array} kind 每个字节的归类，下标是地址减 $8000
 * @property {Map<number, ReturnType<typeof decodeInstruction>>} instructions 以指令地址为键的解码结果
 * @property {Map<number, Set<string>>} references 被引用的地址与引用方式（seed、jsr、jmp、branch）
 * @property {Map<number, string>} names 入口的名字
 * @property {{pc: number, reason: string}[]} rejected 停下来的位置与原因
 * @property {Set<number>} externalTargets 跳到视图之外的目标地址
 * @property {Map<number, number>} tableWords JSR 后面内嵌的跳转表：字所在的地址与它指向的目标
 * @property {number} codeBytes 被当作代码的字节数
 */

/** 内嵌跳转表最多读几个字 */
const MAX_TABLE_WORDS = 64;
/** 内嵌字符串最多找多少字节的结尾 $00 */
const MAX_INLINE_STRING = 256;

/**
 * 地址上是不是一条能当入口的官方指令。
 * @param {import('./sb97-memory.mjs').MemoryView} view 视图
 * @param {number} address 地址
 * @returns {boolean}
 */
export function isPlausibleEntry(view, address) {
  if (!view.has(address)) return false;
  const ins = decodeInstruction(view.read, address);
  return ins.info.official && ins.info.mnemonic !== 'BRK' && view.has(address + ins.length - 1);
}

/**
 * 一个子程序是不是“按 JSR 后面内嵌的字表跳转”：开头几条指令里连着弹两次返回地址再存进零页（PLA、STA、PLA、STA）。
 * @param {import('./sb97-memory.mjs').MemoryView} view 视图
 * @param {number} address 子程序的地址
 * @returns {boolean}
 */
export function isInlineTableDispatcher(view, address) {
  if (!view.has(address)) return false;
  const mnemonics = [];
  let pc = address;
  for (let i = 0; i < 12 && view.has(pc + 2); i += 1) {
    const ins = decodeInstruction(view.read, pc);
    mnemonics.push(ins.info.mnemonic);
    if (endsFlow(ins)) break;
    pc += ins.length;
  }
  for (let i = 0; i + 3 < mnemonics.length; i += 1) {
    if (mnemonics[i] === 'PLA' && mnemonics[i + 1] === 'STA' && mnemonics[i + 2] === 'PLA' && mnemonics[i + 3] === 'STA') return true;
  }
  return false;
}

/**
 * 从入口出发找出代码。
 * @param {import('./sb97-memory.mjs').MemoryView} view 视图
 * @param {Seed[]} seeds 入口
 * @returns {Analysis}
 */
export function analyzeView(view, seeds) {
  const kind = new Uint8Array(SPAN);
  const instructions = new Map();
  const references = new Map();
  const names = new Map();
  const rejected = [];
  const externalTargets = new Set();
  const tableWords = new Map();
  const queue = [];

  const addReference = (address, how) => {
    if (!references.has(address)) references.set(address, new Set());
    references.get(address).add(how);
  };
  const follow = (address, how) => {
    if (view.has(address)) {
      addReference(address, how);
      queue.push(address);
    } else {
      externalTargets.add(address);
    }
  };

  for (const seed of seeds) {
    if (seed.name && !names.has(seed.address)) names.set(seed.address, seed.name);
    follow(seed.address, 'seed');
  }

  while (queue.length > 0) {
    let pc = queue.pop();
    for (;;) {
      if (!view.has(pc)) break;
      const offset = pc - WINDOW_START;
      if (kind[offset] === KIND_OPCODE) break;
      if (kind[offset] === KIND_OPERAND) {
        rejected.push({ pc, reason: '落在另一条指令中间' });
        break;
      }
      const ins = decodeInstruction(view.read, pc);
      if (!ins.info.official || ins.info.mnemonic === 'BRK') {
        rejected.push({ pc, reason: ins.info.mnemonic === 'BRK' ? 'BRK（多半是数据）' : `非官方指令 ${ins.info.mnemonic}` });
        break;
      }
      const last = pc + ins.length - 1;
      if (last > WINDOW_END || !view.has(last)) {
        rejected.push({ pc, reason: '指令超出映射范围' });
        break;
      }
      let overlap = false;
      for (let i = 1; i < ins.length; i += 1) if (kind[offset + i] !== KIND_UNKNOWN) overlap = true;
      if (overlap) {
        rejected.push({ pc, reason: '与已分析的指令重叠' });
        break;
      }
      kind[offset] = KIND_OPCODE;
      for (let i = 1; i < ins.length; i += 1) kind[offset + i] = KIND_OPERAND;
      instructions.set(pc, ins);

      const mnemonic = ins.info.mnemonic;
      if (mnemonic === 'JSR') {
        follow(ins.target, 'jsr');
        if (isInlineTableDispatcher(view, ins.target)) {
          // 调用之后紧跟一张字表，子程序按下标取其中一项当跳转目标，表之后才是接着执行的代码
          let address = pc + ins.length;
          for (let count = 0; count < MAX_TABLE_WORDS && view.has(address + 1); count += 1) {
            const target = view.read(address) | (view.read(address + 1) << 8);
            if (!isPlausibleEntry(view, target)) break;
            tableWords.set(address, target);
            follow(target, 'table');
            address += 2;
          }
          if (address === pc + ins.length) {
            // 一项都不是代码地址：多半是内嵌的字符串，以 $00 结尾
            for (let i = 0; i < MAX_INLINE_STRING && view.has(address + i); i += 1) {
              if (view.read(address + i) === 0) {
                address += i + 1;
                break;
              }
            }
          }
          pc = address;
          continue;
        }
      } else if (mnemonic === 'JMP' && ins.target !== null) follow(ins.target, 'jmp');
      else if (isBranch(ins)) follow(ins.target, 'branch');
      if (endsFlow(ins)) break;
      pc += ins.length;
    }
  }

  let codeBytes = 0;
  for (const value of kind) if (value !== KIND_UNKNOWN) codeBytes += 1;
  return { view, kind, instructions, references, names, rejected, externalTargets, tableWords, codeBytes };
}

/**
 * 按地址从小到大列出已分析的指令。
 * @param {Analysis} analysis 分析结果
 * @returns {ReturnType<typeof decodeInstruction>[]}
 */
export function orderedInstructions(analysis) {
  return [...analysis.instructions.values()].sort((a, b) => a.pc - b.pc);
}

/** 只写不读的助记符 */
const WRITE_ONLY = new Set(['STA', 'STX', 'STY', 'SAX', 'AHX', 'TAS', 'SHX', 'SHY']);
/** 读后写回的助记符 */
const READ_MODIFY_WRITE = new Set(['INC', 'DEC', 'ASL', 'LSR', 'ROL', 'ROR', 'SLO', 'RLA', 'SRE', 'RRA', 'DCP', 'ISC']);
/** 会改变 A 的助记符（累加器寻址的移位另行判断） */
const CHANGES_A = new Set(['LDA', 'ADC', 'SBC', 'AND', 'ORA', 'EOR', 'PLA', 'TXA', 'TYA', 'LAX']);
const CHANGES_X = new Set(['LDX', 'TAX', 'TSX', 'INX', 'DEX', 'LAX', 'AXS']);
const CHANGES_Y = new Set(['LDY', 'TAY', 'INY', 'DEY']);
/** 带地址的寻址方式里，参与 I/O 统计的三种 */
const ABSOLUTE_MODES = new Set(['abs', 'abx', 'aby']);

/** 统计的最低地址：$2000 起是 PPU、APU、整机寄存器与主板上的存储 */
export const ACCESS_FIRST = 0x2000;
/** 统计的最高地址（含）：$7FFF，$8000 起是程序窗口 */
export const ACCESS_LAST = 0x7fff;

/**
 * @typedef {object} Access
 * @property {number} pc 访问所在的指令地址
 * @property {number} address 操作数里的绝对地址
 * @property {string} mnemonic 助记符
 * @property {'read'|'write'|'rw'} access 读、写或读后写回
 * @property {boolean} indexed 是否带 X 或 Y 的下标
 * @property {number|undefined} value 写入的值（只有常量追踪得到时才有）
 */

/**
 * 统计对 $2000-$7FFF 的访问，并用常量追踪给写入加上立即数的值。
 * @param {Analysis} analysis 分析结果
 * @returns {Access[]}
 */
export function collectAccesses(analysis) {
  const accesses = [];
  const branchTargets = new Set(analysis.references.keys());
  let known = { a: undefined, x: undefined, y: undefined };
  let expected = -1;
  for (const ins of orderedInstructions(analysis)) {
    if (ins.pc !== expected || branchTargets.has(ins.pc)) known = { a: undefined, x: undefined, y: undefined };
    expected = ins.pc + ins.length;
    const { mnemonic, mode } = ins.info;

    if (ABSOLUTE_MODES.has(mode) && ins.operand >= ACCESS_FIRST && ins.operand <= ACCESS_LAST && mnemonic !== 'JMP' && mnemonic !== 'JSR') {
      const access = WRITE_ONLY.has(mnemonic) ? 'write' : READ_MODIFY_WRITE.has(mnemonic) ? 'rw' : 'read';
      let value;
      if (mnemonic === 'STA') value = known.a;
      else if (mnemonic === 'STX') value = known.x;
      else if (mnemonic === 'STY') value = known.y;
      accesses.push({ pc: ins.pc, address: ins.operand, mnemonic, access, indexed: mode !== 'abs', value });
    }

    // 更新追踪的寄存器
    if (mnemonic === 'LDA' && mode === 'imm') known.a = ins.operand;
    else if (CHANGES_A.has(mnemonic) || (mode === 'acc' && ['ASL', 'LSR', 'ROL', 'ROR'].includes(mnemonic))) known.a = undefined;
    if (mnemonic === 'LDX' && mode === 'imm') known.x = ins.operand;
    else if (mnemonic === 'TAX') known.x = known.a;
    else if (CHANGES_X.has(mnemonic)) known.x = undefined;
    if (mnemonic === 'LDY' && mode === 'imm') known.y = ins.operand;
    else if (mnemonic === 'TAY') known.y = known.a;
    else if (CHANGES_Y.has(mnemonic)) known.y = undefined;
    if (mnemonic === 'TXA') known.a = known.x;
    if (mnemonic === 'TYA') known.a = known.y;
    if (mnemonic === 'JSR' || endsFlow(ins)) known = { a: undefined, x: undefined, y: undefined };
  }
  return accesses;
}

/**
 * @typedef {object} AccessSummary
 * @property {number} address 地址
 * @property {number} reads 读的次数
 * @property {number} writes 写的次数（含读后写回）
 * @property {number[]} values 写入过的立即数（去重、从小到大）
 * @property {number[]} pcs 前几次访问所在的指令地址
 */

/**
 * 按地址汇总访问。
 * @param {Access[]} accesses collectAccesses 的结果
 * @returns {AccessSummary[]} 按地址从小到大
 */
export function summarizeAccesses(accesses) {
  const byAddress = new Map();
  for (const item of accesses) {
    if (!byAddress.has(item.address)) byAddress.set(item.address, { address: item.address, reads: 0, writes: 0, values: new Set(), pcs: [] });
    const entry = byAddress.get(item.address);
    if (item.access === 'read' || item.access === 'rw') entry.reads += 1;
    if (item.access === 'write' || item.access === 'rw') entry.writes += 1;
    if (item.value !== undefined) entry.values.add(item.value);
    if (entry.pcs.length < 4) entry.pcs.push(item.pc);
  }
  return [...byAddress.values()].map((entry) => ({ ...entry, values: [...entry.values].sort((a, b) => a - b) })).sort((a, b) => a.address - b.address);
}
