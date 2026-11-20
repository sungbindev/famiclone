// ------------------------------------------------------------------------
// 名称：access.mjs
// 说明：判断一条指令对内存的访问方式：读、写、先读后写，或不访问内存。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-21
// 备注：JMP、JSR 与分支是控制转移，不算内存访问；累加器、立即数、隐含寻址不访问内存。
// ------------------------------------------------------------------------

/** 只读内存的助记符 */
const READS = new Set(['adc', 'and', 'bit', 'cmp', 'cpx', 'cpy', 'eor', 'lda', 'ldx', 'ldy', 'ora', 'sbc']);

/** 只写内存的助记符 */
const WRITES = new Set(['sta', 'stx', 'sty']);

/** 先读后写内存的助记符 */
const READ_WRITES = new Set(['asl', 'dec', 'inc', 'lsr', 'rol', 'ror']);

/** 不访问内存的寻址方式 */
const NO_MEMORY_MODES = new Set(['implied', 'accumulator', 'immediate', 'relative']);

/**
 * 指令对它的操作数地址的访问方式。
 * @param {string} mnemonic 助记符，小写
 * @param {string} mode 寻址方式
 * @returns {'read'|'write'|'readWrite'|null} 不访问内存时为 null
 */
export function memoryAccess(mnemonic, mode) {
  if (NO_MEMORY_MODES.has(mode)) return null;
  if (READS.has(mnemonic)) return 'read';
  if (WRITES.has(mnemonic)) return 'write';
  if (READ_WRITES.has(mnemonic)) return 'readWrite';
  return null;
}
