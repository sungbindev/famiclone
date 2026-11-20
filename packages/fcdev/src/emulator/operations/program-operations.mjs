// ------------------------------------------------------------------------
// 名称：program-operations.mjs
// 说明：程序的载入与运行：load（装入卡带或整机固件与磁盘并执行启动步骤）、run（运行指定帧数）、breakpoint（断点）、step（单步）。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-21
// 备注：运行与单步返回时机器总是暂停的，并带上 CPU 的状态；运行被断点打断时 stopped 为 breakpoint。
// ------------------------------------------------------------------------
import { existsSync } from 'node:fs';
import path from 'node:path';
import { FcdevError } from '../../shared/index.mjs';
import { bootMachine, enterProgram } from '../sb2000-session.mjs';
import { invalidParam, optionalAddress, optionalBoolean, optionalEnum, optionalInteger, optionalString } from './params.mjs';
import { currentSession } from './session.mjs';

/** 一次运行请求等待的上限，单位毫秒 */
const RUN_TIMEOUT_MS = 600_000;

/**
 * 把 CPU 状态整理成对象：每个状态项一个字段，另有十六进制的 pc。
 * @param {import('./session.mjs').Session['client']} client 调试客户端
 * @returns {Promise<Record<string, number|string>>} 状态，如 { pc: '$C000', a: 0, x: 0, ... }
 */
export async function cpuState(client) {
  const { states } = await client.request('inspector.getState', { module: 'cpu' });
  const fields = Object.fromEntries((states[0]?.fields ?? []).map((field) => [field.name, field.value]));
  const pc = fields.pc;
  return typeof pc === 'number' ? { ...fields, pc: `$${pc.toString(16).toUpperCase().padStart(4, '0')}` } : fields;
}

/**
 * 装入程序并执行启动步骤：卡带用 file，整机用 machine（加固件、磁盘与要在 DOS 里运行的程序名）。
 * @param {Record<string, unknown>} params file，或 machine 加可选的 firmware、diskImage、program；可选 familyKeyboard、powerOn
 * @returns {Promise<object>} 运行状态
 * @throws {FcdevError} 参数组合不对、文件不存在，或 Ember 拒绝
 */
export async function load(params) {
  const { client } = currentSession();
  const file = optionalString(params, 'file');
  const machine = optionalString(params, 'machine');
  if ((file === undefined) === (machine === undefined)) invalidParam('要给 file（卡带文件）或 machine（整机机型）二者之一', '整机还可以给 firmware、diskImage、program');
  if (params.familyKeyboard !== undefined) await client.request('input.setFamilyKeyboard', { connected: optionalBoolean(params, 'familyKeyboard', false) });

  if (file !== undefined) {
    const absolute = path.resolve(file);
    if (!existsSync(absolute)) throw new FcdevError(`找不到要载入的文件：${absolute}`, { code: 'EMULATOR_FILE_MISSING' });
    await client.request('cartridges.load', { path: absolute, powerOn: optionalBoolean(params, 'powerOn', true), resume: false });
  } else {
    const firmware = optionalString(params, 'firmware');
    const diskImage = optionalString(params, 'diskImage');
    await bootMachine(client, { machine, firmware: firmware === undefined ? undefined : path.resolve(firmware), diskImage: diskImage === undefined ? undefined : path.resolve(diskImage) });
    const program = optionalString(params, 'program');
    if (program !== undefined) await enterProgram(client, program);
  }
  return client.request('runtime.status', {});
}

/**
 * 运行指定的帧数（不限速），结束或命中断点后暂停。
 * @param {Record<string, unknown>} params 可选 frames（默认 60）
 * @returns {Promise<object>} 停止原因、实际帧数与周期数、CPU 状态
 */
export async function run(params) {
  const { client } = currentSession();
  const frames = optionalInteger(params, 'frames', { min: 1, max: 10_000_000 }) ?? 60;
  const result = await client.request('runtime.run', { frames, wait: true }, { timeoutMs: RUN_TIMEOUT_MS });
  return { stopped: result.stopped, frames: result.frames, cycles: result.cycles, state: result.state, cpu: await cpuState(client) };
}

/**
 * 断点：add 添加、list 列出、remove 删除一个、clear 全部删除。
 * @param {Record<string, unknown>} params action；add 要 kind（execute、read、write）与 address，可选 endAddress；remove 要 id
 * @returns {Promise<object>} 添加的断点，或当前的全部断点
 */
export async function breakpoint(params) {
  const { client } = currentSession();
  const action = optionalEnum(params, 'action', ['add', 'list', 'remove', 'clear'], 'list');
  if (action === 'add') {
    const kind = optionalEnum(params, 'kind', ['execute', 'read', 'write']);
    if (kind === undefined) invalidParam('添加断点要给 kind：execute、read、write');
    const address = optionalAddress(params, 'address');
    if (address === undefined) invalidParam('添加断点要给 address');
    const endAddress = optionalAddress(params, 'endAddress');
    await client.request('inspector.addBreakpoint', { kind, address, ...(endAddress === undefined ? {} : { endAddress }) });
  } else if (action === 'remove') {
    const id = optionalInteger(params, 'id', { min: 1 });
    if (id === undefined) invalidParam('删除断点要给 id');
    await client.request('inspector.removeBreakpoint', { id });
  } else if (action === 'clear') {
    await client.request('inspector.clearBreakpoints', {});
  }
  return client.request('inspector.listBreakpoints', {});
}

/**
 * 单步：先暂停，再按单位运行指定的数量。
 * @param {Record<string, unknown>} params 可选 unit（instruction、cycle、scanline、frame，默认 instruction）、count（默认 1）
 * @returns {Promise<object>} 实际执行的数量、停下时的 pc 与 CPU 状态
 */
export async function step(params) {
  const { client } = currentSession();
  const unit = optionalEnum(params, 'unit', ['instruction', 'cycle', 'scanline', 'frame'], 'instruction');
  const count = optionalInteger(params, 'count', { min: 1, max: 100_000 }) ?? 1;
  const result = await client.request('runtime.step', { unit, count });
  return { ...result, pc: `$${result.pc.toString(16).toUpperCase().padStart(4, '0')}`, cpu: await cpuState(client) };
}
