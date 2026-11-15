// ------------------------------------------------------------------------
// 名称：sb2000-session.mjs
// 说明：学习机（SB2000）在 Ember 里的启动与操作：经调试服务添加固件、放入磁盘镜像、整机开机，并在 SB-DOS 里输入程序名运行；还有按键与运行帧数的小工具。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-15
// 备注：固件是学习机的 BIOS（iNES 文件），Ember 把它复制进自己的 BIOS 目录；磁盘镜像按 fcdev disk 生成，里面只有程序，开机后进入 SB-DOS 命令行，要输入程序名才运行。
//       帧数按 Ember 的运行速度估计：runtime.run 不限速，几千帧只要几秒；SB-DOS 启动后先问日期与时间，回车跳过。
// ------------------------------------------------------------------------
import { existsSync } from 'node:fs';
import { FcdevError } from '../shared/index.mjs';

/** 整机机型的标识 */
export const SB2000_MACHINE = 'sb2000';

/** 开机到 SB-DOS 询问日期所需的帧数 */
export const DOS_PROMPT_FRAMES = 2600;

/** 按键按下与松开之后各运行的帧数，让 BIOS 读到 */
const KEY_DOWN_FRAMES = 5;
const KEY_UP_FRAMES = 4;

/** 一次运行请求等待的上限，单位毫秒 */
const RUN_TIMEOUT_MS = 120_000;

/**
 * 字符对应的物理键位（KeyboardEvent.code）。
 * @param {string} char 程序名里的一个字符，只支持字母与数字
 * @returns {string} 键位
 * @throws {FcdevError} 不支持的字符
 */
export function keyCodeOf(char) {
  if (/^[a-z]$/i.test(char)) return `Key${char.toUpperCase()}`;
  if (/^[0-9]$/.test(char)) return `Digit${char}`;
  throw new FcdevError(`无法在 SB-DOS 里输入字符“${char}”`, { code: 'DOS_KEY_UNSUPPORTED', hint: '程序名只能用字母与数字（用 fcdev.json 的 name 改名）' });
}

/**
 * 运行指定的帧数，运行结束才返回。
 * @param {import('ember-debug-sdk').EmberDebugClient} client 已连接的调试客户端
 * @param {number} frames 帧数
 * @returns {Promise<{stopped: string, frames: number, cycles: number}>} 运行结果，stopped 说明停止原因
 */
export function runFrames(client, frames) {
  return client.request('runtime.run', { frames, wait: true }, { timeoutMs: RUN_TIMEOUT_MS });
}

/**
 * 按下并松开一个键，期间运行若干帧。
 * @param {import('ember-debug-sdk').EmberDebugClient} client 已连接的调试客户端
 * @param {string} code 物理键位（KeyboardEvent.code），如 KeyA、Enter、F1
 * @returns {Promise<void>}
 */
export async function tapKey(client, code) {
  await client.request('input.key', { code, down: true });
  await runFrames(client, KEY_DOWN_FRAMES);
  await client.request('input.key', { code, down: false });
  await runFrames(client, KEY_UP_FRAMES);
}

/**
 * 添加固件、放入磁盘镜像，并让整机机型开机：开机后已上电，没有运行。
 * @param {import('ember-debug-sdk').EmberDebugClient} client 已连接的调试客户端
 * @param {{firmware: string, diskImage: string}} input 固件（BIOS）文件与磁盘镜像
 * @returns {Promise<void>}
 * @throws {FcdevError} 文件不存在，或 Ember 拒绝固件、镜像
 */
export async function bootSb2000(client, { firmware, diskImage }) {
  if (!existsSync(firmware)) throw new FcdevError(`找不到固件：${firmware}`, { code: 'FIRMWARE_MISSING', hint: '在 fcdev.json 的 firmware 里指向学习机的 BIOS 文件' });
  if (!existsSync(diskImage)) throw new FcdevError(`找不到磁盘镜像：${diskImage}`, { code: 'EMULATOR_FILE_MISSING' });
  const steps = [
    ['添加固件', 'machines.setBios', { machine: SB2000_MACHINE, path: firmware }],
    ['开机', 'machines.boot', { machine: SB2000_MACHINE, powerOn: true, resume: false }],
    ['放入磁盘镜像', 'media.floppyInsert', { drive: 0, path: diskImage }],
  ];
  for (const [name, method, params] of steps) {
    try {
      await client.request(method, params);
    } catch (error) {
      throw new FcdevError(`Ember ${name}失败：${error.message}`, { code: 'EMULATOR_LOAD_FAILED', cause: error });
    }
  }
}

/**
 * 在 SB-DOS 里运行程序：等 DOS 起来，回车跳过日期与时间，输入程序名回车。返回时程序刚被启动，机器仍是暂停的。
 * @param {import('ember-debug-sdk').EmberDebugClient} client 已连接的调试客户端
 * @param {string} programName 程序名（不含扩展名）
 * @returns {Promise<void>}
 * @throws {FcdevError} 程序名里有无法输入的字符
 */
export async function enterProgram(client, programName) {
  const codes = [...programName].map(keyCodeOf);
  await runFrames(client, DOS_PROMPT_FRAMES);
  await tapKey(client, 'Enter');
  await runFrames(client, 60);
  await tapKey(client, 'Enter');
  await runFrames(client, 120);
  for (const code of codes) await tapKey(client, code);
  await tapKey(client, 'Enter');
}
