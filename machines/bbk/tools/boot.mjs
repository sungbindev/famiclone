// ------------------------------------------------------------------------
// 名称：boot.mjs
// 说明：在模拟器里启动 ROM 并加载 DOS 启动盘，回车通过日期与时间提示，直到出现命令提示符，保存截图。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-12
// 备注：用法 node tools/boot.mjs [--rom <文件>] [--command <命令>] [--shot <png>]
//       缺省加载 build/bbk_src.nes（先运行 node tools/build-nes.mjs），截图写到 build/boot.png；
//       --command 指定时在提示符下输入该命令（如 dir）再截图。
//       是否成功要看截图里有没有“A:\>”，本工具不识别画面文字。
// ------------------------------------------------------------------------

import { copyFileSync, mkdirSync } from 'node:fs';
import { join } from 'node:path';
import { BUILD_DIR, DOS_FLOPPY } from './config.mjs';
import { startSession, typeText } from './emu-session.mjs';

const args = process.argv.slice(2);
const option = (name, fallback) => {
  const index = args.indexOf(name);
  return index >= 0 ? args[index + 1] : fallback;
};

const rom = option('--rom', join(BUILD_DIR, 'bbk_src.nes'));
const command = option('--command', '');
const shot = option('--shot', join(BUILD_DIR, 'boot.png'));

// 调试服务按本机代码页解析路径，盘符名带中文时先复制成纯英文文件名
mkdirSync(BUILD_DIR, { recursive: true });
const floppy = join(BUILD_DIR, 'dos.img');
copyFileSync(DOS_FLOPPY, floppy);

const session = await startSession(rom);
try {
  console.log((await session.send(`disk ${floppy}`)).head);
  await session.send('reset');
  await session.send('run 900');
  await typeText(session, '\n');
  await session.send('run 120');
  await typeText(session, '\n');
  await session.send('run 300');
  if (command) {
    await typeText(session, `${command}\n`);
    await session.send('run 300');
  }
  console.log((await session.send(`shot ${shot}`)).head);
  console.log(`截图：${shot}`);
} finally {
  session.close();
}
