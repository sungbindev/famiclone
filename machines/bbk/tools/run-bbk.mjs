// ------------------------------------------------------------------------
// 名称：run-bbk.mjs
// 说明：在模拟器里验证装载器 START.EXE：用 SB2000 的 BIOS 启动 SB-DOS，运行 start 加载 bios.bbk，再运行若干帧并截图。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-12
// 备注：用法 node tools/run-bbk.mjs [帧数] [截图文件] [dos] [命令]；先构建 bios.bbk（node tools/port-bios.mjs、node tools/build-bios.mjs）
//       并在 apps/sb2000/startbbk 里构建（npm run build）。帧数缺省 300，截图缺省 build/work/shots/bbk.png。
//       第三个参数写 dos 时，软盘用步步高 DOS 启动盘加上 START.EXE、BIOS.BBK（build/work/dosboot.img），这样步步高系统起来后
//       可以从同一张盘启动 BBGDOS；缺省只含 START.EXE、BIOS.BBK（build/work/start.img）。
//       运行中先挂载软盘，等 SB-DOS 起来后输入 start；之后再运行指定帧数。
// ------------------------------------------------------------------------

import { existsSync, mkdirSync, readFileSync, writeFileSync } from 'node:fs';
import { dirname, join, resolve } from 'node:path';
import { buildFat12Image } from '../../sb2000/disassembly/tools/fat12-image.mjs';
import { BIN_DIR, DOS_FLOPPY, REPO_ROOT, WORK_DIR } from './config.mjs';
import { startSession, typeText } from './emu-session.mjs';
import { addFat12Files } from './fat12-add.mjs';

const firmware = join(REPO_ROOT, 'reference', 'rom', 'SB2K_BIOS.NES');
const loader = join(REPO_ROOT, 'apps', 'sb2000', 'startbbk', 'build', 'sb2000', 'START.EXE');
const bios = join(BIN_DIR, 'bios.bbk');
for (const file of [firmware, loader, bios, DOS_FLOPPY]) {
  if (!existsSync(file)) throw new Error(`找不到 ${file}`);
}

const frames = Number(process.argv[2] ?? 300);
const shot = resolve(process.argv[3] ?? join(WORK_DIR, 'shots', 'bbk.png'));
const useDos = process.argv[4] === 'dos';
const command = process.argv[5];

const files = [
  { name: 'START.EXE', data: readFileSync(loader) },
  { name: 'BIOS.BBK', data: readFileSync(bios) },
];
const image = join(WORK_DIR, useDos ? 'dosboot.img' : 'start.img');
mkdirSync(dirname(image), { recursive: true });
mkdirSync(dirname(shot), { recursive: true });
writeFileSync(image, useDos ? addFat12Files(readFileSync(DOS_FLOPPY), files) : buildFat12Image(files, { label: 'BBK' }));

const session = await startSession(firmware);
try {
  await session.send(`disk ${image}`);
  await session.send('reset');
  await session.send('run 1100');
  await typeText(session, '\n');
  await session.send('run 60');
  await typeText(session, '\n');
  await session.send('run 120');
  await typeText(session, 'start\n');
  await session.send(`run ${frames}`);
  if (useDos) {
    // BBGDOS 起来后先问日期与时间，回车跳过
    await typeText(session, '\n');
    await session.send('run 60');
    await typeText(session, '\n');
    await session.send('run 120');
    if (command) {
      await typeText(session, `${command}\n`);
      await session.send('run 300');
    }
  }
  const reply = await session.send(`shot ${shot}`);
  console.log(`截图：${reply.head}`);
  const status = await session.send('status');
  console.log(status.lines.join('\n'));
} finally {
  session.close();
}
