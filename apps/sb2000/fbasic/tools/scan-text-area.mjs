// ------------------------------------------------------------------------
// 名称：scan-text-area.mjs
// 说明：列出原版里可能与程序文本区（$6000-$7FFF）有关的指令：绝对地址落在 $6000-$7FFF 的访问，以及立即数在 $5F-$71 之间的指令（可能是文本区的高字节），带上前后各两条指令，供逐处判断要不要加偏移。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-16
// 备注：用法 node tools/scan-text-area.mjs [--abs] [--imm]；结果人工判断后写进 analysis/text-area.txt。
// ------------------------------------------------------------------------
import { OPCODES } from './lib/opcodes.mjs';
import { BASE, loadImageModel } from './lib/image-model.mjs';
import { addressingMode, formatInstruction } from './lib/opcode-modes.mjs';

const { items, bytes } = loadImageModel();
const args = process.argv.slice(2);
const wantAbs = args.includes('--abs') || !args.includes('--imm');
const wantImm = args.includes('--imm') || !args.includes('--abs');

const text = (item) => {
  if (item.type !== 'code') return `(${item.type} @${item.start.toString(16)})`;
  const mode = addressingMode(item.opcode);
  const offset = item.start - BASE;
  let operand = '';
  if (mode === 'imm') operand = `#$${bytes[offset + 1].toString(16).toUpperCase().padStart(2, '0')}`;
  else if (['zp', 'zpx', 'zpy', 'izx', 'izy'].includes(mode)) operand = `$${bytes[offset + 1].toString(16).toUpperCase().padStart(2, '0')}`;
  else if (['abs', 'abx', 'aby', 'ind'].includes(mode)) operand = `$${bytes.readUInt16LE(offset + 1).toString(16).toUpperCase().padStart(4, '0')}`;
  else if (mode === 'rel') operand = `$${((item.start + 2 + bytes.readInt8(offset + 1)) & 0xffff).toString(16).toUpperCase()}`;
  return `${item.start.toString(16).toUpperCase()} ${OPCODES[item.opcode][0]} ${operand}${mode === 'abx' || mode === 'zpx' ? ',x' : ''}${mode === 'aby' || mode === 'zpy' ? ',y' : ''}`.trim();
};

items.forEach((item, index) => {
  if (item.type !== 'code') return;
  const mode = addressingMode(item.opcode);
  const offset = item.start - BASE;
  let hit = false;
  if (wantAbs && ['abs', 'abx', 'aby', 'ind'].includes(mode)) {
    const value = bytes.readUInt16LE(offset + 1);
    hit = value >= 0x6000 && value <= 0x7fff;
  }
  if (wantImm && mode === 'imm') {
    const value = bytes[offset + 1];
    hit = value >= 0x5f && value <= 0x71;
  }
  if (!hit) return;
  const context = [-2, -1, 0, 1, 2].map((delta) => text(items[index + delta] ?? { type: 'none', start: 0 }));
  console.log(context.join('  |  '));
});
