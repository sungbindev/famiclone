// ------------------------------------------------------------------------
// 名称：address-tables.mjs
// 说明：在已追踪的代码里找经地址表分发的跳转：从表里读出目标地址，供静态追踪继续追下去。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-13
// 备注：识别的写法（窗口为紧随加载指令之后的 10 条顺序指令）：
//       1. LDA 表,X/Y 与 LDA 表+1,X/Y 成对，窗口内有间接 JMP：字表，目标就是表里的值；
//       2. 一对加载指令，窗口内有两条 PHA 与一条 RTS：目标是表里的值加 1；表是字表（两个加载地址相差 1）或低、高字节分开的两张表；
//       3. 间接 JMP 的地址指向映像内未识别的字节：表项是目标。
//       表项必须落在映像里、是官方操作码、不在已识别指令的中间，否则表在这一项结束；单张表最多 128 项。
// ------------------------------------------------------------------------
import { KIND_ADDRESS, KIND_OPCODE, KIND_UNKNOWN } from './byte-kinds.mjs';
import { OPCODES } from './opcode-table.mjs';

const LDA_ABS_Y = 0xb9;
const LDA_ABS_X = 0xbd;
const PHA = 0x48;
const RTS = 0x60;
const JMP_INDIRECT = 0x6c;

/** 加载指令之后检查的顺序指令条数 */
const WINDOW = 10;

/** 单张表的最大项数 */
const MAX_ENTRIES = 128;

/**
 * @typedef {object} AddressTable
 * @property {string} key 表的标识，同一张表只返回一次
 * @property {number|null} wordOffset 字表在映像里的偏移；分开的两张表为 null
 * @property {number[]} targets 表里读出的目标地址
 */

/**
 * 判断一个目标地址能不能作为代码入口。
 * @param {{bytes: Buffer, base: number, kinds: Uint8Array}} image 映像与分类
 * @param {number} target 目标地址
 * @returns {boolean} 在映像内、是官方操作码、不在已识别指令中间
 */
function isCodeTarget({ bytes, base, kinds }, target) {
  const offset = target - base;
  if (offset < 0 || offset >= bytes.length) return false;
  if (OPCODES[bytes[offset]] === null) return false;
  return kinds[offset] === KIND_UNKNOWN || kinds[offset] === KIND_OPCODE;
}

/**
 * 从字表的偏移处读出连续的目标，遇到不能当入口的值或已识别的字节就停止。
 * @param {{bytes: Buffer, base: number, kinds: Uint8Array}} image 映像与分类
 * @param {number} start 表在映像里的偏移
 * @param {number} adjust 加到每个值上的偏移，RTS 表为 1
 * @returns {number[]} 目标地址
 */
function readWordTable(image, start, adjust) {
  const { bytes, kinds } = image;
  const targets = [];
  for (let offset = start; offset + 1 < bytes.length && targets.length < MAX_ENTRIES; offset += 2) {
    if (kinds[offset] !== KIND_UNKNOWN || kinds[offset + 1] !== KIND_UNKNOWN) break;
    const target = (bytes.readUInt16LE(offset) + adjust) & 0xffff;
    if (!isCodeTarget(image, target)) break;
    targets.push(target);
  }
  return targets;
}

/**
 * 读出低、高字节分开的两张表。
 * @param {{bytes: Buffer, base: number, kinds: Uint8Array}} image 映像与分类
 * @param {number} lowOffset 低字节表的偏移
 * @param {number} highOffset 高字节表的偏移
 * @returns {number[]} 目标地址（已加 1）
 */
function readSplitTable(image, lowOffset, highOffset) {
  const { bytes, kinds } = image;
  const targets = [];
  while (targets.length < MAX_ENTRIES) {
    const low = lowOffset + targets.length;
    const high = highOffset + targets.length;
    if (low >= bytes.length || high >= bytes.length || kinds[low] !== KIND_UNKNOWN || kinds[high] !== KIND_UNKNOWN) break;
    const target = ((bytes[high] << 8) | bytes[low]) + 1;
    if (!isCodeTarget(image, target & 0xffff)) break;
    targets.push(target & 0xffff);
  }
  return targets;
}

/**
 * 取出某条指令之后的顺序指令的偏移，遇到未识别的字节就停止。
 * @param {{bytes: Buffer, kinds: Uint8Array}} image 映像与分类
 * @param {number} offset 起点指令的偏移
 * @returns {number[]} 起点之后最多 WINDOW 条指令的偏移
 */
function followingInstructions({ bytes, kinds }, offset) {
  const result = [];
  let current = offset + OPCODES[bytes[offset]][1];
  while (result.length < WINDOW && current < bytes.length && kinds[current] === KIND_OPCODE) {
    result.push(current);
    current += OPCODES[bytes[current]][1];
  }
  return result;
}

/**
 * 查找经地址表分发的跳转，只返回还没有返回过的表。
 * @param {{bytes: Buffer, base: number, kinds: Uint8Array, seen: Set<string>}} input 映像、分类与已返回过的表的标识
 * @returns {AddressTable[]} 新找到的表
 */
export function findAddressTables({ bytes, base, kinds, seen }) {
  const image = { bytes, base, kinds };
  const found = [];
  const add = (key, wordOffset, targets) => {
    if (seen.has(key) || targets.length === 0) return;
    seen.add(key);
    found.push({ key, wordOffset, targets });
  };

  for (let offset = 0; offset < bytes.length; offset += 1) {
    if (kinds[offset] !== KIND_OPCODE) continue;
    const opcode = bytes[offset];

    if (opcode === JMP_INDIRECT) {
      const tableOffset = bytes.readUInt16LE(offset + 1) - base;
      if (tableOffset >= 0 && tableOffset < bytes.length) add(`w:${tableOffset}:0`, tableOffset, readWordTable(image, tableOffset, 0));
      continue;
    }
    if (opcode !== LDA_ABS_X && opcode !== LDA_ABS_Y) continue;

    const window = followingInstructions(image, offset);
    const loads = [offset, ...window.filter((index) => bytes[index] === LDA_ABS_X || bytes[index] === LDA_ABS_Y)];
    if (loads.length < 2) continue;
    const addresses = loads.map((index) => bytes.readUInt16LE(index + 1) - base);
    const pushes = window.filter((index) => bytes[index] === PHA).length;
    const returns = window.some((index) => bytes[index] === RTS);
    const indirect = window.some((index) => bytes[index] === JMP_INDIRECT);

    if (indirect) {
      // 两条加载的地址相差 1：字表在较小的地址上
      const low = Math.min(addresses[0], addresses[1]);
      if (Math.abs(addresses[0] - addresses[1]) === 1 && low >= 0 && low < bytes.length) add(`w:${low}:0`, low, readWordTable(image, low, 0));
    } else if (pushes >= 2 && returns) {
      // 先压入高字节再压入低字节，RTS 返回到“表里的值加 1”
      const high = addresses[0];
      const low = addresses[1];
      if (high < 0 || low < 0 || high >= bytes.length || low >= bytes.length) continue;
      if (Math.abs(high - low) === 1 && low < high) add(`w:${low}:1`, low, readWordTable(image, low, 1));
      else if (Math.abs(high - low) > 1) add(`s:${low}:${high}`, null, readSplitTable(image, low, high));
    }
  }
  return found;
}

/**
 * 把字表占的字节标成地址。
 * @param {Uint8Array} kinds 每个字节的分类
 * @param {AddressTable} table 字表
 */
export function markWordTable(kinds, table) {
  if (table.wordOffset === null) return;
  for (let index = 0; index < table.targets.length * 2; index += 1) kinds[table.wordOffset + index] = KIND_ADDRESS;
}
