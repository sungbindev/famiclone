// ------------------------------------------------------------------------
// 名称：discover.mjs
// 说明：对 Family BASIC 的 PRG 做代码发现并打印报告：CDL 种子与人工确认的入口先追踪，再在剩下的未知区段里试探，列出试探通过的候选供人工复核。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-16
// 备注：用法 node tools/discover.mjs [--min <指令条数>] [--list]；--list 额外列出剩余的未知区段。
//       输入：analysis/cdl-entries.txt（CDL 种子）、analysis/code-entries.txt（人工确认的入口，可含 # 注释）、analysis/data-ranges.txt（人工确认的数据区段，起止 地址），
//       试探不会跨进人工确认的数据区段。
// ------------------------------------------------------------------------
import { existsSync, readFileSync, writeFileSync } from 'node:fs';
import { dirname, join, resolve } from 'node:path';
import { fileURLToPath } from 'node:url';
import { OPCODES } from '../../../../packages/fcdev/src/disasm/opcode-table.mjs';
import { CodeMap, hex } from './lib/code-discovery.mjs';
import { readAddressList } from './lib/analysis-files.mjs';
import { BASE, ROOT } from './lib/image-model.mjs';
import { buildSeedMap } from './lib/seed-map.mjs';

const args = process.argv.slice(2);
const minIndex = args.indexOf('--min');
const MIN_INSTRUCTIONS = minIndex >= 0 ? Number(args[minIndex + 1]) : 6;

const { bytes, map, inLayout: inData, tableTargets } = buildSeedMap();
const skipFile = join(ROOT, 'analysis', 'skip-prefixes.txt');
const skipPrefixes = new Set(existsSync(skipFile) ? readAddressList(skipFile) : []);
const manualEntries = existsSync(join(ROOT, 'analysis', 'code-entries.txt')) ? readAddressList(join(ROOT, 'analysis', 'code-entries.txt')) : [];
console.log(`映像 ${bytes.length} 字节；人工入口 ${manualEntries.length} 个，地址表追到 ${tableTargets.length} 个入口，代码 ${map.codeBytes()} 字节`);

/**
 * 找出指向某个地址的证据：代码里的 JSR/JMP/分支、数据里的字（目标或目标减 1），以及它紧跟在 RTS、JMP 之后。
 * @param {number} address 候选起点
 * @returns {string} 依据，空串表示没有
 */
function evidenceFor(address) {
  const found = [];
  const low = address & 0xff;
  const high = address >> 8;
  const lowMinus = (address - 1) & 0xff;
  const highMinus = (address - 1) >> 8;
  for (let offset = 0; offset + 1 < bytes.length; offset += 1) {
    const isOperand = map.kinds[offset] === 2;
    if (bytes[offset] === low && bytes[offset + 1] === high && offset > 0 && (map.kinds[offset - 1] === 1) && [0x20, 0x4c].includes(bytes[offset - 1])) found.push(`${hex(BASE + offset - 1)} ${bytes[offset - 1] === 0x20 ? 'JSR' : 'JMP'}`);
    else if (!isOperand && map.kinds[offset] === 0 && bytes[offset] === low && bytes[offset + 1] === high) found.push(`${hex(BASE + offset)} 数据字`);
    else if (!isOperand && map.kinds[offset] === 0 && bytes[offset] === lowMinus && bytes[offset + 1] === highMinus) found.push(`${hex(BASE + offset)} 数据字-1`);
  }
  let previous = address - 1 - BASE;
  while (previous > 0 && bytes[previous] === 0xea && map.kinds[previous] !== 2) previous -= 1;
  if (previous >= 0 && [0x60, 0x40].includes(bytes[previous]) && map.kinds[previous] === 1) found.push('紧跟 RTS（可隔 NOP 填充）');
  for (let offset = 1; offset + 1 < bytes.length; offset += 1) {
    if (map.kinds[offset - 1] === 1 && map.kinds[offset] === 2 && OPCODES[bytes[offset - 1]]?.[2] === 'branch') {
      const target = (BASE + offset - 1 + 2 + bytes.readInt8(offset)) & 0xffff;
      if (target === address) found.push(`${hex(BASE + offset - 1)} 分支`);
    }
  }
  return found.slice(0, 4).join('；');
}

const candidates = [];
for (let round = 0; round < 50; round += 1) {
  let added = 0;
  for (const range of map.unknownRanges()) {
    // 试探区段的每个可能起点，只取通过严格检查且指令数最多的那个起点所在的完整追踪
    for (let address = range.start; address <= range.end; address += 1) {
      if (inData(address)) continue;
      const trial = map.trace([address], { strict: true, commit: false });
      if (!trial.ok || trial.count < MIN_INSTRUCTIONS) continue;
      map.trace([address], { strict: true, commit: true });
      candidates.push({ address, count: trial.count });
      added += 1;
      break;
    }
  }
  if (added === 0) break;
}
console.log(`试探通过 ${candidates.length} 个候选，合计代码 ${map.codeBytes()} 字节`);
for (const candidate of candidates) console.log(`  ${hex(candidate.address)}  ${candidate.count} 条指令  依据：${evidenceFor(candidate.address) || '无'}`);

/**
 * 代码里被当作数据读写的映像内地址（LDA、STA 等绝对寻址的操作数，不含 JSR、JMP）。
 * @returns {Set<number>} 地址集合
 */
function dataReferences() {
  const found = new Set();
  for (let offset = 0; offset + 2 < bytes.length; offset += 1) {
    if (map.kinds[offset] !== 1) continue;
    const entry = OPCODES[bytes[offset]];
    if (entry[1] !== 3 || entry[2] !== 'next') continue;
    const target = bytes.readUInt16LE(offset + 1);
    if (map.contains(target)) found.add(target);
  }
  return found;
}

// 小片段：未知区段从头严格追踪，恰好把整段读成指令，并且没有被代码当作数据读写，就当作代码
const fragments = [];
for (let round = 0; round < 50; round += 1) {
  let added = 0;
  const referenced = dataReferences();
  for (const range of map.unknownRanges()) {
    if (inData(range.start)) continue;
    let touched = false;
    for (let address = range.start; address <= range.end; address += 1) if (referenced.has(address)) touched = true;
    if (touched) continue;
    const trial = map.trace([range.start], { strict: true, commit: false });
    if (!trial.ok || trial.count < 1) continue;
    const probe = new CodeMap(bytes, BASE, skipPrefixes);
    probe.kinds.set(map.kinds);
    probe.trace([range.start], { strict: true, commit: true });
    let covers = true;
    let firstGap = -1;
    for (let address = range.start; address <= range.end; address += 1) {
      if (probe.kinds[address - BASE] !== 0) continue;
      if (firstGap < 0) firstGap = address;
    }
    // 整段都是代码，或者只在末尾剩下一串相同的填充字节
    if (firstGap >= 0) {
      for (let address = firstGap; address <= range.end; address += 1) {
        if (probe.kinds[address - BASE] !== 0 || bytes[address - BASE] !== bytes[firstGap - BASE]) covers = false;
      }
      if (range.end - firstGap + 1 < 2) covers = false;
    }
    if (!covers) continue;
    map.trace([range.start], { strict: true, commit: true });
    fragments.push({ address: range.start, count: trial.count, size: range.end - range.start + 1 });
    added += 1;
  }
  if (added === 0) break;
}
console.log(`小片段整段读成代码 ${fragments.length} 个：${fragments.map((fragment) => `${hex(fragment.address)}(${fragment.size})`).join(' ')}`);
if (args.includes('--list')) {
  const rest = map.unknownRanges();
  console.log(`剩余未知区段 ${rest.length} 个，共 ${rest.reduce((sum, range) => sum + range.end - range.start + 1, 0)} 字节`);
  for (const range of rest) console.log(`  ${hex(range.start)}-${hex(range.end)}  ${range.end - range.start + 1}`);
}

if (args.includes('--write-entries')) {
  const lines = ['# 人工复核后确认的代码入口：来自 discover.mjs 的试探结果（每行：地址  指令数  依据），复核通过后由 --write-entries 写入', ...manualEntries.map((address) => `${hex(address).slice(1)}  已有`)];
  for (const candidate of candidates) lines.push(`${hex(candidate.address).slice(1)}  ${candidate.count} 条指令  ${evidenceFor(candidate.address) || '无直接依据，前后字节已人工查看'}`);
  for (const fragment of fragments) lines.push(`${hex(fragment.address).slice(1)}  ${fragment.size} 字节小片段  整段读成指令`);
  writeFileSync(join(ROOT, 'analysis', 'code-entries.txt'), `${lines.join('\n')}\n`);
  console.log(`已写入 ${candidates.length + fragments.length} 个入口到 analysis/code-entries.txt`);
}
