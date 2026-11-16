// ------------------------------------------------------------------------
// 名称：fc-shot.mjs
// 说明：在 Ember 的 FC 模式里运行原版 Family BASIC（可选不接 Family BASIC 键盘），运行指定帧数后截图，用来和学习机里新版的画面对照。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-16
// 备注：用法 node tools/fc-shot.mjs <输出 png> [帧数，默认 600] [--no-keyboard]
// ------------------------------------------------------------------------
import { writeFileSync } from 'node:fs';
import { frames, ROM, startEmber } from './lib/ember-fc.mjs';

const output = process.argv[2];
const count = Number(process.argv.find((value, index) => index > 2 && /^\d+$/.test(value)) ?? 600);
const keyboard = !process.argv.includes('--no-keyboard');
const session = await startEmber();
try {
  const { client } = session;
  await client.api.preferences.set({ id: 'input.familyKeyboard', value: keyboard });
  await client.api.cartridges.load({ path: ROM, powerOn: true, resume: false });
  await frames(client, count);
  writeFileSync(output, (await client.api.output.screenshot()).data);
  console.log('已保存', output);
} finally {
  session.close();
}
