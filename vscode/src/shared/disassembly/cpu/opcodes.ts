// ------------------------------------------------------------------------
// 名称：opcodes.ts
// 说明：6502 官方指令表：按助记符与寻址方式登记 151 条指令，生成 256 项的操作码表（非官方操作码为
//       null）。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-21
// 备注：流程类别：next 顺序执行；branch 条件分支；jump 绝对跳转；jump-indirect 间接跳转；call 子程
//       序调用；return 返回；stop 中断指令。表中的编码由测试用 ca65 逐条汇编核对（test/disasm-cpu.
//       test.mjs）。
// ------------------------------------------------------------------------

/** 寻址方式的名字 */
export type AddressingMode =
  | 'implied'
  | 'accumulator'
  | 'immediate'
  | 'zeroPage'
  | 'zeroPageX'
  | 'zeroPageY'
  | 'absolute'
  | 'absoluteX'
  | 'absoluteY'
  | 'indirect'
  | 'indexedIndirect'
  | 'indirectIndexed'
  | 'relative';

/** 流程类别 */
export type FlowKind = 'next' | 'branch' | 'jump' | 'jump-indirect' | 'call' | 'return' | 'stop';

/** 寻址方式及其指令长度（含操作码）。 */
export const ADDRESSING_MODES: Readonly<Record<AddressingMode, { size: number; description: string }>> = Object.freeze({
  implied: { size: 1, description: '隐含' },
  accumulator: { size: 1, description: '累加器' },
  immediate: { size: 2, description: '立即数' },
  zeroPage: { size: 2, description: '零页' },
  zeroPageX: { size: 2, description: '零页加 X' },
  zeroPageY: { size: 2, description: '零页加 Y' },
  absolute: { size: 3, description: '绝对' },
  absoluteX: { size: 3, description: '绝对加 X' },
  absoluteY: { size: 3, description: '绝对加 Y' },
  indirect: { size: 3, description: '间接（只有 JMP）' },
  indexedIndirect: { size: 2, description: '先变址后间接 (零页,X)' },
  indirectIndexed: { size: 2, description: '先间接后变址 (零页),Y' },
  relative: { size: 2, description: '相对（分支）' },
});

export interface Opcode {
  /** 操作码字节 */
  opcode: number;
  /** 助记符（小写） */
  mnemonic: string;
  /** 寻址方式，见 ADDRESSING_MODES */
  mode: AddressingMode;
  /** 指令长度（含操作码） */
  size: number;
  /** 流程类别 */
  flow: FlowKind;
}

/** 助记符、流程类别、各寻址方式的操作码 */
const DEFINITIONS: ReadonlyArray<readonly [string, FlowKind, Partial<Record<AddressingMode, number>>]> = [
  ['adc', 'next', { immediate: 0x69, zeroPage: 0x65, zeroPageX: 0x75, absolute: 0x6d, absoluteX: 0x7d, absoluteY: 0x79, indexedIndirect: 0x61, indirectIndexed: 0x71 }],
  ['and', 'next', { immediate: 0x29, zeroPage: 0x25, zeroPageX: 0x35, absolute: 0x2d, absoluteX: 0x3d, absoluteY: 0x39, indexedIndirect: 0x21, indirectIndexed: 0x31 }],
  ['asl', 'next', { accumulator: 0x0a, zeroPage: 0x06, zeroPageX: 0x16, absolute: 0x0e, absoluteX: 0x1e }],
  ['bcc', 'branch', { relative: 0x90 }],
  ['bcs', 'branch', { relative: 0xb0 }],
  ['beq', 'branch', { relative: 0xf0 }],
  ['bit', 'next', { zeroPage: 0x24, absolute: 0x2c }],
  ['bmi', 'branch', { relative: 0x30 }],
  ['bne', 'branch', { relative: 0xd0 }],
  ['bpl', 'branch', { relative: 0x10 }],
  ['brk', 'stop', { implied: 0x00 }],
  ['bvc', 'branch', { relative: 0x50 }],
  ['bvs', 'branch', { relative: 0x70 }],
  ['clc', 'next', { implied: 0x18 }],
  ['cld', 'next', { implied: 0xd8 }],
  ['cli', 'next', { implied: 0x58 }],
  ['clv', 'next', { implied: 0xb8 }],
  ['cmp', 'next', { immediate: 0xc9, zeroPage: 0xc5, zeroPageX: 0xd5, absolute: 0xcd, absoluteX: 0xdd, absoluteY: 0xd9, indexedIndirect: 0xc1, indirectIndexed: 0xd1 }],
  ['cpx', 'next', { immediate: 0xe0, zeroPage: 0xe4, absolute: 0xec }],
  ['cpy', 'next', { immediate: 0xc0, zeroPage: 0xc4, absolute: 0xcc }],
  ['dec', 'next', { zeroPage: 0xc6, zeroPageX: 0xd6, absolute: 0xce, absoluteX: 0xde }],
  ['dex', 'next', { implied: 0xca }],
  ['dey', 'next', { implied: 0x88 }],
  ['eor', 'next', { immediate: 0x49, zeroPage: 0x45, zeroPageX: 0x55, absolute: 0x4d, absoluteX: 0x5d, absoluteY: 0x59, indexedIndirect: 0x41, indirectIndexed: 0x51 }],
  ['inc', 'next', { zeroPage: 0xe6, zeroPageX: 0xf6, absolute: 0xee, absoluteX: 0xfe }],
  ['inx', 'next', { implied: 0xe8 }],
  ['iny', 'next', { implied: 0xc8 }],
  ['jmp', 'jump', { absolute: 0x4c }],
  ['jmp', 'jump-indirect', { indirect: 0x6c }],
  ['jsr', 'call', { absolute: 0x20 }],
  ['lda', 'next', { immediate: 0xa9, zeroPage: 0xa5, zeroPageX: 0xb5, absolute: 0xad, absoluteX: 0xbd, absoluteY: 0xb9, indexedIndirect: 0xa1, indirectIndexed: 0xb1 }],
  ['ldx', 'next', { immediate: 0xa2, zeroPage: 0xa6, zeroPageY: 0xb6, absolute: 0xae, absoluteY: 0xbe }],
  ['ldy', 'next', { immediate: 0xa0, zeroPage: 0xa4, zeroPageX: 0xb4, absolute: 0xac, absoluteX: 0xbc }],
  ['lsr', 'next', { accumulator: 0x4a, zeroPage: 0x46, zeroPageX: 0x56, absolute: 0x4e, absoluteX: 0x5e }],
  ['nop', 'next', { implied: 0xea }],
  ['ora', 'next', { immediate: 0x09, zeroPage: 0x05, zeroPageX: 0x15, absolute: 0x0d, absoluteX: 0x1d, absoluteY: 0x19, indexedIndirect: 0x01, indirectIndexed: 0x11 }],
  ['pha', 'next', { implied: 0x48 }],
  ['php', 'next', { implied: 0x08 }],
  ['pla', 'next', { implied: 0x68 }],
  ['plp', 'next', { implied: 0x28 }],
  ['rol', 'next', { accumulator: 0x2a, zeroPage: 0x26, zeroPageX: 0x36, absolute: 0x2e, absoluteX: 0x3e }],
  ['ror', 'next', { accumulator: 0x6a, zeroPage: 0x66, zeroPageX: 0x76, absolute: 0x6e, absoluteX: 0x7e }],
  ['rti', 'return', { implied: 0x40 }],
  ['rts', 'return', { implied: 0x60 }],
  ['sbc', 'next', { immediate: 0xe9, zeroPage: 0xe5, zeroPageX: 0xf5, absolute: 0xed, absoluteX: 0xfd, absoluteY: 0xf9, indexedIndirect: 0xe1, indirectIndexed: 0xf1 }],
  ['sec', 'next', { implied: 0x38 }],
  ['sed', 'next', { implied: 0xf8 }],
  ['sei', 'next', { implied: 0x78 }],
  ['sta', 'next', { zeroPage: 0x85, zeroPageX: 0x95, absolute: 0x8d, absoluteX: 0x9d, absoluteY: 0x99, indexedIndirect: 0x81, indirectIndexed: 0x91 }],
  ['stx', 'next', { zeroPage: 0x86, zeroPageY: 0x96, absolute: 0x8e }],
  ['sty', 'next', { zeroPage: 0x84, zeroPageX: 0x94, absolute: 0x8c }],
  ['tax', 'next', { implied: 0xaa }],
  ['tay', 'next', { implied: 0xa8 }],
  ['tsx', 'next', { implied: 0xba }],
  ['txa', 'next', { implied: 0x8a }],
  ['txs', 'next', { implied: 0x9a }],
  ['tya', 'next', { implied: 0x98 }],
];

/** 按操作码字节索引的指令表，非官方操作码为 null。 */
export const OPCODE_TABLE: ReadonlyArray<Readonly<Opcode> | null> = (() => {
  const table: Array<Readonly<Opcode> | null> = Array.from({ length: 256 }, () => null);
  for (const [mnemonic, flow, modes] of DEFINITIONS) {
    for (const [mode, opcode] of Object.entries(modes) as Array<[AddressingMode, number]>) {
      if (table[opcode] !== null) throw new Error(`操作码 $${opcode.toString(16)} 重复登记`);
      table[opcode] = Object.freeze({ opcode, mnemonic, mode, size: ADDRESSING_MODES[mode].size, flow });
    }
  }
  return Object.freeze(table);
})();

/**
 * 按助记符与寻址方式查操作码。
 * @param mnemonic 助记符，不分大小写
 * @param mode 寻址方式
 * @returns 指令；这个组合不存在时为 undefined
 */
export function findOpcode(mnemonic: string, mode: AddressingMode): Readonly<Opcode> | undefined {
  const name = mnemonic.toLowerCase();
  return OPCODE_TABLE.find((entry) => entry !== null && entry.mnemonic === name && entry.mode === mode) ?? undefined;
}
