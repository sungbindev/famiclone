// ------------------------------------------------------------------------
// 名称：program-operations.ts
// 说明：程序的载入与运行：load（装入卡带或整机固件与磁盘并执行启动步骤）、run（运行指定帧数）、
//       breakpoint（断点）、step（单步）。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-21
// 备注：运行与单步返回时机器总是暂停的，并带上 CPU 的状态；运行被断点打断时 stopped 为 breakpoint。
// ------------------------------------------------------------------------

import * as path from 'node:path';
import type { DebugSession } from '../../contracts/debug-client/debug-session';
import { FcdevError } from '../../shared/errors/fcdev-error';
import { selectProfileInput } from '../../shared/targets/target-profile';
import { runBootSteps } from './boot-steps';
import type { OperationContext } from './operation-context';
import { invalidParam, optionalAddress, optionalBoolean, optionalEnum, optionalInteger, optionalString } from './params';
import { bootMachine, enterProgram } from './machine-session';

/** 一次运行请求等待的上限，单位毫秒。 */
const RUN_TIMEOUT_MS = 600_000;

/** 把数写成四位十六进制，前面加 $。 */
function hexAddress(value: number): string {
  return `$${value.toString(16).toUpperCase().padStart(4, '0')}`;
}

/**
 * 把 CPU 状态整理成对象：每个状态项一个字段，另有十六进制的 pc。
 * @param client 调试会话。
 * @returns 状态，如 { pc: '$C000', a: 0, x: 0, ... }。
 */
export async function cpuState(client: DebugSession): Promise<Record<string, number | string>> {
  const { states } = await client.request('inspector.getState', { module: 'cpu' });
  const fields: Record<string, number | string> = Object.fromEntries((states[0]?.fields ?? []).map((field) => [field.name, field.value]));
  const pc = fields.pc;
  return typeof pc === 'number' ? { ...fields, pc: hexAddress(pc) } : fields;
}

/**
 * 装入程序并执行启动步骤：给 target（内置目标描述的名字或 profile.json 的路径）时按它的输入里写的步
 * 骤
 * 执行；不给时，卡带用 file，整机用 machine（加固件、磁盘与要在 DOS 里运行的程序名）。
 * @param context 上下文。
 * @param params target 与可选的 input，或 file，或 machine；可选 firmware、diskImage、program、
 *   familyKeyboard、powerOn。
 * @returns 运行状态。
 * @throws FcdevError 参数组合不对、文件不存在，或 Ember 拒绝。
 */
export async function load(context: OperationContext, params: Record<string, unknown>): Promise<Record<string, unknown>> {
  const { env, store } = context;
  const { client } = store.currentSession();
  const target = optionalString(params, 'target');
  if (optionalBoolean(params, 'preview', false)) {
    const previewFile = optionalString(params, 'file');
    if (previewFile === undefined || target !== undefined || params.machine !== undefined) invalidParam('preview 只能与 file 一起用，不要再给 target、machine');
    const absolute = path.resolve(previewFile);
    if (env.files.kindOf(absolute) !== 'file') throw new FcdevError(`找不到要预览的文件：${absolute}`, { code: 'EMULATOR_FILE_MISSING' });
    return { ...(await client.request('cartridges.parseHeader', { path: absolute })) };
  }
  if (target !== undefined) {
    if (params.machine !== undefined) invalidParam('给了 target 就不要再给 machine，机型由目标描述的启动步骤决定');
    const { profile } = env.profiles.read(target, '');
    const chosen = selectProfileInput(profile, optionalString(params, 'input'));
    if (params.familyKeyboard !== undefined) {
      await client.request('input.setFamilyKeyboard', { connected: optionalBoolean(params, 'familyKeyboard', false) });
    }
    await runBootSteps(env.files, client, chosen.boot, params);
    return { target: profile.name, input: chosen.name, ...(await client.request('runtime.status', {})) };
  }
  if (params.input !== undefined) invalidParam('给了 input 就要同时给 target');
  const file = optionalString(params, 'file');
  const machine = optionalString(params, 'machine');
  if ((file === undefined) === (machine === undefined)) {
    invalidParam('要给 file（卡带文件）或 machine（整机机型）二者之一', '整机还可以给 firmware、diskImage、program');
  }
  if (params.familyKeyboard !== undefined) {
    await client.request('input.setFamilyKeyboard', { connected: optionalBoolean(params, 'familyKeyboard', false) });
  }

  if (file !== undefined) {
    const absolute = path.resolve(file);
    if (env.files.kindOf(absolute) !== 'file') throw new FcdevError(`找不到要载入的文件：${absolute}`, { code: 'EMULATOR_FILE_MISSING' });
    await client.request('cartridges.load', { path: absolute, powerOn: optionalBoolean(params, 'powerOn', true), resume: false });
  } else if (machine !== undefined) {
    const firmware = optionalString(params, 'firmware');
    const diskImage = optionalString(params, 'diskImage');
    await bootMachine(env.files, client, {
      machine,
      firmware: firmware === undefined ? undefined : path.resolve(firmware),
      diskImage: diskImage === undefined ? undefined : path.resolve(diskImage)
    });
    const program = optionalString(params, 'program');
    if (program !== undefined) await enterProgram(client, program, machine);
  }
  return { ...(await client.request('runtime.status', {})) };
}

/**
 * 按授权量运行（不限速），结束或命中断点后暂停。
 * @param context 上下文。
 * @param params 可选 frames、cycles、instructions、scanlines 之一（都不给时 frames 为 60）。
 * @returns 停止原因、实际帧数与周期数（按周期、指令、扫描线授权时还有实际指令条数）、CPU 状态。
 */
export async function run(context: OperationContext, params: Record<string, unknown>): Promise<Record<string, unknown>> {
  const { client } = context.store.currentSession();
  const frames = optionalInteger(params, 'frames', { min: 1, max: 10_000_000 });
  const cycles = optionalInteger(params, 'cycles', { min: 1, max: 100_000_000_000 });
  const instructions = optionalInteger(params, 'instructions', { min: 1, max: 100_000_000_000 });
  const scanlines = optionalInteger(params, 'scanlines', { min: 1, max: 1_000_000_000 });
  if ([frames, cycles, instructions, scanlines].filter((value) => value !== undefined).length > 1) invalidParam('frames、cycles、instructions、scanlines 只能给一个');
  const quota = cycles !== undefined ? { cycles } : instructions !== undefined ? { instructions } : scanlines !== undefined ? { scanlines } : { frames: frames ?? 60 };
  const result = await client.request('runtime.run', { ...quota, wait: true }, { timeoutMs: RUN_TIMEOUT_MS });
  return {
    stopped: result.stopped,
    frames: result.frames,
    cycles: result.cycles,
    ...(result.instructions === undefined ? {} : { instructions: result.instructions }),
    state: result.state,
    cpu: await cpuState(client)
  };
}

/** 带地址的断点种类。 */
const ADDRESS_BREAKPOINT_KINDS = ['execute', 'read', 'write'] as const;

/** 事件类断点种类：CPU 响应中断或复位之后停下，没有地址。 */
const EVENT_BREAKPOINT_KINDS = ['irq', 'nmi', 'brk', 'reset'] as const;

/**
 * 断点：add 添加、list 列出、remove 删除一个、clear 全部删除、enable 与 disable 启用或禁用一个、
 * update 改条件。
 * @param context 上下文。
 * @param params action；add 要 kind（execute、read、write、irq、nmi、brk、reset），前三种要
 *   address，可选 endAddress；add 与 update 可带 condition；remove、enable、disable、update 要 id。
 * @returns 当前的全部断点。
 */
export async function breakpoint(context: OperationContext, params: Record<string, unknown>): Promise<Record<string, unknown>> {
  const { client } = context.store.currentSession();
  const action = optionalEnum(params, 'action', ['add', 'list', 'remove', 'clear', 'enable', 'disable', 'update'], 'list');
  if (action === 'add') {
    const kind = optionalEnum(params, 'kind', [...ADDRESS_BREAKPOINT_KINDS, ...EVENT_BREAKPOINT_KINDS]);
    if (kind === undefined) invalidParam('添加断点要给 kind：execute、read、write、irq、nmi、brk、reset');
    const condition = optionalString(params, 'condition');
    const conditionPart = condition === undefined ? {} : { condition };
    if ((EVENT_BREAKPOINT_KINDS as readonly string[]).includes(kind)) {
      await client.request('inspector.addBreakpoint', { kind: kind as (typeof EVENT_BREAKPOINT_KINDS)[number], ...conditionPart });
    } else {
      const address = optionalAddress(params, 'address');
      if (address === undefined) invalidParam(`${kind} 断点要给 address`);
      const endAddress = optionalAddress(params, 'endAddress');
      await client.request('inspector.addBreakpoint', {
        kind: kind as (typeof ADDRESS_BREAKPOINT_KINDS)[number],
        address,
        ...(endAddress === undefined ? {} : { endAddress }),
        ...conditionPart
      });
    }
  } else if (action === 'remove') {
    const id = optionalInteger(params, 'id', { min: 1 });
    if (id === undefined) invalidParam('删除断点要给 id');
    await client.request('inspector.removeBreakpoint', { id });
  } else if (action === 'clear') {
    await client.request('inspector.clearBreakpoints', {});
  } else if (action === 'enable' || action === 'disable' || action === 'update') {
    const id = optionalInteger(params, 'id', { min: 1 });
    if (id === undefined) invalidParam(`${action} 要给 id`);
    const condition = optionalString(params, 'condition');
    if (action === 'update' && condition === undefined) invalidParam('update 要给 condition（空字符串表示去掉条件）');
    await client.request('inspector.updateBreakpoint', {
      id,
      ...(action === 'update' ? {} : { enabled: action === 'enable' }),
      ...(condition === undefined ? {} : { condition })
    });
  }
  return { ...(await client.request('inspector.listBreakpoints', {})) };
}

/**
 * 设置自由运行的速度与声音方式。
 * @param context 上下文。
 * @param params percent 或 max（二选一）、audio（real、fast），至少给一项。
 * @returns 设置之后的 speed、maxSpeed（设置了速度时）与 mode（设置了声音方式时）。
 */
export async function speed(context: OperationContext, params: Record<string, unknown>): Promise<Record<string, unknown>> {
  const { client } = context.store.currentSession();
  const percent = optionalInteger(params, 'percent', { min: 10, max: 1000 });
  const max = optionalBoolean(params, 'max', false);
  const audio = optionalEnum(params, 'audio', ['real', 'fast']) as 'real' | 'fast' | undefined;
  if (percent === undefined && !max && audio === undefined) invalidParam('要给 percent、max、audio 之一');
  if (percent !== undefined && max) invalidParam('percent 与 max 不能同时给');
  const result: Record<string, unknown> = {};
  if (percent !== undefined || max) Object.assign(result, await client.request('runtime.setSpeed', max ? { max } : { percent }));
  if (audio !== undefined) Object.assign(result, await client.request('output.setAudioMode', { mode: audio }));
  return result;
}

/**
 * 单步：先暂停，再按单位运行指定的数量。
 * @param context 上下文。
 * @param params 可选 unit（instruction、cycle、scanline、frame，默认 instruction）、count（默认
 *   1）。
 * @returns 实际执行的数量、停下时的 pc 与 CPU 状态。
 */
export async function step(context: OperationContext, params: Record<string, unknown>): Promise<Record<string, unknown>> {
  const { client } = context.store.currentSession();
  const unit = optionalEnum(params, 'unit', ['instruction', 'cycle', 'scanline', 'frame'], 'instruction') as 'instruction' | 'cycle' | 'scanline' | 'frame';
  const count = optionalInteger(params, 'count', { min: 1, max: 100_000 }) ?? 1;
  const result = await client.request('runtime.step', { unit, count });
  return { ...result, pc: hexAddress(result.pc), cpu: await cpuState(client) };
}
