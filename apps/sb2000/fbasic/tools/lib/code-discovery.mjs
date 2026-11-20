// ------------------------------------------------------------------------
// 名称：code-discovery.mjs
// 说明：GBASIC 映像的代码发现：先从运行轨迹里执行过的指令地址出发追踪控制流，再在没追到的字节里试探找出没执行过的代码。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-16
// 备注：追踪规则与 fcdev 的 analysis.mjs 一致（顺序执行继续，分支两路都追，JMP 只追目标，JSR 追目标并假定会返回，RTS、RTI、BRK、间接 JMP 结束这一路）。
//       种子（轨迹或人工确认的入口）的路径出错只放弃那一路；试探用严格模式：遇到非法操作码、重叠、分支目标越界或访问非法目标就整体放弃，
//       只有通过严格检查、指令数不少于下限的区段才算代码。试探的结果要人工复核后写进 analysis/code-entries.txt，不自动当作结论。
// ------------------------------------------------------------------------
import { OPCODES } from './opcodes.mjs';

/** 字节分类：0 未知，1 指令的操作码，2 指令的操作数，3 跳过前缀（见 CodeMap 构造函数） */
export const UNKNOWN = 0;
export const OPCODE = 1;
export const OPERAND = 2;
export const SKIP_PREFIX = 3;

/** 允许 JSR、JMP 到映像之外的系统入口范围（步步高 BBGDOS 入口） */
const EXTERNAL_CALL_FIRST = 0x5800;
const EXTERNAL_CALL_LAST = 0x580f;

/**
 * 映像里的字节分类。
 */
export class CodeMap {
  /**
   * @param {Buffer} bytes 映像字节
   * @param {number} base 第一个字节的地址
   * @param {Set<number>} [skipPrefixes] 跳过前缀的地址：原版用 LDX、LDA、BIT 的绝对寻址操作码当作“跳过下一条双字节指令”的前缀，
   *   从前缀处顺序执行时它吞掉后面的两个字节，从后面的字节处进入时才执行那条指令；这里把前缀当作一字节的伪指令，后面的字节照常按指令读
   */
  constructor(bytes, base, skipPrefixes = new Set()) {
    this.bytes = bytes;
    this.base = base;
    this.end = base + bytes.length;
    this.kinds = new Uint8Array(bytes.length);
    this.skipPrefixes = skipPrefixes;
  }

  /**
   * @param {number} address 地址
   * @returns {boolean} 是否在映像内
   */
  contains(address) {
    return address >= this.base && address < this.end;
  }

  /**
   * 沿控制流追踪。严格模式下任何问题都使整次追踪失败且不写入分类；宽松模式只放弃出问题的那一路。
   * @param {number[]} starts 入口地址
   * @param {{strict: boolean, commit: boolean}} options 模式
   * @returns {{ok: boolean, count: number, reason?: string, warnings: string[]}} 结果；count 是新识别的指令条数
   */
  trace(starts, { strict, commit }) {
    const pending = starts.map((start) => [start, start]);
    const local = new Map();
    const warnings = [];
    let count = 0;
    const kindAt = (offset) => (local.has(offset) ? local.get(offset) : this.kinds[offset]);
    const fail = (reason) => {
      warnings.push(reason);
      return strict;
    };

    while (pending.length > 0) {
      let [address, origin] = pending.pop();
      for (;;) {
        if (!this.contains(address)) {
          if (fail(`${hex(address)}：离开映像（来自 ${hex(origin)}）`)) return { ok: false, count: 0, reason: warnings.at(-1), warnings };
          break;
        }
        const offset = address - this.base;
        if (kindAt(offset) === OPCODE || kindAt(offset) === SKIP_PREFIX) break;
        if (this.skipPrefixes.has(address)) {
          if (kindAt(offset) !== UNKNOWN) {
            if (fail(`${hex(address)}：跳过前缀与已识别的字节重叠（来自 ${hex(origin)}）`)) return { ok: false, count: 0, reason: warnings.at(-1), warnings };
            break;
          }
          local.set(offset, SKIP_PREFIX);
          count += 1;
          address += 1;
          continue;
        }
        const entry = OPCODES[this.bytes[offset]];
        if (entry === null) {
          if (fail(`${hex(address)}：非法操作码（来自 ${hex(origin)}）`)) return { ok: false, count: 0, reason: warnings.at(-1), warnings };
          break;
        }
        const [, size, flow] = entry;
        let overlap = offset + size > this.bytes.length;
        for (let index = 0; index < size && !overlap; index += 1) overlap = kindAt(offset + index) !== UNKNOWN;
        if (overlap) {
          if (fail(`${hex(address)}：与已识别的字节重叠（来自 ${hex(origin)}）`)) return { ok: false, count: 0, reason: warnings.at(-1), warnings };
          break;
        }
        local.set(offset, OPCODE);
        for (let index = 1; index < size; index += 1) local.set(offset + index, OPERAND);
        count += 1;

        if (flow === 'branch') {
          pending.push([(address + 2 + this.bytes.readInt8(offset + 1)) & 0xffff, address]);
        } else if (flow === 'jump' || flow === 'call') {
          const target = this.bytes.readUInt16LE(offset + 1);
          const external = target >= EXTERNAL_CALL_FIRST && target <= EXTERNAL_CALL_LAST;
          if (this.contains(target)) pending.push([target, address]);
          else if (!external && strict) return { ok: false, count: 0, reason: `${hex(address)}：调用映像外的 ${hex(target)}`, warnings };
        }
        if (flow === 'jump' || flow === 'jump-indirect' || flow === 'return' || flow === 'stop') break;
        address += size;
      }
    }
    if (commit) for (const [offset, kind] of local) this.kinds[offset] = kind;
    return { ok: true, count, warnings };
  }

  /**
   * 统计分类为指令的字节数。
   * @returns {number} 字节数
   */
  codeBytes() {
    let total = 0;
    for (const kind of this.kinds) if (kind !== UNKNOWN) total += 1;
    return total;
  }

  /**
   * 列出还没分类的连续区段。
   * @returns {{start: number, end: number}[]} 区段，end 含在内
   */
  unknownRanges() {
    const ranges = [];
    for (let offset = 0; offset < this.kinds.length; ) {
      if (this.kinds[offset] !== UNKNOWN) { offset += 1; continue; }
      let last = offset;
      while (last + 1 < this.kinds.length && this.kinds[last + 1] === UNKNOWN) last += 1;
      ranges.push({ start: this.base + offset, end: this.base + last });
      offset = last + 1;
    }
    return ranges;
  }
}

/**
 * 四位十六进制地址。
 * @param {number} address 地址
 * @returns {string} 如 $7B00
 */
export function hex(address) {
  return `$${address.toString(16).toUpperCase().padStart(4, '0')}`;
}
