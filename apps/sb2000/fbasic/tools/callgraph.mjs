// ------------------------------------------------------------------------
// 名称：callgraph.mjs
// 说明：统计块（见 lib/chunks.mjs）的大小与块间引用，求 NMI 要用到的块（必须常驻），给出分页前的总量。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-16
// 备注：用法 node tools/callgraph.mjs [--list]；NMI 的入口是 $8971（NMI 向量指向的 RAM 小段跳到这里）与 $C8A3（精灵动画），再加 $89A3 地址表里的命令例程。
// ------------------------------------------------------------------------
import { BASE, hex4 } from './lib/image-model.mjs';
import { buildChunks } from './lib/chunks.mjs';

const { model, chunks, chunkAt, edges } = buildChunks();
const code = chunks.filter((chunk) => chunk.kind === 'code');
const data = chunks.filter((chunk) => chunk.kind === 'data');
const sum = (list) => list.reduce((total, chunk) => total + chunk.size, 0);
console.log(`块 ${chunks.length} 个：代码块 ${code.length} 个共 ${sum(code)} 字节，数据块 ${data.length} 个共 ${sum(data)} 字节；最大的代码块 ${Math.max(...code.map((chunk) => chunk.size))} 字节`);

const out = new Map();
for (const edge of edges.values()) { if (!out.has(edge.from)) out.set(edge.from, []); out.get(edge.from).push(edge); }
const nmiEntries = [0x8971, 0xc8a3];
for (const range of model.layout) {
  if (range.start === 0x89a3) for (let address = range.start; address <= range.end; address += 2) nmiEntries.push(model.bytes.readUInt16LE(address - BASE));
}
const seen = new Set();
const stack = nmiEntries.map((address) => chunkAt(address));
while (stack.length > 0) {
  const id = stack.pop();
  if (seen.has(id)) continue;
  seen.add(id);
  for (const edge of out.get(id) ?? []) stack.push(edge.to);
}
const nmiChunks = [...seen].map((id) => chunks[id]).sort((left, right) => left.start - right.start);
console.log(`NMI 要用到的块（必须常驻）：${nmiChunks.length} 个，共 ${sum(nmiChunks)} 字节（其中代码 ${sum(nmiChunks.filter((chunk) => chunk.kind === 'code'))}，数据 ${sum(nmiChunks.filter((chunk) => chunk.kind === 'data'))}）`);
if (process.argv.includes('--list')) for (const chunk of nmiChunks) console.log(`  ${hex4(chunk.start)}-${hex4(chunk.end)} ${chunk.kind} ${chunk.size}`);

const inWeight = new Map();
for (const edge of edges.values()) inWeight.set(edge.to, (inWeight.get(edge.to) ?? 0) + edge.jsr + edge.jmp + edge.table);
const hot = code.map((chunk) => ({ chunk, weight: inWeight.get(chunk.id) ?? 0 })).sort((left, right) => right.weight - left.weight).slice(0, 12);
console.log(`被引用最多的代码块：${hot.map(({ chunk, weight }) => `${hex4(chunk.start)}(${chunk.size}字节,${weight}次)`).join(' ')}`);
const sizes = code.map((chunk) => chunk.size).sort((left, right) => right - left);
console.log(`代码块大小：前 10 大 ${sizes.slice(0, 10).join(' ')}；中位数 ${sizes[Math.floor(sizes.length / 2)]}`);
