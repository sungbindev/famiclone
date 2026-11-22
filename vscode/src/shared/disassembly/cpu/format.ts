// ------------------------------------------------------------------------
// 名称：format.ts
// 说明：把解码出的指令写成 ca65 的汇编文字。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-21
// 备注：操作数是绝对寻址而地址小于 $100 时加 a: 前缀，否则 ca65 会缩成零页寻址，生成的字节就不一致
//       了；调用者可以用 expression 把数值换成符号名，前缀规则不变。
// ------------------------------------------------------------------------
import type { DecodedInstruction } from './decode';

export interface FormatOptions {
  /** 代替操作数数值的表达式（符号名、符号加偏移等），不含寻址方式的括号、# 与变址 */
  expression?: string;
}

/**
 * 判断这条指令能否用 ca65 写成文字后汇编回同样的字节。
 * @param instruction 解码结果
 * @returns 间接 JMP 的指针低字节是 $FF 时为 false：ca65 对 6502 的页边界错误直接拒绝汇编，生成时要
 *   改写成 .byte
 */
export function isAssemblable(instruction: DecodedInstruction): boolean {
  return !(instruction.mode === 'indirect' && ((instruction.operand ?? 0) & 0xff) === 0xff);
}

/**
 * 把数值写成 $ 开头的十六进制。
 * @param value 数值
 * @param digits 位数
 * @returns 如 $C000
 */
export function formatHex(value: number, digits: number): string {
  return `$${value.toString(16).toUpperCase().padStart(digits, '0')}`;
}

/**
 * 写出指令的操作数部分。
 * @param instruction 解码结果
 * @param [options] 选项
 * @returns 操作数文字；没有操作数时为空字符串
 */
export function formatOperand(instruction: DecodedInstruction, options: FormatOptions = {}): string {
  const { mode, operand, target } = instruction;
  const byteText = options.expression ?? formatHex(operand ?? 0, 2);
  const wordText = options.expression ?? formatHex(operand ?? 0, 4);
  const forcedWord = operand !== null && operand < 0x100 ? `a:${wordText}` : wordText;
  switch (mode) {
    case 'implied': return '';
    case 'accumulator': return 'a';
    case 'immediate': return `#${byteText}`;
    case 'zeroPage': return byteText;
    case 'zeroPageX': return `${byteText},x`;
    case 'zeroPageY': return `${byteText},y`;
    case 'absolute': return forcedWord;
    case 'absoluteX': return `${forcedWord},x`;
    case 'absoluteY': return `${forcedWord},y`;
    case 'indirect': return `(${wordText})`;
    case 'indexedIndirect': return `(${byteText},x)`;
    case 'indirectIndexed': return `(${byteText}),y`;
    case 'relative': return options.expression ?? formatHex(target ?? 0, 4);
    default: throw new RangeError(`未知的寻址方式：${mode}`);
  }
}

/**
 * 写出一条完整的指令文字。
 * @param instruction 解码结果
 * @param [options] 选项
 * @returns 如 `lda #$01`、`jmp $C000`、`rts`
 */
export function formatInstruction(instruction: DecodedInstruction, options: FormatOptions = {}): string {
  const operandText = formatOperand(instruction, options);
  return operandText === '' ? instruction.mnemonic : `${instruction.mnemonic} ${operandText}`;
}
