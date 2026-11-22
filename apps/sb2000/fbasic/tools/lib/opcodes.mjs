// ------------------------------------------------------------------------
// 名称：opcodes.mjs
// 说明：fbasic 工具共用的 6502 指令表：按操作码取 [助记符, 长度, 流程类别]，非官方操作码为 null。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-21
// 备注：数据取自 fcdev 反汇编核心的指令表（OPCODE_TABLE），这里只换成各工具习惯的数组形式。
// ------------------------------------------------------------------------
import { OPCODE_TABLE } from '../../../../../tools/extension.mjs';

/** 256 项：官方操作码为 [助记符, 长度, 流程类别]，其余为 null */
export const OPCODES = OPCODE_TABLE.map((entry) => (entry === null ? null : [entry.mnemonic, entry.size, entry.flow]));
