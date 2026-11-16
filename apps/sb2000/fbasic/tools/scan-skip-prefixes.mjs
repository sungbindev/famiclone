// 找可能漏掉的跳过前缀：绝对寻址的 BIT/LDX/CPX 指令，操作数的低字节正好是一条立即数加载的操作码（A9、A2、A0）
import { OPCODES } from '../../../../packages/fcdev/src/disasm/opcode-table.mjs';
import { BASE, hex4, loadImageModel } from './lib/image-model.mjs';

const { bytes, items, map } = loadImageModel();
for (const item of items) {
  if (item.type !== 'code' || ![0x2c, 0xae, 0xec, 0xac, 0xcc].includes(item.opcode)) continue;
  const low = bytes[item.start - BASE + 1];
  if (![0xa9, 0xa2, 0xa0].includes(low)) continue;
  console.log(`${hex4(item.start)}  ${OPCODES[item.opcode][0]} $${hex4(bytes.readUInt16LE(item.start - BASE + 1))}`);
}
