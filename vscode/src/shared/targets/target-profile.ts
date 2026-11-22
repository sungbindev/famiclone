// ------------------------------------------------------------------------
// 名称：target-profile.ts
// 说明：目标描述（profile.json）的检查与取用：一台机器的符号集、图形数据端口，以及每种输入（卡带、
//       程序、BIOS）的映像格式与启动步骤。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-22
// 备注：反汇编读格式、符号集与图形数据端口，模拟器读启动步骤；这里只检查已解析的 JSON，读文件由
//       逻辑层的目标描述读取器负责；符号集的路径相对描述文件所在的目录，检查时换成绝对路径。
// ------------------------------------------------------------------------

import * as path from 'node:path';
import { FcdevError } from '../errors/fcdev-error';
import { createParamChecks } from '../params/param-checks';

/** 描述文件名。 */
export const PROFILE_FILE = 'profile.json';

/** 各种启动步骤允许的字段（step 之外）及其检查：后缀 ? 表示可省略。 */
const STEP_FIELDS: Readonly<Record<string, Readonly<Record<string, string>>>> = {
  familyKeyboard: { connected: 'boolean' },
  cartridge: {},
  machine: { machine: 'string' },
  run: { frames: 'frames' },
  key: { code: 'string', downFrames: 'frames?', upFrames: 'frames?' },
  type: { text: 'string', downFrames: 'frames?', upFrames: 'frames?' }
};

/** 全部启动步骤的名字。 */
export const BOOT_STEP_NAMES: readonly string[] = Object.freeze(Object.keys(STEP_FIELDS));

/** 一个启动步骤：step 是步骤名，其余是该步骤的参数。 */
export type BootStep = Readonly<{ step: string } & Record<string, unknown>>;

/** 目标描述里的一种输入。 */
export interface ProfileInput {
  /** 显示名。 */
  readonly title: string;
  /** 映像格式名。 */
  readonly format: string;
  /** 格式的解析选项。 */
  readonly formatOptions: Readonly<Record<string, unknown>>;
  /** 启动步骤。 */
  readonly boot: readonly BootStep[];
}

/** 选出的输入：带输入名。 */
export type SelectedInput = { readonly name: string } & ProfileInput;

/** 目标描述。 */
export interface TargetProfile {
  /** 名字。 */
  readonly name: string;
  /** 显示名。 */
  readonly title: string;
  /** 符号集文件的绝对路径。 */
  readonly symbolSets: readonly string[];
  /** 图形数据端口的 CPU 地址。 */
  readonly graphicsPorts: readonly number[];
  /** 没有指定输入时用的输入名。 */
  readonly defaultInput: string | undefined;
  /** 各种输入。 */
  readonly inputs: Readonly<Record<string, Readonly<ProfileInput>>>;
}

const { toAddress } = createParamChecks('PROFILE_INVALID');

/**
 * 抛出描述错误。
 * @param message 说明
 * @param hint 建议
 * @returns 不返回
 */
function invalid(message: string, hint?: string): never {
  throw new FcdevError(message, { code: 'PROFILE_INVALID', hint });
}

/**
 * 判断值是不是普通对象（非 null、非数组）。
 * @param value 值
 * @returns 是对象时为 true
 */
function isRecord(value: unknown): value is Record<string, unknown> {
  return value !== null && typeof value === 'object' && !Array.isArray(value);
}

/**
 * 检查一个启动步骤。
 * @param step 步骤
 * @param where 位置说明，用在错误里
 * @returns 步骤的副本
 */
function normalizeStep(step: unknown, where: string): BootStep {
  if (!isRecord(step)) return invalid(`${where} 必须是对象`);
  const stepName = step.step;
  const fields = typeof stepName === 'string' && Object.hasOwn(STEP_FIELDS, stepName) ? STEP_FIELDS[stepName] : undefined;
  if (fields === undefined) return invalid(`${where} 的 step 不认识：${String(stepName)}`, `可用的步骤：${BOOT_STEP_NAMES.join('、')}`);
  for (const key of Object.keys(step)) {
    if (key !== 'step' && !Object.hasOwn(fields, key)) invalid(`${where}（${String(stepName)}）没有字段 ${key}`);
  }
  for (const [key, kind] of Object.entries(fields)) {
    const value = step[key];
    const optional = kind.endsWith('?');
    if (value === undefined) {
      if (!optional) invalid(`${where}（${String(stepName)}）缺少字段 ${key}`);
      continue;
    }
    const base = optional ? kind.slice(0, -1) : kind;
    const ok =
      base === 'boolean'
        ? typeof value === 'boolean'
        : base === 'string'
          ? typeof value === 'string' && value !== ''
          : typeof value === 'number' && Number.isInteger(value) && value >= 1 && value <= 10_000_000;
    if (!ok) invalid(`${where}（${String(stepName)}）的 ${key} 不合法：${String(value)}`);
  }
  return Object.freeze({ ...step, step: String(stepName) });
}

/**
 * 检查一种输入。
 * @param name 输入名
 * @param input 输入的描述
 * @returns 规范化的输入
 */
function normalizeInput(name: string, input: unknown): Readonly<ProfileInput> {
  if (!isRecord(input)) return invalid(`输入 ${name} 必须是对象`);
  if (typeof input.format !== 'string' || input.format === '') invalid(`输入 ${name} 要写 format（映像格式名）`);
  if (input.formatOptions !== undefined && !isRecord(input.formatOptions)) invalid(`输入 ${name} 的 formatOptions 必须是对象`);
  if (input.boot !== undefined && !Array.isArray(input.boot)) invalid(`输入 ${name} 的 boot 必须是数组`);
  const boot: unknown[] = input.boot ?? [];
  return Object.freeze({
    title: typeof input.title === 'string' ? input.title : name,
    format: input.format,
    formatOptions: Object.freeze({ ...(input.formatOptions as Record<string, unknown> | undefined) }),
    boot: Object.freeze(boot.map((step, index) => normalizeStep(step, `输入 ${name} 的第 ${index + 1} 个启动步骤`)))
  });
}

/**
 * 检查并规范化目标描述。
 * @param data 解析后的 JSON
 * @param directory 描述文件所在的目录，符号集的相对路径据此换成绝对路径
 * @returns 目标描述
 * @throws FcdevError 描述不合法
 */
export function normalizeProfile(data: unknown, directory: string): Readonly<TargetProfile> {
  if (!isRecord(data)) return invalid('目标描述必须是 JSON 对象');
  if (typeof data.name !== 'string' || data.name === '') invalid('目标描述要写 name');
  const rawInputs = data.inputs;
  if (!isRecord(rawInputs) || Object.keys(rawInputs).length === 0) return invalid('目标描述要写至少一种输入（inputs）');
  const inputs = Object.fromEntries(Object.entries(rawInputs).map(([name, input]) => [name, normalizeInput(name, input)]));
  const inputNames = Object.keys(inputs);
  const defaultInput = (data.defaultInput ?? (inputNames.length === 1 ? inputNames[0] : undefined)) as string | undefined;
  if (defaultInput !== undefined && !Object.hasOwn(inputs, defaultInput)) invalid(`defaultInput 指向不存在的输入：${String(defaultInput)}`);
  const symbolSets = data.symbolSets;
  if (symbolSets !== undefined && (!Array.isArray(symbolSets) || symbolSets.some((item) => typeof item !== 'string' || item === ''))) {
    invalid('symbolSets 必须是文件路径的数组');
  }
  const graphicsPorts = data.graphicsPorts;
  if (graphicsPorts !== undefined && !Array.isArray(graphicsPorts)) invalid('graphicsPorts 必须是地址的数组');
  return Object.freeze({
    name: data.name,
    title: typeof data.title === 'string' ? data.title : data.name,
    symbolSets: Object.freeze(((symbolSets ?? []) as string[]).map((item) => path.resolve(directory, item))),
    graphicsPorts: Object.freeze(((graphicsPorts ?? []) as unknown[]).map((port) => toAddress(port, 'graphicsPorts'))),
    defaultInput,
    inputs: Object.freeze(inputs)
  });
}

/**
 * 选出目标描述里的一种输入。
 * @param profile 目标描述
 * @param inputName 输入名；省略时用 defaultInput
 * @returns 输入
 * @throws FcdevError 输入不存在，或没有指定又没有默认输入
 */
export function selectProfileInput(profile: Readonly<TargetProfile>, inputName: string | undefined): SelectedInput {
  const name = inputName ?? profile.defaultInput;
  const names = Object.keys(profile.inputs);
  if (name === undefined) {
    throw new FcdevError(`目标 ${profile.name} 有多种输入，要指定 input`, { code: 'PROFILE_INPUT_UNKNOWN', hint: `可用的输入：${names.join('、')}` });
  }
  const input = Object.hasOwn(profile.inputs, name) ? profile.inputs[name] : undefined;
  if (input === undefined) {
    throw new FcdevError(`目标 ${profile.name} 没有输入 ${name}`, { code: 'PROFILE_INPUT_UNKNOWN', hint: `可用的输入：${names.join('、')}` });
  }
  return { name, ...input };
}
