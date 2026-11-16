// ------------------------------------------------------------------------
// 名称：exe-trace.mjs
// 说明：在 Ember 的学习机里运行 FBASIC.EXE，并跟踪程序运行期间执行过的指令（每个地址只记第一次），把最后几十个新地址按符号名打印出来，用来找移植版跑偏的位置。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-16
// 备注：用法 node tools/exe-trace.mjs [运行帧数，默认 300] [打印条数，默认 60]；先用 fcdev build 构建 exe/src。
// ------------------------------------------------------------------------
import { readFileSync } from 'node:fs';
import { dirname, join, resolve } from 'node:path';
import { fileURLToPath } from 'node:url';
import { frames, startSb2000, tap, typeDos } from './lib/ember-sb2000.mjs';

const ROOT = resolve(dirname(fileURLToPath(import.meta.url)), '..');
const total = Number(process.argv[2] ?? 300);
const count = Number(process.argv[3] ?? 60);

const session = await startSb2000({ projectDir: join(ROOT, 'exe') });
const { client, symbols } = session;
const sorted = [...symbols].map(([name, address]) => ({ name, address })).sort((left, right) => left.address - right.address);
const nameOf = (address) => {
  let best = null;
  for (const entry of sorted) { if (entry.address <= address && (best === null || entry.address >= best.address)) best = entry; }
  return best === null ? '' : `${best.name}+${address - best.address}`;
};
try {
  await frames(client, 2700);
  await tap(client, 'Enter');
  await frames(client, 20);
  await tap(client, 'Enter');
  await frames(client, 20);
  await typeDos(client, 'fbasic');
  const { id } = await client.api.inspector.traceStart({ kind: 'instruction', unique: true, maxLines: 40000, file: 'fbasic-trace.txt' });
  await tap(client, 'Enter');
  await frames(client, total);
  const result = await client.api.inspector.traceStop({ id });
  const lines = readFileSync(result.file, 'utf8').split(/\r?\n/).filter((line) => line.trim() !== '');
  const parsed = lines.map((line) => ({ text: line.trim(), address: Number.parseInt(/^([0-9A-Fa-f]{4})/.exec(line.trim())?.[1] ?? '0', 16) }));
  const wild = parsed.findIndex((entry, index) => index > 0 && entry.address < 0x8000 && parsed[index - 1].address >= 0x8000 && parsed[index - 1].address < 0xc000);
  const from = wild >= 0 ? Math.max(0, wild - count) : Math.max(0, parsed.length - count);
  console.log(`共 ${result.lines} 行；${wild >= 0 ? `第一次跑进 RAM 在第 ${wild} 行，` : ''}从第 ${from} 行起列 ${count} 行：`);
  for (const entry of parsed.slice(from, from + count + 3)) {
    console.log(`${entry.text.slice(0, 70).padEnd(72)}${entry.address >= 0x8000 && entry.address < 0xc000 ? nameOf(entry.address) : ''}`);
  }
} finally {
  session.close();
}
