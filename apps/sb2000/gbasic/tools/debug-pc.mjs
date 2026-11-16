// 一次性调试脚本：运行若干帧后单步取样 PC，并反汇编停住的位置。
import { startGbasic } from './lib/sb2000-session.mjs';
const session = await startGbasic();
try {
  await session.frames(300);
  await session.tap('Digit1');
  await session.frames(300);
  const counts = new Map();
  for (let index = 0; index < 400; index += 1) {
    const result = await session.client.request('runtime.step', { unit: 'instruction', count: 7 });
    counts.set(result.pc, (counts.get(result.pc) ?? 0) + 1);
  }
  const top = [...counts].sort((a, b) => b[1] - a[1]).slice(0, 12);
  console.log('PC 取样：', top.map(([pc, n]) => `$${pc.toString(16)}×${n}`).join(' '));
  const last = [...counts.keys()].sort((a, b) => a - b);
  console.log('PC 范围：', last.slice(0, 40).map((pc) => pc.toString(16)).join(' '));
  const first = top[0][0];
  const dis = await session.client.request('inspector.disassemble', { address: first - 12, count: 14 });
  console.log(dis.instructions.map((item) => `${item.address.toString(16)} ${item.text}`).join('\n'));
} finally {
  await session.close();
}
