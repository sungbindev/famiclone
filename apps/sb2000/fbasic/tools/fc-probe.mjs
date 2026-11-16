// 试跑：启动 Ember，载入 Family BASIC，截图，用于确认开机画面与键入。用法 node tools/fc-probe.mjs [要键入的文字]
import { mkdirSync } from 'node:fs';
import { dirname, join, resolve } from 'node:path';
import { fileURLToPath } from 'node:url';
import { bootFamilyBasic, frames, screenshot, startEmber, typeText } from './lib/ember-fc.mjs';

const BUILD = join(resolve(dirname(fileURLToPath(import.meta.url)), '..'), 'build');
mkdirSync(BUILD, { recursive: true });
const session = await startEmber();
try {
  await bootFamilyBasic(session.client, 200);
  await screenshot(session.client, join(BUILD, 'probe-0-boot.png'));
  const text = process.argv[2];
  if (text) {
    await typeText(session.client, text.replaceAll('\\n', '\n'));
    await frames(session.client, 30);
    await screenshot(session.client, join(BUILD, 'probe-1-typed.png'));
  }
  console.log('完成，Ember 进程号', session.pid);
} finally {
  session.close();
}
