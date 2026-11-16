// ------------------------------------------------------------------------
// 名称：exe-type.mjs
// 说明：在学习机里运行 FBASIC.EXE，按字符键入一段文字（美式键盘），等画面稳定后截图，用来和 FC 模式里原版键入同一段文字的截图对照。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-16
// 备注：用法 node tools/exe-type.mjs <输出 png> <文字，\n 是回车> [--exit]；--exit 在键入后按 F12 退出，再截一张退出后的画面（输出名加 -exit）。
// ------------------------------------------------------------------------
import { writeFileSync } from 'node:fs';
import { dirname, join, resolve } from 'node:path';
import { fileURLToPath } from 'node:url';
import { bootAndRun, frames, startSb2000, tap, typePc } from './lib/ember-sb2000.mjs';

const ROOT = resolve(dirname(fileURLToPath(import.meta.url)), '..');
const output = process.argv[2];
const text = (process.argv[3] ?? '').replaceAll('\\n', '\n');
const exitAfter = process.argv.includes('--exit');
const session = await startSb2000({ projectDir: join(ROOT, 'exe') });
const { client } = session;
const shot = async (path) => writeFileSync(path, (await client.api.output.screenshot()).data);
try {
  await bootAndRun(client);
  await frames(client, 400);
  await typePc(client, text, { hold: Number(process.env.HOLD ?? 6), gap: Number(process.env.GAP ?? 6) });
  await frames(client, Number(process.env.AFTER ?? 120));
  await shot(output);
  console.log('已保存', output);
  if (exitAfter) {
    await tap(client, 'F12');
    await frames(client, 300);
    await shot(output.replace(/\.png$/, '-exit.png'));
    console.log('已保存退出后的画面');
  }
} finally {
  session.close();
}
