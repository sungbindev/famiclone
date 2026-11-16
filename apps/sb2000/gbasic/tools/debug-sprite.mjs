// 一次性调试脚本：进入图形编辑器后依次改写 $2008 的精灵图案表位，截图比较光标形状。
import { join } from 'node:path';
import { ROOT } from './lib/image-model.mjs';
import { startGbasic } from './lib/sb2000-session.mjs';
const session = await startGbasic();
try {
  await session.frames(300);
  await session.tap('Digit2');
  await session.frames(300);
  for (const value of [0x00, 0x04, 0x08, 0x0c]) {
    await session.client.request('inspector.writeMemory', { space: 'cpu', address: 0x2008, bytes: [value] });
    await session.frames(5);
    await session.shot(join(ROOT, 'build', 'session', `spr-${value.toString(16)}.png`));
  }
  const oam = await session.client.request('inspector.readMemory', { space: 'chip.oam', address: 0, length: 16 });
  console.log('OAM', Buffer.from(oam.data).toString('hex'));
} finally {
  await session.close();
}
