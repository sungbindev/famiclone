// ------------------------------------------------------------------------
// 名称：boot-steps.mjs
// 说明：执行目标描述里的启动步骤：接入 Family BASIC 键盘、载入卡带、整机开机、运行若干帧、按键、输入文字。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-21
// 备注：步骤的字段由 shared/target-profile.mjs 检查过；文字里的 {名字} 用调用者给的同名参数代入（如 {program} 是程序名）；
//       步骤做完机器是暂停的，返回时程序刚被启动。
// ------------------------------------------------------------------------
import { existsSync } from 'node:fs';
import path from 'node:path';
import { FcdevError } from '../../shared/index.mjs';
import { bootMachine, keyCodeOf, runFrames, tapKey } from '../sb2000-session.mjs';

/**
 * 把文字里的 {名字} 换成参数。
 * @param {string} text 带占位符的文字
 * @param {Record<string, unknown>} params 参数
 * @returns {string} 换好的文字
 * @throws {FcdevError} 占位符对应的参数没有给
 */
function substitute(text, params) {
  return text.replaceAll(/\{(\w+)\}/g, (_, name) => {
    const value = params[name];
    if (typeof value !== 'string' || value === '') {
      throw new FcdevError(`这个目标的启动步骤要用参数 ${name}`, { code: 'EMULATOR_PARAM_INVALID', hint: `给 emu_load 加上 ${name}` });
    }
    return value;
  });
}

/**
 * 取要载入的文件的绝对路径。
 * @param {unknown} value 参数值
 * @param {string} name 参数名
 * @param {string} step 步骤名，用在错误里
 * @returns {string} 绝对路径
 * @throws {FcdevError} 没给，或文件不存在
 */
function requireFile(value, name, step) {
  if (typeof value !== 'string' || value === '') throw new FcdevError(`启动步骤 ${step} 要用参数 ${name}`, { code: 'EMULATOR_PARAM_INVALID', hint: `给 emu_load 加上 ${name}` });
  const absolute = path.resolve(value);
  if (!existsSync(absolute)) throw new FcdevError(`找不到要载入的文件：${absolute}`, { code: 'EMULATOR_FILE_MISSING' });
  return absolute;
}

/**
 * 依次执行启动步骤。
 * @param {{request: Function}} client 已连接的调试客户端
 * @param {ReadonlyArray<Readonly<Record<string, unknown>>>} steps 步骤
 * @param {Record<string, unknown>} params 调用者给的参数：file（卡带）、firmware、diskImage、program 等
 * @returns {Promise<void>}
 * @throws {FcdevError} 缺少步骤要用的参数、文件不存在，或 Ember 拒绝
 */
export async function runBootSteps(client, steps, params) {
  for (const step of steps) {
    switch (step.step) {
      case 'familyKeyboard':
        await client.request('input.setFamilyKeyboard', { connected: step.connected });
        break;
      case 'cartridge':
        await client.request('cartridges.load', { path: requireFile(params.file, 'file', 'cartridge'), powerOn: true, resume: false });
        break;
      case 'machine': {
        const firmware = params.firmware === undefined ? undefined : requireFile(params.firmware, 'firmware', 'machine');
        const diskImage = params.diskImage === undefined ? undefined : requireFile(params.diskImage, 'diskImage', 'machine');
        await bootMachine(client, { machine: step.machine, firmware, diskImage });
        break;
      }
      case 'run':
        await runFrames(client, step.frames);
        break;
      case 'key':
        await tapKey(client, step.code, step.downFrames, step.upFrames);
        break;
      default:
        for (const character of substitute(step.text, params)) await tapKey(client, keyCodeOf(character), step.downFrames, step.upFrames);
    }
  }
}
