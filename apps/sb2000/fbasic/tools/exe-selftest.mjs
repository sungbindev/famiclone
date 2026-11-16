// ------------------------------------------------------------------------
// 名称：exe-selftest.mjs
// 说明：在 Ember 的学习机里运行 exe/ 里的骨架并核对：钩子每帧运行、跨页调用与程序文本页无误、入口门往返、退出后 DOS 正常、重复运行页号不变。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-16
// 备注：用法 node tools/exe-selftest.mjs；先在 exe/ 里用 fcdev build 构建；截图在 build/selftest/。
// ------------------------------------------------------------------------
import { mkdirSync, writeFileSync } from 'node:fs';
import { dirname, join, resolve } from 'node:path';
import { fileURLToPath } from 'node:url';
import { frames, peek, startSb2000, tap, typeDos } from './lib/ember-sb2000.mjs';

const ROOT = resolve(dirname(fileURLToPath(import.meta.url)), '..');
const SHOTS = join(ROOT, 'build', 'selftest');
mkdirSync(SHOTS, { recursive: true });

const session = await startSb2000({ projectDir: join(ROOT, 'exe') });
const { client, symbols } = session;
const read = async (name) => (await peek(client, symbols.get(name), 1))[0];
const shot = async (name) => writeFileSync(join(SHOTS, `${name}.png`), (await client.api.output.screenshot()).data);
const problems = [];
const expect = (condition, text) => { console.log(`${condition ? '通过' : '失败'}：${text}`); if (!condition) problems.push(text); };

async function runProgram(label) {
  await typeDos(client, 'fbasic\n');
  await frames(client, 200);
  const before = { frame: await read('frame_count'), rounds: await read('rounds') };
  await frames(client, 600);
  const after = {
    frame: await read('frame_count'), rounds: await read('rounds'), bank: await read('bank_errors'), text: await read('text_errors'),
    window: await read('window_errors'), page: await read('probe_page'), error: await read('probe_error'), count: await read('probe_count'),
  };
  await shot(`${label}-running`);
  console.log(`${label}：`, JSON.stringify(after));
  expect(after.frame !== before.frame, `${label}：钩子每帧在运行`);
  expect(after.rounds !== before.rounds, `${label}：自检在循环`);
  expect(after.bank === 0 && after.text === 0 && after.window === 0, `${label}：跨页调用、程序文本页、窗口影子都没有错误`);
  expect(after.error === 0 && after.page !== 0 && after.count === 1, `${label}：入口门往返成功，申请到页 ${after.page}`);
  await tap(client, 'Escape');
  await frames(client, 120);
  await shot(`${label}-exited`);
  return after;
}

try {
  await frames(client, 2700);
  await tap(client, 'Enter');
  await frames(client, 20);
  await tap(client, 'Enter');
  await frames(client, 20);
  const first = await runProgram('第一次');
  await typeDos(client, 'dir\n');
  await frames(client, 120);
  await shot('dir');
  const second = await runProgram('第二次');
  expect(first.page === second.page, `两次运行申请到的页号相同（${first.page}、${second.page}），没有泄漏`);
} finally {
  session.close();
}
console.log(problems.length === 0 ? '全部通过' : `有 ${problems.length} 项失败`);
process.exit(problems.length === 0 ? 0 : 1);
