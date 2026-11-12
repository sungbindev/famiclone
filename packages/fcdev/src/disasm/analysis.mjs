// ------------------------------------------------------------------------
// 名称：analysis.mjs
// 说明：静态追踪：从入口出发沿控制流标出哪些字节是指令的操作码、哪些是操作数；CDL 里的代码段起点也当作入口。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-10
// 备注：处理规则：顺序执行继续，条件分支两路都追，JMP 只追目标，JSR 追目标并假定会返回，RTS、RTI、BRK、间接 JMP 结束这一路；
//       遇到非法操作码、越界或与已识别指令重叠时放弃这一路并记入警告。没被追到的字节都算数据，这不影响回读校验（da65 对非法操作码输出 .byte）。
// ------------------------------------------------------------------------
import { CDL_CODE } from './cdl.mjs';
import { OPCODES } from './opcode-table.mjs';

/** 字节分类：没有被标成指令 */
export const KIND_UNKNOWN = 0;

/** 字节分类：指令的第一个字节 */
export const KIND_OPCODE = 1;

/** 字节分类：指令的操作数 */
export const KIND_OPERAND = 2;

/**
 * @typedef {object} Analysis
 * @property {Uint8Array} kinds 每个字节的分类
 * @property {number} instructions 识别出的指令条数
 * @property {string[]} warnings 追踪中放弃某一路的原因
 */

/**
 * 格式化地址，写进警告。
 * @param {number} address 地址
 * @returns {string} 如 $C000
 */
function hex(address) {
  return `$${address.toString(16).toUpperCase().padStart(4, '0')}`;
}

/**
 * 从入口出发追踪代码。
 * @param {{bytes: Buffer, base: number, entries: number[], cdl?: Uint8Array|null}} input 映像字节、第一个字节的地址、入口地址与可选的 CDL 标志
 * @returns {Analysis} 分析结果
 */
export function analyzeCode({ bytes, base, entries, cdl = null }) {
  const kinds = new Uint8Array(bytes.length);
  const warnings = [];
  const pending = [...entries];
  let instructions = 0;

  if (cdl) {
    // CDL 里每段连续的代码字节，起点一定是被执行过的指令的开头
    for (let offset = 0; offset < cdl.length; offset += 1) {
      if ((cdl[offset] & CDL_CODE) !== 0 && (offset === 0 || (cdl[offset - 1] & CDL_CODE) === 0)) pending.push(base + offset);
    }
  }

  while (pending.length > 0) {
    let address = pending.pop();
    // 沿着顺序执行一路走下去，遇到分支目标或调用目标就放进待办
    for (;;) {
      const offset = address - base;
      if (offset < 0 || offset >= bytes.length) break;
      if (kinds[offset] === KIND_OPCODE) break;
      const entry = OPCODES[bytes[offset]];
      if (entry === null) {
        warnings.push(`${hex(address)}：操作码 $${bytes[offset].toString(16).toUpperCase().padStart(2, '0')} 不是官方 6502 指令，放弃这一路`);
        break;
      }
      const [, size, flow] = entry;
      if (offset + size > bytes.length) {
        warnings.push(`${hex(address)}：指令超出映像末尾，放弃这一路`);
        break;
      }
      let overlaps = false;
      for (let index = 0; index < size; index += 1) {
        // 操作码位置必须是未知，操作数位置不能是别的指令的开头，也不能是别的指令的操作数
        if (kinds[offset + index] !== KIND_UNKNOWN) overlaps = true;
      }
      if (overlaps) {
        warnings.push(`${hex(address)}：与已识别的指令重叠，放弃这一路`);
        break;
      }
      kinds[offset] = KIND_OPCODE;
      for (let index = 1; index < size; index += 1) kinds[offset + index] = KIND_OPERAND;
      instructions += 1;

      if (flow === 'branch') {
        const displacement = bytes.readInt8(offset + 1);
        pending.push((address + 2 + displacement) & 0xffff);
      } else if (flow === 'jump' || flow === 'call') {
        pending.push(bytes.readUInt16LE(offset + 1));
      }
      if (flow === 'jump' || flow === 'jump-indirect' || flow === 'return' || flow === 'stop') break;
      address += size;
    }
  }
  return { kinds, instructions, warnings };
}
