// ------------------------------------------------------------------------
// 名称：sb2000-session.ts
// 说明：学习机（SB2000）在 Ember 里的启动与操作：经调试服务添加固件、放入磁盘镜像、整机开机，并在
//       SB-DOS里输入程序名运行；还有按键与运行帧数的小工具。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-15
// 备注：固件是学习机的 BIOS（iNES 文件），Ember 把它复制进自己的 ROM 目录；磁盘镜像由磁盘镜像命令生
//       成，里面只有程序，开机后进入 SB-DOS 命令行，要输入程序名才运行。帧数按 Ember 的运行速度估
//       计：runtime.run 不限速，几千帧只要几秒；SB-DOS 启动后先问日期与时间，回车跳过。
// ------------------------------------------------------------------------

import type { DebugSession } from '../../contracts/debug-client/debug-session';
import type { FileSystem } from '../../contracts/files/file-system';
import { FcdevError } from '../../shared/errors/fcdev-error';

/** 整机机型的标识。 */
export const SB2000_MACHINE = 'sb2000';

/** 开机到 SB-DOS 开始读键盘所需的帧数；日期提示约 850 帧才读键盘，更早按的回车会丢。 */
export const DOS_PROMPT_FRAMES = 900;

/** 按键按下与松开之后各运行的帧数，让 BIOS 读到。 */
const KEY_DOWN_FRAMES = 5;
const KEY_UP_FRAMES = 4;

/** 在 SB-DOS 命令行里输入时，按下与松开之后各运行的帧数，更短会丢键。 */
const DOS_KEY_FRAMES = 8;

/** 一次运行请求等待的上限，单位毫秒。 */
const RUN_TIMEOUT_MS = 120_000;

/**
 * 字符对应的物理键位（KeyboardEvent.code）。
 * @param char 程序名里的一个字符，只支持字母与数字。
 * @returns 键位。
 * @throws FcdevError 不支持的字符。
 */
export function keyCodeOf(char: string): string {
  if (/^[a-z]$/i.test(char)) return `Key${char.toUpperCase()}`;
  if (/^\d$/.test(char)) return `Digit${char}`;
  throw new FcdevError(`无法在 SB-DOS 里输入字符“${char}”`, {
    code: 'DOS_KEY_UNSUPPORTED',
    hint: '程序名只能用字母与数字（用 fcdev.json 的 name 改名）'
  });
}

/**
 * 运行指定的帧数，运行结束才返回。
 * @param client 已连接的调试会话。
 * @param frames 帧数。
 * @returns 运行结果，stopped 说明停止原因。
 */
export function runFrames(client: DebugSession, frames: number) {
  return client.request('runtime.run', { frames, wait: true }, { timeoutMs: RUN_TIMEOUT_MS });
}

/**
 * 按下并松开一个键，期间运行若干帧。
 * @param client 已连接的调试会话。
 * @param code 物理键位（KeyboardEvent.code），如 KeyA、Enter、F1。
 * @param downFrames 按下后运行的帧数。
 * @param upFrames 松开后运行的帧数。
 */
export async function tapKey(client: DebugSession, code: string, downFrames = KEY_DOWN_FRAMES, upFrames = KEY_UP_FRAMES): Promise<void> {
  await client.request('input.key', { code, down: true });
  await runFrames(client, downFrames);
  await client.request('input.key', { code, down: false });
  await runFrames(client, upFrames);
}

/** 整机开机的输入。 */
export interface BootMachineInput {
  /** 机型，缺省学习机。 */
  readonly machine?: string | undefined;
  /** 固件（BIOS）文件。 */
  readonly firmware?: string | undefined;
  /** 磁盘镜像。 */
  readonly diskImage?: string | undefined;
}

/**
 * 添加固件、放入磁盘镜像，并让整机机型开机：开机后已上电，没有运行。
 * @param files 文件系统，用来确认文件存在。
 * @param client 已连接的调试会话。
 * @param input 机型、固件与磁盘镜像；固件与镜像都可以不给。
 * @throws FcdevError 文件不存在，或 Ember 拒绝固件、镜像。
 */
export async function bootMachine(files: FileSystem, client: DebugSession, { machine = SB2000_MACHINE, firmware, diskImage }: BootMachineInput): Promise<void> {
  if (firmware !== undefined && files.kindOf(firmware) !== 'file') {
    throw new FcdevError(`找不到固件：${firmware}`, { code: 'FIRMWARE_MISSING', hint: '在 fcdev.json 的 firmware 里指向学习机的 BIOS 文件' });
  }
  if (diskImage !== undefined && files.kindOf(diskImage) !== 'file') {
    throw new FcdevError(`找不到磁盘镜像：${diskImage}`, { code: 'EMULATOR_FILE_MISSING' });
  }
  const steps: Array<[string, () => Promise<unknown>]> = [];
  if (firmware !== undefined) steps.push(['添加固件', () => client.request('machines.setRom', { machine, path: firmware })]);
  steps.push(['开机', () => client.request('machines.boot', { machine, powerOn: true, resume: false })]);
  if (diskImage !== undefined) steps.push(['放入磁盘镜像', () => client.request('media.floppyInsert', { drive: 0, path: diskImage })]);
  for (const [name, action] of steps) {
    try {
      await action();
    } catch (error) {
      throw new FcdevError(`Ember ${name}失败：${error instanceof Error ? error.message : String(error)}`, { code: 'EMULATOR_LOAD_FAILED', cause: error });
    }
  }
}

/**
 * 学习机：添加固件、放入磁盘镜像并开机。
 * @param files 文件系统。
 * @param client 已连接的调试会话。
 * @param input 固件（BIOS）文件与磁盘镜像。
 * @throws FcdevError 文件不存在，或 Ember 拒绝固件、镜像。
 */
export function bootSb2000(files: FileSystem, client: DebugSession, input: { firmware: string; diskImage: string }): Promise<void> {
  return bootMachine(files, client, { machine: SB2000_MACHINE, ...input });
}

/**
 * 在 SB-DOS 里运行程序：开机运行 900 帧，直接按两次回车跳过日期与时间，输入程序名回车。返回时程序刚
 * 被
 * 启动，机器仍是暂停的。
 * @param client 已连接的调试会话。
 * @param programName 程序名（不含扩展名）。
 * @throws FcdevError 程序名里有无法输入的字符。
 */
export async function enterProgram(client: DebugSession, programName: string): Promise<void> {
  const codes = [...programName].map(keyCodeOf);
  await runFrames(client, DOS_PROMPT_FRAMES);
  await tapKey(client, 'Enter', DOS_KEY_FRAMES, DOS_KEY_FRAMES);
  await tapKey(client, 'Enter', DOS_KEY_FRAMES, DOS_KEY_FRAMES);
  for (const code of codes) await tapKey(client, code, DOS_KEY_FRAMES, DOS_KEY_FRAMES);
  await tapKey(client, 'Enter', DOS_KEY_FRAMES, DOS_KEY_FRAMES);
}
