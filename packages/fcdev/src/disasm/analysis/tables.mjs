// ------------------------------------------------------------------------
// 名称：tables.mjs
// 说明：在已追踪的代码里找经地址表分发的跳转：从表里读出目标地址，供静态追踪继续追下去。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-21
// 备注：识别的写法（窗口为紧随加载指令之后的 10 条顺序指令）：
//       1. LDA 表,X/Y 与 LDA 表+1,X/Y 成对，窗口内有间接 JMP：字表，目标就是表里的值；
//       2. 一对加载指令，窗口内有两条 PHA 与一条 RTS：目标是表里的值加 1；表是字表（两个加载地址相差 1）或低、高字节分开的两张表；
//       3. 间接 JMP 的地址指向段内未识别的字节：表项是目标。
//       表项的目标必须落在某个段里（换页窗口里只认同一个 bank）、是官方操作码、不在已识别指令的中间，否则表在这一项结束；单张表最多 128 项。
// ------------------------------------------------------------------------
import { OPCODE_TABLE, decode } from '../cpu/index.mjs';
import { addressToOffset } from '../model/index.mjs';
import { BYTE_KIND } from './kinds.mjs';

/** 加载指令之后检查的顺序指令条数 */
const WINDOW = 10;

/** 单张表的最大项数 */
const MAX_ENTRIES = 128;

/**
 * @typedef {object} AddressTable
 * @property {string} key 表的标识，同一张表只返回一次
 * @property {'words'|'split'} kind words 是字表；split 是低、高字节分开的两张表
 * @property {number} segmentId 表所在的段
 * @property {number} offset 字表的偏移；split 时是低字节表的偏移
 * @property {number} address 字表的 CPU 地址；split 时是低字节表的 CPU 地址
 * @property {number|null} highOffset split 时高字节表的偏移，否则为 null
 * @property {number} adjust 加到表里每个值上的数，RTS 表为 1
 * @property {number} count 表项数
 * @property {number[]} targets 表里读出的目标地址
 * @property {number[]} jumpOffsets 用这张表分发的间接 JMP 的偏移，用来把它们从“未解析的间接跳转”里去掉
 */

/**
 * 查找经地址表分发的跳转，只返回还没有返回过的表。
 * @param {import('./tracer.mjs').Analysis} analysis 当前的分析结果，会在 seenTables 里记下返回过的表
 * @param {{bytes: Uint8Array, space: import('./address-space.mjs').AddressSpace}} context 映像字节与地址空间
 * @returns {AddressTable[]} 新找到的表
 */
export function findAddressTables(analysis, { bytes, space }) {
  const { kinds } = analysis;
  const found = [];

  const locate = (address, fromSegmentId) => {
    const resolved = space.resolve(address, fromSegmentId);
    return resolved.status === 'ok' ? { segment: resolved.segment, offset: addressToOffset(resolved.segment, address) } : null;
  };

  const isCodeTarget = (address, fromSegmentId) => {
    const spot = locate(address, fromSegmentId);
    if (spot === null || OPCODE_TABLE[bytes[spot.offset]] === null) return false;
    return kinds[spot.offset] === BYTE_KIND.UNKNOWN || kinds[spot.offset] === BYTE_KIND.OPCODE;
  };

  const readWordTable = (spot, adjust) => {
    const end = spot.segment.fileOffset + spot.segment.length;
    const targets = [];
    for (let offset = spot.offset; offset + 1 < end && targets.length < MAX_ENTRIES; offset += 2) {
      if (kinds[offset] !== BYTE_KIND.UNKNOWN || kinds[offset + 1] !== BYTE_KIND.UNKNOWN) break;
      const target = ((bytes[offset] | (bytes[offset + 1] << 8)) + adjust) & 0xffff;
      if (!isCodeTarget(target, spot.segment.id)) break;
      targets.push(target);
    }
    return targets;
  };

  const readSplitTable = (lowSpot, highSpot) => {
    const lowEnd = lowSpot.segment.fileOffset + lowSpot.segment.length;
    const highEnd = highSpot.segment.fileOffset + highSpot.segment.length;
    const targets = [];
    while (targets.length < MAX_ENTRIES) {
      const low = lowSpot.offset + targets.length;
      const high = highSpot.offset + targets.length;
      if (low >= lowEnd || high >= highEnd || kinds[low] !== BYTE_KIND.UNKNOWN || kinds[high] !== BYTE_KIND.UNKNOWN) break;
      const target = (((bytes[high] << 8) | bytes[low]) + 1) & 0xffff;
      if (!isCodeTarget(target, lowSpot.segment.id)) break;
      targets.push(target);
    }
    return targets;
  };

  const addWords = (spot, adjust, jumpOffsets) => {
    const key = `w:${spot.offset}:${adjust}`;
    if (analysis.seenTables.has(key)) return;
    const targets = readWordTable(spot, adjust);
    if (targets.length === 0) return;
    analysis.seenTables.add(key);
    found.push({
      key, kind: 'words', segmentId: spot.segment.id, offset: spot.offset, address: spot.segment.address + (spot.offset - spot.segment.fileOffset),
      highOffset: null, adjust, count: targets.length, targets, jumpOffsets,
    });
  };

  const addSplit = (lowSpot, highSpot) => {
    const key = `s:${lowSpot.offset}:${highSpot.offset}`;
    if (analysis.seenTables.has(key)) return;
    const targets = readSplitTable(lowSpot, highSpot);
    if (targets.length === 0) return;
    analysis.seenTables.add(key);
    found.push({
      key, kind: 'split', segmentId: lowSpot.segment.id, offset: lowSpot.offset, address: lowSpot.segment.address + (lowSpot.offset - lowSpot.segment.fileOffset),
      highOffset: highSpot.offset, adjust: 1, count: targets.length, targets, jumpOffsets: [],
    });
  };

  /** 紧随 offset 处指令之后的顺序指令，遇到没追到的字节就停止 */
  const following = (offset) => {
    const result = [];
    let next = offset + analysis.instructions.get(offset).size;
    while (result.length < WINDOW && analysis.instructions.has(next)) {
      result.push(next);
      next += analysis.instructions.get(next).size;
    }
    return result;
  };

  const isIndexedLoad = (instruction) => instruction.mnemonic === 'lda' && (instruction.mode === 'absoluteX' || instruction.mode === 'absoluteY');

  for (const offset of [...analysis.instructions.keys()].sort((a, b) => a - b)) {
    const info = analysis.instructions.get(offset);
    const instruction = decode(bytes, offset, info.address);
    if (!instruction.ok) continue;

    if (instruction.mnemonic === 'jmp' && instruction.mode === 'indirect') {
      const spot = locate(instruction.operand, info.segmentId);
      if (spot !== null) addWords(spot, 0, [offset]);
      continue;
    }
    if (!isIndexedLoad(instruction)) continue;

    const window = following(offset).map((index) => ({ offset: index, instruction: decode(bytes, index, analysis.instructions.get(index).address) }));
    const loads = [instruction, ...window.filter((item) => item.instruction.ok && isIndexedLoad(item.instruction)).map((item) => item.instruction)];
    if (loads.length < 2) continue;
    const addresses = loads.map((item) => item.operand);
    const pushes = window.filter((item) => item.instruction.ok && item.instruction.mnemonic === 'pha').length;
    const returns = window.some((item) => item.instruction.ok && item.instruction.mnemonic === 'rts');
    const indirect = window.filter((item) => item.instruction.ok && item.instruction.mnemonic === 'jmp' && item.instruction.mode === 'indirect');

    if (indirect.length > 0) {
      // 两条加载的地址相差 1：字表在较小的地址上
      if (Math.abs(addresses[0] - addresses[1]) === 1) {
        const spot = locate(Math.min(addresses[0], addresses[1]), info.segmentId);
        if (spot !== null) addWords(spot, 0, indirect.map((item) => item.offset));
      }
    } else if (pushes >= 2 && returns) {
      // 先压入高字节再压入低字节，RTS 返回到“表里的值加 1”
      const high = addresses[0];
      const low = addresses[1];
      const lowSpot = locate(low, info.segmentId);
      const highSpot = locate(high, info.segmentId);
      if (lowSpot === null || highSpot === null) continue;
      if (Math.abs(high - low) === 1 && low < high) addWords(lowSpot, 1, []);
      else if (Math.abs(high - low) > 1) addSplit(lowSpot, highSpot);
    }
  }
  return found;
}

/**
 * 把字表占的字节标成地址；低、高字节分开的表保持为数据。
 * @param {Uint8Array} kinds 每个字节的分类
 * @param {AddressTable} table 表
 */
export function markWordTable(kinds, table) {
  if (table.kind !== 'words') return;
  for (let index = 0; index < table.count * 2; index += 1) kinds[table.offset + index] = BYTE_KIND.ADDRESS;
}
