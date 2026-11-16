// ------------------------------------------------------------------------
// 名称：exe-run.mjs
// 说明：在 Ember 的学习机里运行 exe/ 里的 FBASIC.EXE，运行一段时间后采样 CPU 的 PC 与几个调试变量，并截图，用来看移植版有没有跑起来。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-16
// 备注：用法 node tools/exe-run.mjs [运行帧数，默认 600]；先在仓库根目录用 fcdev build 构建 exe/src；截图在 build/run/。
// ------------------------------------------------------------------------
import { mkdirSync, writeFileSync } from 'node:fs';
import { dirname, join, resolve } from 'node:path';
import { fileURLToPath } from 'node:url';
import { bootAndRun, frames, peek, startSb2000 } from './lib/ember-sb2000.mjs';
import { createLookup } from './lib/lookup.mjs';

const ROOT = resolve(dirname(fileURLToPath(import.meta.url)), '..');
const SHOTS = join(ROOT, 'build', 'run');
mkdirSync(SHOTS, { recursive: true });
const total = Number(process.argv[2] ?? 600);

const session = await startSb2000({ projectDir: join(ROOT, 'exe') });
const { client, symbols } = session;
const lookup = createLookup(join(ROOT, 'exe', 'build', 'obj', 'sb2000', 'fbasic', 'fbasic.dbg'));
const read = async (name) => (await peek(client, symbols.get(name), 1))[0];
const pc = async () => (await client.api.inspector.getState({ module: 'cpu' })).states[0];
try {
  await bootAndRun(client);
  await frames(client, total);
  const samples = [];
  for (let step = 0; step < 4; step += 1) {
    await frames(client, 7);
    const state = await pc();
    samples.push(`${state.fields.find((field) => field.name === 'pc').value.toString(16)} ${lookup(state.fields.find((field) => field.name === 'pc').value, await read('bank_current'))}`);
  }
  console.log('PC 采样：\n', samples.join('\n '));
  console.log('frame_count', await read('frame_count'), 'bank_current', await read('bank_current'));
  const cell = async (address) => (await peek(client, address, 1))[0];
  const zero = await peek(client, 0x00, 0x10);
  console.log('零页 $00-$0F：', [...zero].map((value) => value.toString(16).padStart(2, '0')).join(' '));
  console.log('程序文本页 $B000-$B00F：', [...(await peek(client, 0xb000, 16))].map((value) => value.toString(16).padStart(2, '0')).join(' '));
  console.log('PPU_CTRL_SHADOW', (await cell(0x062a)).toString(16), 'NMI_FLAGS', (await cell(0x0630)).toString(16), '$63', (await cell(0x63)).toString(16), 'hook', (await cell(0x1e)).toString(16), (await cell(0x1f)).toString(16));
  writeFileSync(join(SHOTS, 'run.png'), (await client.api.output.screenshot()).data);
} finally {
  session.close();
}
