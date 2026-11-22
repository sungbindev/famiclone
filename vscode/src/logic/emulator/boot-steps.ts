// ------------------------------------------------------------------------
// 名称：boot-steps.ts
// 说明：执行目标描述里的启动步骤：接入 Family BASIC 键盘、载入卡带、整机开机、运行若干帧、按键、输
//       入文字。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-21
// 备注：步骤的字段由 shared/targets/target-profile.ts 检查过；文字里的 {名字} 用调用者给的同名参数
//       代入（如 {program} 是程序名）；步骤做完机器是暂停的，返回时程序刚被启动。
// ------------------------------------------------------------------------

import * as path from 'node:path';
import type { DebugSession } from '../../contracts/debug-client/debug-session';
import type { FileSystem } from '../../contracts/files/file-system';
import { FcdevError } from '../../shared/errors/fcdev-error';
import type { BootStep } from '../../shared/targets/target-profile';
import { bootMachine, keyCodeOf, runFrames, tapKey } from './sb2000-session';

/**
 * 把文字里的 {名字} 换成参数。
 * @param text 带占位符的文字。
 * @param params 参数。
 * @returns 换好的文字。
 * @throws FcdevError 占位符对应的参数没有给。
 */
function substitute(text: string, params: Record<string, unknown>): string {
  return text.replaceAll(/\{(\w+)\}/g, (_, name: string) => {
    const value = params[name];
    if (typeof value !== 'string' || value === '') {
      throw new FcdevError(`这个目标的启动步骤要用参数 ${name}`, { code: 'EMULATOR_PARAM_INVALID', hint: `给 emu_load 加上 ${name}` });
    }
    return value;
  });
}

/**
 * 取要载入的文件的绝对路径。
 * @param files 文件系统。
 * @param value 参数值。
 * @param name 参数名。
 * @param step 步骤名，用在错误里。
 * @returns 绝对路径。
 * @throws FcdevError 没给，或文件不存在。
 */
function requireFile(files: FileSystem, value: unknown, name: string, step: string): string {
  if (typeof value !== 'string' || value === '') {
    throw new FcdevError(`启动步骤 ${step} 要用参数 ${name}`, { code: 'EMULATOR_PARAM_INVALID', hint: `给 emu_load 加上 ${name}` });
  }
  const absolute = path.resolve(value);
  if (files.kindOf(absolute) !== 'file') throw new FcdevError(`找不到要载入的文件：${absolute}`, { code: 'EMULATOR_FILE_MISSING' });
  return absolute;
}

/**
 * 取可选的步骤字段：数字或 undefined。
 * @param step 步骤。
 * @param name 字段名。
 * @returns 值。
 */
function frameField(step: BootStep, name: string): number | undefined {
  const value = step[name];
  return typeof value === 'number' ? value : undefined;
}

/**
 * 依次执行启动步骤。
 * @param files 文件系统。
 * @param client 已连接的调试会话。
 * @param steps 步骤。
 * @param params 调用者给的参数：file（卡带）、firmware、diskImage、program 等。
 * @throws FcdevError 缺少步骤要用的参数、文件不存在，或 Ember 拒绝。
 */
export async function runBootSteps(files: FileSystem, client: DebugSession, steps: readonly BootStep[], params: Record<string, unknown>): Promise<void> {
  for (const step of steps) {
    switch (step.step) {
      case 'familyKeyboard':
        await client.request('input.setFamilyKeyboard', { connected: step.connected === true });
        break;
      case 'cartridge':
        await client.request('cartridges.load', { path: requireFile(files, params.file, 'file', 'cartridge'), powerOn: true, resume: false });
        break;
      case 'machine': {
        const firmware = params.firmware === undefined ? undefined : requireFile(files, params.firmware, 'firmware', 'machine');
        const diskImage = params.diskImage === undefined ? undefined : requireFile(files, params.diskImage, 'diskImage', 'machine');
        await bootMachine(files, client, { machine: typeof step.machine === 'string' ? step.machine : undefined, firmware, diskImage });
        break;
      }
      case 'run':
        await runFrames(client, Number(step.frames));
        break;
      case 'key':
        await tapKey(client, String(step.code), frameField(step, 'downFrames'), frameField(step, 'upFrames'));
        break;
      default:
        for (const character of substitute(String(step.text), params)) {
          await tapKey(client, keyCodeOf(character), frameField(step, 'downFrames'), frameField(step, 'upFrames'));
        }
    }
  }
}
