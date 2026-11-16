// ------------------------------------------------------------------------
// 名称：extract-seeds.mjs
// 说明：从步步高模拟器的运行轨迹里取出 GBASIC 执行过的指令地址，写成 analysis/trace-entries.txt，作为代码发现的种子。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-16
// 备注：用法 node tools/extract-seeds.mjs；轨迹文件来自 apps/bbk/gbasic/tools/trace-gbasic.mjs（build/ 不进 Git，所以把种子存进本项目）。
//       轨迹按 CPU 地址记录，分页内存里别的页（BIOS、BBGDOS 的页）执行的指令也会落在 $8000-$BFFF，所以每一行都要核对：操作码必须与映像一致，行里非零的操作数也必须一致才采用。
// ------------------------------------------------------------------------
import { readFileSync, writeFileSync } from 'node:fs';
import { dirname, join, resolve } from 'node:path';
import { fileURLToPath } from 'node:url';
import { OPCODES } from '../../../../packages/fcdev/src/disasm/opcode-table.mjs';

const ROOT = resolve(dirname(fileURLToPath(import.meta.url)), '..');
const TRACE = join(ROOT, '..', '..', 'bbk', 'gbasic', 'build', 'gbasic-trace.txt');
const IMAGE = join(ROOT, '..', '..', 'bbk', 'gbasic', 'disasm', 'gbasic.bin');
const BASE = 0x7b00;

const image = readFileSync(IMAGE);
const addresses = new Set();
let rejected = 0;
for (const line of readFileSync(TRACE, 'utf8').split('\n')) {
  const match = /^T ([0-9A-F]{4}) \S+ ([0-9A-F]{2}) ([0-9A-F]{2}) ([0-9A-F]{2}) /.exec(line);
  if (!match) continue;
  const address = Number.parseInt(match[1], 16);
  if (address < BASE || address >= BASE + image.length) continue;
  const offset = address - BASE;
  const entry = OPCODES[image[offset]];
  const traced = [match[2], match[3], match[4]].map((text) => Number.parseInt(text, 16));
  const size = entry === null ? 1 : entry[1];
  let same = traced[0] === image[offset];
  // 轨迹行里 GBASIC 自己的指令操作数常记成 00，所以只有非零且与映像不同的操作数才算不符
  for (let index = 1; index < size; index += 1) if (traced[index] !== 0 && traced[index] !== image[offset + index]) same = false;
  if (same) addresses.add(address);
  else rejected += 1;
}
const sorted = [...addresses].sort((left, right) => left - right);
const lines = ['# 步步高运行轨迹里 GBASIC 执行过的指令地址（每行一个，十六进制；已剔除指令字节与映像不符的行）', ...sorted.map((address) => address.toString(16).toUpperCase().padStart(4, '0'))];
writeFileSync(join(ROOT, 'analysis', 'trace-entries.txt'), `${lines.join('\n')}\n`);
console.log(`写入 ${sorted.length} 个地址，剔除 ${rejected} 行（字节与映像不符，来自分页里别的页）`);
