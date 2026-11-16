// ------------------------------------------------------------------------
// 名称：plan-banks.mjs
// 说明：把代码块分配到常驻页与各库页：先按被引用的次数挑出常驻的块，其余按块间引用的强弱合并成不超过一页的簇，再装箱。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-16
// 备注：用法 node tools/plan-banks.mjs [--resident <字节>] [--bank <字节>] [--write]；--write 把分配写进 analysis/banks.txt（每行：块起点 位置，位置是 R 或库页号）。
//       跨库页的引用要经跳板，所以输出里列出跳板数（被跨页引用的不同目标）与跨页引用总数。数据块不在这里分配（见 data-layout.txt 与阶段 4 的说明）。
// ------------------------------------------------------------------------
import { writeFileSync } from 'node:fs';
import { join } from 'node:path';
import { ROOT, hex4 } from './lib/image-model.mjs';
import { buildChunks } from './lib/chunks.mjs';

const args = process.argv.slice(2);
const option = (name, fallback) => { const index = args.indexOf(name); return index >= 0 ? Number(args[index + 1]) : fallback; };
const RESIDENT_BUDGET = option('--resident', 4500);
const BANK_SIZE = option('--bank', 4096);

const { chunks, edges } = buildChunks();
const code = chunks.filter((chunk) => chunk.kind === 'code');
const weight = (edge) => edge.jsr + edge.jmp + edge.table;

// 被其他块引用的权重 / 大小，越大越适合常驻
const inbound = new Map();
for (const edge of edges.values()) inbound.set(edge.to, (inbound.get(edge.to) ?? 0) + weight(edge));
const ranked = [...code].sort((left, right) => (inbound.get(right.id) ?? 0) / right.size - (inbound.get(left.id) ?? 0) / left.size);
const resident = new Set();
let used = 0;
for (const chunk of ranked) {
  if ((inbound.get(chunk.id) ?? 0) < 4) break;
  if (used + chunk.size > RESIDENT_BUDGET) continue;
  resident.add(chunk.id);
  used += chunk.size;
}

// 其余的块：凝聚聚类
let clusters = code.filter((chunk) => !resident.has(chunk.id)).map((chunk) => ({ members: [chunk.id], size: chunk.size }));
const clusterOf = new Map();
const rebuild = () => { clusterOf.clear(); clusters.forEach((cluster, index) => cluster.members.forEach((id) => clusterOf.set(id, index))); };
rebuild();
for (;;) {
  const pair = new Map();
  for (const edge of edges.values()) {
    const a = clusterOf.get(edge.from);
    const b = clusterOf.get(edge.to);
    if (a === undefined || b === undefined || a === b) continue;
    const key = a < b ? `${a},${b}` : `${b},${a}`;
    pair.set(key, (pair.get(key) ?? 0) + weight(edge));
  }
  let best = null;
  for (const [key, value] of pair) {
    const [a, b] = key.split(',').map(Number);
    if (clusters[a].size + clusters[b].size > BANK_SIZE) continue;
    if (best === null || value > best.value) best = { a, b, value };
  }
  if (best === null) break;
  clusters[best.a] = { members: [...clusters[best.a].members, ...clusters[best.b].members], size: clusters[best.a].size + clusters[best.b].size };
  clusters.splice(best.b, 1);
  rebuild();
}
// 常驻页还有余量时，再把“被别的簇调用得最多（按字节）”的簇搬进来：调用常驻页里的块不需要跳板
for (;;) {
  const incoming = new Map();
  for (const edge of edges.values()) {
    const a = clusterOf.get(edge.from);
    const b = clusterOf.get(edge.to);
    if (b === undefined || a === b) continue;
    incoming.set(b, (incoming.get(b) ?? 0) + weight(edge));
  }
  let best = null;
  for (const [index, value] of incoming) {
    if (used + clusters[index].size > RESIDENT_BUDGET) continue;
    const score = value / clusters[index].size;
    if (best === null || score > best.score) best = { index, score };
  }
  if (best === null) break;
  clusters[best.index].members.forEach((id) => resident.add(id));
  used += clusters[best.index].size;
  clusters.splice(best.index, 1);
  rebuild();
}
// 装箱：从大到小放进第一个放得下的库页
const banks = [];
for (const cluster of [...clusters].sort((left, right) => right.size - left.size)) {
  const bank = banks.find((entry) => entry.size + cluster.size <= BANK_SIZE);
  if (bank) { bank.members.push(...cluster.members); bank.size += cluster.size; } else banks.push({ members: [...cluster.members], size: cluster.size });
}
const location = new Map();
resident.forEach((id) => location.set(id, 'R'));
banks.forEach((bank, index) => bank.members.forEach((id) => location.set(id, String(index + 1))));

let crossRefs = 0;
const stubTargets = new Set();
for (const edge of edges.values()) {
  const from = location.get(edge.from);
  const to = location.get(edge.to);
  if (from === undefined || to === undefined || to === 'R' || from === to) continue;
  crossRefs += weight(edge);
  stubTargets.add(edge.to);
}
console.log(`常驻 ${resident.size} 块 ${used} 字节；库页 ${banks.length} 个：${banks.map((bank) => `${bank.size}字节/${bank.members.length}块`).join('，')}`);
console.log(`跨页引用 ${crossRefs} 处，需要跳板的目标块 ${stubTargets.size} 个`);
if (args.includes('--write')) {
  const lines = ['# 代码块分配：每行“块起点 位置”，位置 R 是常驻页，数字是库页号；由 tools/plan-banks.mjs 生成'];
  for (const chunk of code) lines.push(`${hex4(chunk.start)} ${location.get(chunk.id)}`);
  writeFileSync(join(ROOT, 'analysis', 'banks.txt'), `${lines.join('\n')}\n`);
  console.log('已写入 analysis/banks.txt');
}
