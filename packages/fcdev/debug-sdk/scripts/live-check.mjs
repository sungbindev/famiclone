/*
 * ------------------------------------------------------------------------
 * 名称：live-check.mjs
 * 说明：对正在运行的 Ember 做真实联调：SDK 连接与握手、类型化请求、截图二进制帧、事件订阅，以及终端客户端。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-14
 * 备注：先 npm run debug-sdk:build（在 packages/fcdev 目录里）；要有开启了调试服务的 Ember 在运行，地址与令牌取环境变量 EMBER_DEBUG_URL、EMBER_DEBUG_TOKEN，
 *       或端点文件（EMBER_USER_DATA 指向它的用户数据目录）；EMBER_LIVE_ROM 指定要装入的 NES 文件。
 * ------------------------------------------------------------------------
 */

import { execFileSync } from 'node:child_process';
import { existsSync, mkdtempSync, readFileSync, rmSync, statSync } from 'node:fs';
import { tmpdir } from 'node:os';
import { dirname, join } from 'node:path';
import { fileURLToPath, pathToFileURL } from 'node:url';

const dist = join(dirname(fileURLToPath(import.meta.url)), '..', '..', 'dist', 'debug');
const rom = process.env.EMBER_LIVE_ROM;
if (rom === undefined || !existsSync(rom)) {
  console.error('请用环境变量 EMBER_LIVE_ROM 指定一个存在的 NES 文件');
  process.exit(2);
}

const { connectToEmber } = await import(pathToFileURL(join(dist, 'node.js')).href);
let failed = 0;

/** 记录一项检查。 */
function check(name, ok, detail = '') {
  console.log(`${ok ? '通过' : '失败'}：${name}${detail === '' ? '' : `（${detail}）`}`);
  if (!ok) failed += 1;
}

const client = await connectToEmber({ clientName: 'live-check' });
check('连接并握手', client.info.protocol === 1 && typeof client.info.appVersion === 'string', JSON.stringify(client.info));

const status = await client.api.runtime.status();
check('api.runtime.status 返回运行状态', typeof status.power === 'string', `${status.power}/${status.mode}`);

const loaded = await client.api.cartridges.load({ path: rom });
check('api.cartridges.load 装入卡带并上电', loaded.state.power === 'on', loaded.cartridge.pluginId);

await client.subscribe(['runtime.*']);
const finished = client.waitFor('runtime.runFinished', { timeoutMs: 20_000 });
const run = await client.api.runtime.run({ frames: 120, wait: true });
const event = await finished;
check('runtime.run 等待跑完 120 帧', run.stopped === 'completed' && run.frames === 120, JSON.stringify(run));
check('收到 runtime.runFinished 事件', event.frames === 120 && event.stopped === 'completed');

const raw = await client.api.output.screenshot({ format: 'rgba' });
check('截图（RGBA）以二进制帧到达，data 是 Uint8Array', raw.data instanceof Uint8Array && raw.data.byteLength === 256 * 240 * 4 && raw.width === 256, `${raw.data?.byteLength}`);
const png = await client.api.output.screenshot();
check('截图（PNG）的数据以 PNG 签名开头', png.data instanceof Uint8Array && png.data[0] === 0x89 && png.data[1] === 0x50 && png.format === 'png', `${png.byteLength} 字节`);
const nonBlack = raw.data.some((value, index) => index % 4 !== 3 && value !== 0);
check('画面不是全黑', nonBlack);

await client.api.input.pad({ port: 0, buttons: 8 });
await client.api.input.releaseAll();
check('input.pad 与 input.releaseAll 成功', true);

// 检查器：暂停后单步、读状态、读写内存、解析地址。
await client.api.runtime.pause();
const stepped = await client.api.runtime.step({ unit: 'instruction', count: 3 });
const cpuState = (await client.api.inspector.getState({ module: 'cpu' })).states[0];
const pcField = cpuState?.fields.find((item) => item.name === 'pc');
check('runtime.step 单步 3 条指令后暂停，pc 与 getState 一致', stepped.instructions === 3 && stepped.state.mode === 'paused' && pcField?.value === stepped.pc, JSON.stringify({ pc: stepped.pc, cycles: stepped.cycles }));
const spaces = (await client.api.inspector.listSpaces()).spaces.map((item) => item.id);
check('inspector.listSpaces 含 cpu、board.ram 与卡带的存储区', ['cpu', 'board.ram', 'cart.prg'].every((id) => spaces.includes(id)), spaces.join('、'));
const prg = await client.api.inspector.readMemory({ space: 'cart.prg', address: 0, length: 16 });
const viaCpu = await client.api.inspector.readMemory({ space: 'cpu', address: 0x8000, length: 16 });
check('inspector.readMemory 以二进制帧返回，cpu 空间与卡带存储区读到相同的字节', prg.data instanceof Uint8Array && prg.data.byteLength === 16 && Buffer.from(prg.data).equals(Buffer.from(viaCpu.data)));
await client.api.inspector.writeMemory({ space: 'board.ram', address: 0x7f0, bytes: [1, 2, 3, 4] });
const ram = await client.api.inspector.readMemory({ space: 'board.ram', address: 0x7f0, length: 4 });
check('inspector.writeMemory 写内部 RAM 后读回', Buffer.from(ram.data).equals(Buffer.from([1, 2, 3, 4])));
const resolved = (await client.api.inspector.resolve({ space: 'cpu', address: 0x8000 })).target;
check('inspector.resolve 把 $8000 解析到卡带的 PRG', resolved.kind === 'rom' && resolved.region === 'cart.prg', JSON.stringify(resolved));
await expectRequestError(() => client.api.inspector.readMemory({ space: 'nowhere', address: 0, length: 1 }), 'inspector.unknownSpace');
const poked = (await client.api.inspector.setState({ module: 'cpu', fields: [{ name: 'a', value: 0x5a }] })).state;
check('inspector.setState 修改 cpu 的 a', poked.fields.find((item) => item.name === 'a')?.value === 0x5a);

// 断点：对 $8000-$FFFF 设读断点，继续运行后第一次取指就命中，机器暂停。
const added = (await client.api.inspector.addBreakpoint({ kind: 'read', address: 0x8000, endAddress: 0xffff })).breakpoint;
const hitEvent = client.waitFor('runtime.breakpointHit', { timeoutMs: 20_000 });
await client.api.runtime.resume();
const hit = await hitEvent;
const afterHit = await client.api.runtime.status();
check('读断点命中：事件带断点编号与寄存器，机器暂停', hit.id === added.id && hit.kind === 'read' && hit.address >= 0x8000 && typeof hit.pc === 'number' && afterHit.mode === 'paused', JSON.stringify(hit));
const listed = (await client.api.inspector.listBreakpoints()).breakpoints;
check('inspector.listBreakpoints 列出断点并累计命中次数', listed.length === 1 && listed[0].hits >= 1);
const cleared = await client.api.inspector.clearBreakpoints();
check('inspector.clearBreakpoints 删除全部断点', cleared.removed === 1);
await expectRequestError(() => client.api.inspector.removeBreakpoint({ id: added.id }), 'inspector.noBreakpoint');

// 监视与快照：监视 cpu.pc，单步后收到 inspector.stateChanged。
await client.subscribe(['inspector.*']);
const watched = (await client.api.inspector.watch({ path: 'cpu.pc' })).watch;
const changedEvent = client.waitFor('inspector.stateChanged', { timeoutMs: 20_000 });
await client.api.runtime.step({ unit: 'instruction' });
const change = await changedEvent;
const snap = await client.api.inspector.snapshot();
check('监视 cpu.pc：单步后收到 inspector.stateChanged，旧值是监视时的取值', change.path === 'cpu.pc' && change.oldValue === watched.value && change.newValue !== change.oldValue, JSON.stringify(change));
check('inspector.snapshot 含各模块状态与监视项', snap.states.length >= 2 && snap.watches.length === 1 && snap.watches[0].value === change.newValue);
await client.api.inspector.unwatch({ path: 'cpu.pc' });
await expectRequestError(() => client.api.inspector.unwatch({ path: 'cpu.pc' }), 'inspector.noWatch');

// 跟踪与调用记录：单步时记录，结束时写成调试输出目录下的文件。
const trace = await client.api.inspector.traceStart({ kind: 'instruction', file: 'live-trace.txt' });
await client.api.runtime.step({ unit: 'instruction', count: 3 });
const traced = await client.api.inspector.traceStop({ id: trace.id });
const traceLines = readFileSync(traced.file, 'utf8').split('\n').filter((line) => line !== '');
check('指令跟踪：单步 3 条指令写出 3 行，每行以地址开头', traced.lines === 3 && traceLines.length === 3 && /^[0-9A-F]{4} {2}/.test(traceLines[0]), traceLines[0]);
const pcNow = (await client.api.inspector.getState({ module: 'cpu' })).states[0].fields.find((item) => item.name === 'pc').value;
const callLog = await client.api.inspector.callLogStart({ address: pcNow, stackBytes: 4, file: 'live-calls.txt' });
await client.api.runtime.step({ unit: 'instruction' });
const logged = await client.api.inspector.callLogStop({ id: callLog.id });
check('调用记录：执行到指定地址时记一行，含栈顶', logged.lines === 1 && readFileSync(logged.file, 'utf8').includes(' | STACK '), JSON.stringify(logged));
await expectRequestError(() => client.api.inspector.traceStop({ id: trace.id }), 'inspector.noRecording');

// 反汇编与代码数据标记（CDL）。
const listing = (await client.api.inspector.disassemble({ address: pcNow, count: 3 })).instructions;
check('inspector.disassemble 返回 3 条指令，首条地址是当前 pc', listing.length === 3 && listing[0].address === pcNow && listing[0].text !== '', JSON.stringify(listing[0]));
const cdl = await client.api.inspector.cdlStart();
await client.api.runtime.step({ unit: 'instruction', count: 20 });
const cdlStopped = await client.api.inspector.cdlStop();
const cdlSaved = await client.api.inspector.cdlSave({ file: 'live.cdl' });
check('代码数据标记：单步 20 条指令后有 Code 标记，文件大小等于 PRG 大小', cdlStopped.codeBytes >= 20 && statSync(cdlSaved.file).size === cdl.size, JSON.stringify(cdlStopped));
await expectRequestError(() => client.api.input.pad({ port: 7, buttons: 1 }), 'input.invalidPort');
await expectRequestError(() => client.api.runtime.run({ frames: 0 }), 'core.invalidParams');

/** 请求应以给定错误码失败。 */
async function expectRequestError(action, code) {
  try {
    await action();
    check(`错误码 ${code}`, false, '没有失败');
  } catch (error) {
    check(`错误码 ${code}`, error?.code === code, `${error?.code}`);
  }
}

const discovered = await client.api.rpc.discover();
check('rpc.discover 返回 OpenRPC 文档', discovered.openrpc === '1.3.2' && discovered.methods.length > 30, `${discovered.methods.length} 个方法`);
client.close();
await client.closed;
check('关闭后 closed 兑现', true);

// 终端客户端：读端点文件，不给地址与令牌。
const cli = (...args) => execFileSync(process.execPath, [join(dist, 'cli.js'), ...args], { encoding: 'utf8', env: process.env });
check('fcdev debug endpoint 显示地址', JSON.parse(cli('endpoint', '--compact')).url.startsWith('ws://127.0.0.1:'));
check('fcdev debug call runtime.status', JSON.parse(cli('call', 'runtime.status', '--compact')).power === 'on');
const work = mkdtempSync(join(tmpdir(), 'ember-live-'));
const shot = join(work, 'shot.png');
cli('screenshot', shot);
check('fcdev debug screenshot 保存 PNG', readFileSync(shot).subarray(1, 4).toString() === 'PNG');
rmSync(work, { recursive: true, force: true });

console.log(failed === 0 ? '联调全部通过' : `联调有 ${failed} 项失败`);
process.exit(failed === 0 ? 0 : 1);
