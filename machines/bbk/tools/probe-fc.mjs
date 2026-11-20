// ------------------------------------------------------------------------
// 名称：probe-fc.mjs
// 说明：运行 apps/sb2000/fcprobe 的探测程序：在 Ember 里用 SB2000 的 BIOS 启动 SB-DOS，运行 fcprobe，
//       读出它在 FC 兼容模式里的测试结果并逐项打印。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-21
// 备注：用法 node tools/probe-fc.mjs；先在 apps/sb2000/fcprobe 里构建并生成软盘镜像：
//       node ../../../packages/fcdev/bin/fcdev.mjs build src；node ../../../packages/fcdev/bin/fcdev.mjs disk src。
//       Ember 按 fcdev 的规则定位（环境变量 FCDEV_EMULATOR，没有时用随包的 Ember）；结束时只结束自己启动的进程。
//       结果区在 ERAM $7F00 起，各项的含义见 probe-stub.s 的文件头。
// ------------------------------------------------------------------------

import { execFileSync } from 'node:child_process';
import { existsSync, rmSync } from 'node:fs';
import net from 'node:net';
import { join } from 'node:path';
import { debugUserDataDirectory, enterProgram, launchEmulator, locateEmulator, runFrames } from '../../../packages/fcdev/src/emulator/index.mjs';
import { REPO_ROOT } from './config.mjs';

const firmware = join(REPO_ROOT, 'reference', 'rom', '小霸王', 'SB2K_BIOS.NES');
const image = join(REPO_ROOT, 'apps', 'sb2000', 'fcprobe', 'build', 'sb2000', 'FCPROBE.img');
for (const file of [firmware, image]) {
  if (!existsSync(file)) throw new Error(`找不到 ${file}`);
}

/** 程序进入 FC 模式后运行的帧数，足够探测桩跑完 */
const PROBE_FRAMES = 200;

/** 按键注入后运行的帧数，让探测桩读到 */
const KEY_FRAMES = 20;

/** 找一个空闲的本机端口 */
function findFreePort() {
  return new Promise((done) => {
    const server = net.createServer().listen(0, '127.0.0.1', () => {
      const { port } = server.address();
      server.close(() => done(port));
    });
  });
}

/** 读 CPU 地址空间，返回字节数组 */
async function readMemory(client, address, length) {
  const result = await client.request('inspector.readMemory', { space: 'cpu', address, length });
  return [...result.data];
}

const hex = (value) => `$${value.toString(16).toUpperCase().padStart(2, '0')}`;
const port = await findFreePort();
const { pid, client } = await launchEmulator({ executable: locateEmulator().executable, sb2000: { firmware, diskImage: image }, debug: true, port });
try {
  await enterProgram(client, 'fcprobe');
  const run = await runFrames(client, PROBE_FRAMES);

  const r = await readMemory(client, 0x7f00, 0x60);
  console.log(`运行：${run.frames} 帧，停止原因 ${run.stopped}`);
  console.log(`启动标记 ${hex(r[0])}（应为 $01），全部测完标记 ${hex(r[2])}（应为 $FF），心跳 ${hex(r[1])}`);

  const romTests = [['$9100', 0x10], ['$A100', 0x12], ['$C100', 0x14], ['$E100', 0x16]];
  console.log('T1 $8000 以上写 0（预置 $AA，写后仍是 $AA 就是没写进去）：');
  for (const [name, slot] of romTests) console.log(`  ${name}：写前 ${hex(r[slot])}，写后 ${hex(r[slot + 1])}${r[slot + 1] === r[slot] ? '，写入无效' : '，写入生效'}`);

  const ramTests = [['$0300', 0x20], ['$5100', 0x21], ['$6800', 0x22], ['$7000', 0x23]];
  console.log('T2 写 $5A 后读回：');
  for (const [name, slot] of ramTests) console.log(`  ${name}：${hex(r[slot])}${r[slot] === 0x5a ? '，可写' : '，不可写'}`);

  console.log('T3 标准 PPU：');
  console.log(`  名表 $2000 写 $5A 读回：${hex(r[0x30])}${r[0x30] === 0x5a ? '，可写' : '，不可写'}`);
  console.log(`  图案表 $0000 经 $2007 写 $A5 读回：${hex(r[0x31])}${r[0x31] === 0xa5 ? '，可写' : '，不可写'}`);
  console.log(`  DMA 把扩展 RAM 的 $AA 复制到图案表后读回：${hex(r[0x32])}${r[0x32] === 0xaa ? '，DMA 有效' : '，DMA 无效'}`);

  console.log(`T4 软驱：复位后主状态 ${hex(r[0x40])}；发 $08 后主状态 ${hex(r[0x41])}，ST0 ${hex(r[0x42])}，主状态 ${hex(r[0x43])}，PCN ${hex(r[0x44])}，主状态 ${hex(r[0x45])}`);

  const keys = [];
  for (const [label, code, down] of [['无按键'], ['按下回车', 'Enter', true], ['松开回车', 'Enter', false], ['按下 A', 'KeyA', true]]) {
    if (code) {
      await client.request('input.key', { code, down });
      await runFrames(client, KEY_FRAMES);
    }
    const k = await readMemory(client, 0x7f50, 4);
    keys.push(`${label}：$4020=${hex(k[0])} $4022=${hex(k[1])} $4033=${hex(k[2])} $4026=${hex(k[3])}`);
  }
  console.log('T5 键盘与中断寄存器（FC 模式下注入按键后的读数）：');
  for (const line of keys) console.log(`  ${line}`);
  await client.request('input.key', { code: 'KeyA', down: false });
} finally {
  client.close();
  try {
    execFileSync('taskkill', ['/PID', String(pid), '/T', '/F'], { stdio: 'ignore' });
  } catch {
    // 进程已经退出
  }
  try {
    rmSync(debugUserDataDirectory(port), { recursive: true, force: true, maxRetries: 10, retryDelay: 300 });
  } catch {
    // 目录还被占用时留在系统临时目录里
  }
}
