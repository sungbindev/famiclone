// ------------------------------------------------------------------------
// 名称：fc-ppu-dump.mjs
// 说明：在 Ember 的 FC 模式里让原版 Family BASIC 开机并键入若干文字，导出 PPU 的名称表、调色板、控制寄存器影子与滚动变量，用来摸清原版的画面模型（阶段 4.2 的依据）。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-16
// 备注：用法 node tools/fc-ppu-dump.mjs [要键入的文字]；输出在 build/ppu-dump.json 并打印摘要；只用于分析原版。
// ------------------------------------------------------------------------
import { mkdirSync, writeFileSync } from 'node:fs';
import { dirname, join, resolve } from 'node:path';
import { fileURLToPath } from 'node:url';
import { bootFamilyBasic, frames, startEmber, typeText } from './lib/ember-fc.mjs';

const BUILD = join(resolve(dirname(fileURLToPath(import.meta.url)), '..'), 'build');
mkdirSync(BUILD, { recursive: true });
const text = (process.argv[2] ?? '').replaceAll('\\n', '\n');
const session = await startEmber();
const hex = (bytes) => [...bytes].map((value) => value.toString(16).padStart(2, '0')).join(' ');
try {
  const { client } = session;
  await bootFamilyBasic(client, 400);
  if (text) { await typeText(client, text); await frames(client, 60); }
  const read = async (space, address, length) => (await client.api.inspector.readMemory({ space, address, length })).data;
  const zero = await read('cpu', 0, 256);
  console.log('$32(ctrl) =', zero[0x32].toString(16), '$33(mask) =', zero[0x33].toString(16), '$4F =', zero[0x4f].toString(16), 'scroll $E4/$E5/$F0 =', hex([zero[0xe4], zero[0xe5], zero[0xf0]]));
  console.log('调色板 $3F00：', hex(await read('ppu', 0x3f00, 32)));
  for (const base of [0x2000, 0x2400, 0x2800, 0x2c00]) {
    const resolved = await client.api.inspector.resolve({ space: 'ppu', address: base });
    console.log(`PPU $${base.toString(16)} ->`, JSON.stringify(resolved).slice(0, 160));
  }
  const dump = {};
  for (const base of [0x2000, 0x2400, 0x2800, 0x2c00]) dump[base.toString(16)] = Buffer.from(await read('ppu', base, 0x400)).toString('hex');
  writeFileSync(join(BUILD, 'ppu-dump.json'), JSON.stringify({ zero: hex(zero), nametables: dump }));
  for (const base of [0x2000, 0x2400]) {
    const data = await read('ppu', base, 0x400);
    console.log(`--- NT $${base.toString(16)}（前 30 行，tile 码）`);
    for (let row = 0; row < 30; row += 1) {
      const line = data.slice(row * 32, row * 32 + 32);
      console.log(`${String(row).padStart(2)} ${[...line].map((value) => (value >= 0x20 && value < 0x7f ? String.fromCharCode(value) : '.')).join('')}  ${hex(line.slice(0, 6))}`);
    }
    console.log('属性表：', hex(data.slice(0x3c0, 0x400)));
  }
} finally {
  session.close();
}
