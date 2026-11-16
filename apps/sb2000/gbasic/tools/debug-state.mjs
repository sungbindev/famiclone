// 一次性调试脚本：运行若干帧后打印 CPU 寄存器与零页关键单元。
import { startGbasic } from './lib/sb2000-session.mjs';
const session = await startGbasic();
try {
  await session.frames(300);
  await session.tap('Digit1');
  await session.frames(300);
  for (const method of ['runtime.status', 'inspector.readRegisters', 'cpu.registers']) {
    try { console.log(method, JSON.stringify(await session.client.request(method, {}))); } catch (error) { console.log(method, '失败', String(error.message ?? error).slice(0, 120)); }
  }
  const zp = await session.read(0, 256);
  console.log('ZP 30-6F:', [...zp.slice(0x30, 0x70)].map((v) => v.toString(16).padStart(2, '0')).join(' '));
  console.log('ZP 00-2F:', [...zp.slice(0, 0x30)].map((v) => v.toString(16).padStart(2, '0')).join(' '));
} finally {
  await session.close();
}
