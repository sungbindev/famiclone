// ------------------------------------------------------------------------
// 名称：show.mjs
// 说明：查看 PRG 里的一段内容：线性反汇编，或十六进制与 ASCII 转储。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-16
// 备注：用法 node tools/show.mjs asm <起始地址> [指令条数，默认 30]；node tools/show.mjs hex <起始地址> [字节数，默认 128]
// ------------------------------------------------------------------------
import { OPCODES } from '../../../../packages/fcdev/src/disasm/opcode-table.mjs';
import { BASE, hex4, readRom } from './lib/image-model.mjs';
import { addressingMode, formatInstruction } from './lib/opcode-modes.mjs';

const [mode, startText, countText] = process.argv.slice(2);
const start = Number.parseInt(startText, 16);
const { prg } = readRom();

if (mode === 'hex') {
  const count = Number(countText ?? 128);
  for (let address = start; address < start + count; address += 16) {
    const slice = prg.subarray(address - BASE, Math.min(prg.length, address - BASE + 16));
    const text = [...slice].map((value) => (value >= 0x20 && value < 0x7f ? String.fromCharCode(value) : '.')).join('');
    console.log(`${hex4(address)}  ${[...slice].map((value) => value.toString(16).toUpperCase().padStart(2, '0')).join(' ').padEnd(47)}  ${text}`);
  }
} else {
  let address = start;
  for (let index = 0; index < Number(countText ?? 30) && address < BASE + prg.length; index += 1) {
    const opcode = prg[address - BASE];
    const entry = OPCODES[opcode];
    if (entry === null) { console.log(`${hex4(address)}  ${opcode.toString(16).toUpperCase().padStart(2, '0')}        .byte`); address += 1; continue; }
    const size = entry[1];
    const raw = [...prg.subarray(address - BASE, address - BASE + size)];
    let operand = '';
    if (size === 2) operand = addressingMode(opcode) === 'rel' ? `$${hex4((address + 2 + (raw[1] << 24 >> 24)) & 0xffff)}` : `$${raw[1].toString(16).toUpperCase().padStart(2, '0')}`;
    if (size === 3) operand = `$${hex4(raw[1] | (raw[2] << 8))}`;
    console.log(`${hex4(address)}  ${raw.map((value) => value.toString(16).toUpperCase().padStart(2, '0')).join(' ').padEnd(8)}  ${formatInstruction(opcode, operand, false)}`);
    address += size;
  }
}
