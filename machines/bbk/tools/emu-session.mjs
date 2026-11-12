// ------------------------------------------------------------------------
// 名称：emu-session.mjs
// 说明：驱动调试版模拟器的小工具：启动、挂载软盘、运行若干帧、按键、截图。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-12
// 备注：用法 node tools/emu-session.mjs <rom> <软盘镜像|-> <命令...>
//       命令是用分号分隔的调试服务命令（见 packages/fcdev/docs/05-模拟器与调试.md 第 5.1 节），
//       另有 type:<文字> 按键输入一行文字（含回车）。路径写绝对路径。
//       每条命令的应答首行会打印；模拟器在结束时关闭。
// ------------------------------------------------------------------------

import { existsSync } from 'node:fs';
import { join } from 'node:path';
import { launchEmulator } from '../../../packages/fcdev/src/emulator/emulator-launcher.mjs';
import { REPO_ROOT } from './config.mjs';

/** 模拟器程序，优先用本仓库的编译输出 */
const EMULATOR_CANDIDATES = [
  join(REPO_ROOT, 'build', 'virtuanes', 'Release', 'bin', 'VirtuaNES.exe'),
  join(REPO_ROOT, 'packages', 'fcdev-emulator', 'bin', 'VirtuaNES.exe'),
];

/** 字符到 DirectInput 扫描码（Shift 状态）的映射，只覆盖在 DOS 提示符下常用的字符 */
const SCAN_CODES = {
  ...Object.fromEntries([...'abcdefghijklmnopqrstuvwxyz'].map((ch, i) => [ch, [[0x1e, 0x30, 0x2e, 0x20, 0x12, 0x21, 0x22, 0x23, 0x17, 0x24, 0x25, 0x26, 0x32, 0x31, 0x18, 0x19, 0x10, 0x13, 0x1f, 0x14, 0x16, 0x2f, 0x11, 0x2d, 0x15, 0x2c][i], false]])),
  ...Object.fromEntries([...'1234567890'].map((ch, i) => [ch, [0x02 + i, false]])),
  ' ': [0x39, false],
  '.': [0x34, false],
  '\\': [0x2b, false],
  ':': [0x27, true],
  '\n': [0x1c, false],
};

/**
 * 启动模拟器并连接调试服务。
 * @param {string} romFile 要加载的文件
 * @param {number} port 调试端口
 * @returns {Promise<{send: (command: string) => Promise<{ok: boolean, head: string, lines: string[]}>, close: () => void}>} 会话
 */
export async function startSession(romFile, port = 5710) {
  const executable = EMULATOR_CANDIDATES.find((path) => existsSync(path));
  if (!executable) throw new Error('找不到 VirtuaNES.exe，先运行 node tools/build-emulator.mjs');
  const { client } = await launchEmulator({ executable, romFile, debug: true, port });
  return {
    send: (command) => client.send(command),
    close: () => {
      client.send('quit').catch(() => {});
      client.close();
    },
  };
}

/**
 * 按键输入一段文字，每个键按下与松开各运行若干帧。
 * @param {{send: Function}} session 会话
 * @param {string} text 文字，大小写按小写处理，\n 为回车
 */
export async function typeText(session, text) {
  for (const raw of text) {
    const entry = SCAN_CODES[raw.toLowerCase()];
    if (!entry) throw new Error(`不支持的字符：${raw}`);
    const [code, shift] = entry;
    if (shift) await session.send('key 2a 1');
    await session.send(`key ${code.toString(16)} 1`);
    await session.send('run 6');
    await session.send(`key ${code.toString(16)} 0`);
    if (shift) await session.send('key 2a 0');
    await session.send('run 6');
  }
}

if (import.meta.url === `file:///${process.argv[1].replace(/\\/g, '/')}`) {
  const [romFile, disk, script] = process.argv.slice(2);
  if (!romFile || !script) {
    console.error('用法：node tools/emu-session.mjs <rom> <软盘镜像|-> "命令;命令;..."');
    process.exit(2);
  }
  const session = await startSession(romFile);
  try {
    if (disk && disk !== '-') console.log((await session.send(`disk ${disk}`)).head);
    for (const step of script.split(';').map((item) => item.trim()).filter(Boolean)) {
      if (step.startsWith('type:')) {
        await typeText(session, step.slice(5).replace(/\\n/g, '\n'));
        console.log(`${step} -> 完成`);
      } else {
        const reply = await session.send(step);
        console.log(`${step} -> ${reply.head}`);
      }
    }
  } finally {
    session.close();
  }
}
