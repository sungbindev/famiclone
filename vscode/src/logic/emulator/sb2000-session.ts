// ------------------------------------------------------------------------
// 名称：sb2000-session.ts
// 说明：整机（学习机 SB2000、小霸王 97、裕兴、步步高）在 Ember 里的启动与操作：经调试服务添加固件、
//       放入磁盘镜像、整机开机，并在 DOS 里输入程序名运行；还有按键与运行帧数的小工具。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-23
// 备注：固件是机器的 BIOS（ROM）文件，Ember 把它复制进自己的 ROM 目录；磁盘镜像由构建生成，
//       里面有程序和项目 disk 目录下的文件，开机后进入 DOS 命令行，要输入程序名才运行。帧数按
//       Ember 的运行速度估计：runtime.run 不限速，几千帧只要几秒；SB-DOS 启动后先问日期与时
//       间，回车跳过；SB97 开机后要先选中菜单里的 DOS；裕兴开机后要经主菜单与“学DOS”的
//       两个画面才进 DOS，提示符下运行程序必须带扩展名；步步高的 DOS 在磁盘上，开机后由 BIOS
//       读入，系统盘有自动批处理时要等它跑完，没有时先问日期与时间（回车跳过），运行程序不用写
//       扩展名。
// ------------------------------------------------------------------------

import type { DebugSession } from '../../contracts/debug-client/debug-session';
import type { FileSystem } from '../../contracts/files/file-system';
import { FcdevError } from '../../shared/errors/fcdev-error';

/** 整机机型的标识。 */
export const SB2000_MACHINE = 'sb2000';

/** 小霸王 97 的机型标识。 */
export const SB97_MACHINE = 'sb97';

/** 开机到 SB-DOS 开始读键盘所需的帧数；日期提示约 850 帧才读键盘，更早按的回车会丢。 */
export const DOS_PROMPT_FRAMES = 900;

/** SB97 开机到显示“按任意键”的画面所需的帧数。 */
const SB97_LOGO_FRAMES = 200;

/** SB97 按键后检测硬件并显示菜单所需的帧数；菜单没稳定前按的左键会丢。 */
const SB97_MENU_FRAMES = 300;

/** SB97 进入 DOS 后等到提示符出现的帧数。 */
const SB97_DOS_FRAMES = 120;

/** 裕兴的机型标识。 */
export const YUXING_MACHINE = 'yuxing';

/** 裕兴开机到显示“按任意键”的画面所需的帧数。 */
const YUXING_LOGO_FRAMES = 300;

/** 裕兴按键后显示主菜单所需的帧数。 */
const YUXING_MENU_FRAMES = 120;

/** 裕兴选中主菜单的“学DOS”要按的下键次数。 */
const YUXING_MENU_DOWNS = 3;

/** 裕兴进入“学DOS”后，“DOS入门”动画播完所需的帧数。 */
const YUXING_INTRO_FRAMES = 300;

/** 裕兴按空格后显示 DOS 学习菜单所需的帧数。 */
const YUXING_LESSON_FRAMES = 120;

/** 裕兴选中“三.DOS系统操作”要按的下键次数。 */
const YUXING_LESSON_DOWNS = 2;

/** 裕兴进入 DOS 后等到提示符出现的帧数。 */
const YUXING_DOS_FRAMES = 120;

/** 裕兴的 DOS 运行程序要输入的扩展名（含点）对应的键位。 */
const YUXING_EXTENSION_CODES = ['Period', 'KeyE', 'KeyX', 'KeyE'];

/** 步步高软驱一号的机型标识。 */
export const BBG_MACHINE = 'bbg1';

/** 步步高开机到 BBGDOS 读键盘（有自动批处理的系统盘要等批处理跑完）所需的帧数。 */
const BBG_DOS_FRAMES = 1200;

/** 步步高按两次回车跳过日期与时间后，等提示符出现的帧数。 */
const BBG_PROMPT_FRAMES = 60;

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
 * 在 DOS 里运行程序：学习机开机运行 900 帧，直接按两次回车跳过日期与时间；SB97 开机后
 * 按空格跳过开机画面，等菜单稳定后按左键选中 DOS，回车进入；裕兴开机后按空格进主菜单，选中
 * “学DOS”回车，等动画后按空格，选中“三.DOS系统操作”回车进入；步步高开机运行 1200 帧（系统盘的
 * 批处理跑完），按两次回车跳过可能出现的日期与时间。再输入程序名回车（裕兴的 DOS 必须带
 * 扩展名，这里补上 .EXE）。返回时程序刚被启动，机器仍是暂停的。
 * @param client 已连接的调试会话。
 * @param programName 程序名（不含扩展名）。
 * @param machine 机型，默认学习机；不是 SB97、裕兴与步步高的都按学习机的流程。
 * @throws FcdevError 程序名里有无法输入的字符。
 */
export async function enterProgram(client: DebugSession, programName: string, machine: string = SB2000_MACHINE): Promise<void> {
  const codes = [...programName].map(keyCodeOf);
  if (machine === YUXING_MACHINE) {
    await runFrames(client, YUXING_LOGO_FRAMES);
    await tapKey(client, 'Space', DOS_KEY_FRAMES, DOS_KEY_FRAMES);
    await runFrames(client, YUXING_MENU_FRAMES);
    for (let count = 0; count < YUXING_MENU_DOWNS; count += 1) await tapKey(client, 'ArrowDown', DOS_KEY_FRAMES, DOS_KEY_FRAMES);
    await tapKey(client, 'Enter', DOS_KEY_FRAMES, DOS_KEY_FRAMES);
    await runFrames(client, YUXING_INTRO_FRAMES);
    await tapKey(client, 'Space', DOS_KEY_FRAMES, DOS_KEY_FRAMES);
    await runFrames(client, YUXING_LESSON_FRAMES);
    for (let count = 0; count < YUXING_LESSON_DOWNS; count += 1) await tapKey(client, 'ArrowDown', DOS_KEY_FRAMES, DOS_KEY_FRAMES);
    await tapKey(client, 'Enter', DOS_KEY_FRAMES, DOS_KEY_FRAMES);
    await runFrames(client, YUXING_DOS_FRAMES);
    codes.push(...YUXING_EXTENSION_CODES);
  } else if (machine === SB97_MACHINE) {
    await runFrames(client, SB97_LOGO_FRAMES);
    await tapKey(client, 'Space', DOS_KEY_FRAMES, DOS_KEY_FRAMES);
    await runFrames(client, SB97_MENU_FRAMES);
    await tapKey(client, 'ArrowLeft', DOS_KEY_FRAMES, DOS_KEY_FRAMES);
    await tapKey(client, 'Enter', DOS_KEY_FRAMES, DOS_KEY_FRAMES);
    await runFrames(client, SB97_DOS_FRAMES);
  } else if (machine === BBG_MACHINE) {
    await runFrames(client, BBG_DOS_FRAMES);
    // 没有自动批处理的系统盘会先问日期与时间；已在提示符下时这两次回车只是空行
    await tapKey(client, 'Enter', DOS_KEY_FRAMES, DOS_KEY_FRAMES);
    await tapKey(client, 'Enter', DOS_KEY_FRAMES, DOS_KEY_FRAMES);
    await runFrames(client, BBG_PROMPT_FRAMES);
  } else {
    await runFrames(client, DOS_PROMPT_FRAMES);
    await tapKey(client, 'Enter', DOS_KEY_FRAMES, DOS_KEY_FRAMES);
    await tapKey(client, 'Enter', DOS_KEY_FRAMES, DOS_KEY_FRAMES);
  }
  for (const code of codes) await tapKey(client, code, DOS_KEY_FRAMES, DOS_KEY_FRAMES);
  await tapKey(client, 'Enter', DOS_KEY_FRAMES, DOS_KEY_FRAMES);
}
