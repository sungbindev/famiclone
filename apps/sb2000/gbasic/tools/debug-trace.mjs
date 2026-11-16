// 一次性调试脚本：按 1 之后记录指令轨迹，打印跳进数据区之前的最后 60 条。
import { readFileSync } from 'node:fs';
import { startGbasic } from './lib/sb2000-session.mjs';
const session = await startGbasic();
try {
  await session.frames(300);
  const trace = await session.client.request('inspector.traceStart', { kind: 'instruction', maxLines: 400000 });
  await session.tap('Digit1');
  await session.frames(200);
  const stop = await session.client.request('inspector.traceStop', { id: trace.id });
  const lines = readFileSync(stop.file, 'utf8').split(/\r?\n/).filter(Boolean);
  console.log(`记录 ${lines.length} 行，文件 ${stop.file}`);
  const index = lines.findIndex((line) => /^E84A/i.test(line.trim()) || /\bE84A\b/i.test(line.slice(0, 8)));
  const from = index > 0 ? Math.max(0, index - 60) : Math.max(0, lines.length - 60);
  console.log(lines.slice(from, from + 70).join('\n'));
} finally {
  await session.close();
}
