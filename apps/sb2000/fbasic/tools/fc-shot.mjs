// ------------------------------------------------------------------------
// 名称：fc-shot.mjs
// 说明：在 Ember 的 FC 模式里运行原版 Family BASIC（可选不接 Family BASIC 键盘），可先键入一段文字，运行指定帧数后截图，用来和学习机里新版的画面对照。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-16
// 备注：用法 node tools/fc-shot.mjs <输出 png> [帧数，默认 600] [--no-keyboard] [--type <文字，\n 是回车>]
// ------------------------------------------------------------------------
import { writeFileSync } from 'node:fs';
import { bootFamilyBasic, frames, startEmber, typeText } from './lib/ember-fc.mjs';

const output = process.argv[2];
const count = Number(process.argv.find((value, index) => index > 2 && /^\d+$/.test(value)) ?? 600);
const keyboard = !process.argv.includes('--no-keyboard');
const typeIndex = process.argv.indexOf('--type');
const text = typeIndex >= 0 ? process.argv[typeIndex + 1].replaceAll('\\n', '\n') : '';
const session = await startEmber();
try {
  const { client } = session;
  if (keyboard) {
    await bootFamilyBasic(client, count);
  } else {
    await client.api.preferences.set({ id: 'input.familyKeyboard', value: false });
    await client.api.cartridges.load({ path: (await import('./lib/ember-fc.mjs')).ROM, powerOn: true, resume: false });
    await frames(client, count);
  }
  if (text) {
    await typeText(client, text);
    await frames(client, 120);
  }
  writeFileSync(output, (await client.api.output.screenshot()).data);
  console.log('已保存', output);
} finally {
  session.close();
}
