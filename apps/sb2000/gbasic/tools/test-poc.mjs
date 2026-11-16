// ------------------------------------------------------------------------
// 名称：test-poc.mjs
// 说明：机制验证（阶段 4.1）：在 Ember 里运行 GBASIC.EXE 的验证映像，核对 NMI 每帧推进、入口门往返、ESC 退出后 DOS 正常。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-16
// 备注：先 node tools/build-image.mjs，再在 exe/ 里 fcdev build src，最后 node tools/test-poc.mjs；截图写到 build/session/。
// ------------------------------------------------------------------------
import { join } from 'node:path';
import { startGbasic } from './lib/sb2000-session.mjs';
import { ROOT } from './lib/image-model.mjs';

const out = (name) => join(ROOT, 'build', 'session', name);
const session = await startGbasic();
try {
  await session.frames(120);
  const first = await session.read(0x10, 1);
  await session.frames(30);
  const second = await session.read(0x10, 1);
  console.log(`NMI 帧计数：${first[0]} -> ${second[0]}`);
  await session.shot(out('poc-1-running.png'));
  const windows = await session.read(0x4040, 8).catch(() => null);
  console.log(`窗口寄存器读回：${windows ? [...windows].map((v) => v.toString(16)).join(' ') : '不可读'}`);

  await session.tap('KeyG');
  await session.frames(60);
  const probe = await session.read(0x11, 3);
  console.log(`入口门：页号 $${probe[0].toString(16)}，出错码 ${probe[1]}，往返次数 ${probe[2]}`);
  const third = await session.read(0x10, 1);
  await session.frames(30);
  const fourth = await session.read(0x10, 1);
  console.log(`入口门之后 NMI 帧计数：${third[0]} -> ${fourth[0]}`);
  await session.shot(out('poc-2-after-gate.png'));

  await session.tap('Escape');
  await session.frames(120);
  await session.shot(out('poc-3-back-to-dos.png'));
  const zero = await session.read(0x0000, 4);
  console.log(`退出后零页前 4 字节：${[...zero].map((v) => v.toString(16)).join(' ')}`);
} finally {
  await session.close();
}
