// 一次性调试脚本：进入图形编辑器后读零页关键单元与 PPU 空间。
import { startGbasic } from './lib/sb2000-session.mjs';
const session = await startGbasic();
try {
  await session.frames(300);
  await session.tap('Digit2');
  await session.frames(300);
  console.log('spaces', JSON.stringify(await session.client.request('inspector.listSpaces', {})));
  const zp = await session.read(0, 256);
  const show = (from, to) => [...zp.slice(from, to)].map((v) => v.toString(16).padStart(2, '0')).join(' ');
  console.log('ZP 30-4F:', show(0x30, 0x50));
  console.log('ZP 60-8F:', show(0x60, 0x90));
  console.log('ZP 90-BF:', show(0x90, 0xc0));
} finally {
  await session.close();
}
