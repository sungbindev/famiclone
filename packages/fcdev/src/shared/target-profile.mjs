// ------------------------------------------------------------------------
// 名称：target-profile.mjs
// 说明：目标描述（profile.json）的读取与检查：一台机器的符号集、图形数据端口，以及每种输入（卡带、程序、BIOS）的映像格式与启动步骤。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-21
// 备注：反汇编读格式、符号集与图形数据端口，模拟器读启动步骤；内置的描述在包里的 targets/<名>/profile.json，
//       用户或模型也可以在工程里自带一份（给路径）；符号集的路径相对描述文件所在的目录，读出时换成绝对路径。
// ------------------------------------------------------------------------
import { existsSync, readdirSync, readFileSync } from 'node:fs';
import path from 'node:path';
import { fileURLToPath } from 'node:url';
import { FcdevError } from './errors.mjs';
import { createParamChecks } from './param-checks.mjs';

/** 内置目标描述所在的目录：包里的 targets */
const BUILTIN_DIRECTORY = fileURLToPath(new URL('../../targets/', import.meta.url));

/** 描述文件名 */
export const PROFILE_FILE = 'profile.json';

/** 内置描述的名字：小写字母、数字与连字符 */
const BUILTIN_NAME = /^[a-z0-9][a-z0-9-]*$/;

/** 各种启动步骤允许的字段（step 之外）及其检查 */
const STEP_FIELDS = {
  familyKeyboard: { connected: 'boolean' },
  cartridge: {},
  machine: { machine: 'string' },
  run: { frames: 'frames' },
  key: { code: 'string', downFrames: 'frames?', upFrames: 'frames?' },
  type: { text: 'string', downFrames: 'frames?', upFrames: 'frames?' },
};

/** 全部启动步骤的名字 */
export const BOOT_STEP_NAMES = Object.freeze(Object.keys(STEP_FIELDS));

const { toAddress } = createParamChecks('PROFILE_INVALID');

/**
 * 抛出描述错误。
 * @param {string} message 说明
 * @param {string} [hint] 建议
 * @returns {never}
 */
function invalid(message, hint) {
  throw new FcdevError(message, { code: 'PROFILE_INVALID', hint });
}

/**
 * 检查一个启动步骤。
 * @param {unknown} step 步骤
 * @param {string} where 位置说明，用在错误里
 * @returns {Readonly<Record<string, unknown>>} 步骤的副本
 */
function normalizeStep(step, where) {
  if (step === null || typeof step !== 'object' || Array.isArray(step)) invalid(`${where} 必须是对象`);
  const fields = Object.hasOwn(STEP_FIELDS, step.step) ? STEP_FIELDS[step.step] : undefined;
  if (fields === undefined) invalid(`${where} 的 step 不认识：${String(step.step)}`, `可用的步骤：${BOOT_STEP_NAMES.join('、')}`);
  for (const key of Object.keys(step)) {
    if (key !== 'step' && !Object.hasOwn(fields, key)) invalid(`${where}（${step.step}）没有字段 ${key}`);
  }
  for (const [key, kind] of Object.entries(fields)) {
    const value = step[key];
    const optional = kind.endsWith('?');
    if (value === undefined) {
      if (!optional) invalid(`${where}（${step.step}）缺少字段 ${key}`);
      continue;
    }
    const base = optional ? kind.slice(0, -1) : kind;
    const ok = base === 'boolean' ? typeof value === 'boolean'
      : base === 'string' ? typeof value === 'string' && value !== ''
        : Number.isInteger(value) && value >= 1 && value <= 10_000_000;
    if (!ok) invalid(`${where}（${step.step}）的 ${key} 不合法：${String(value)}`);
  }
  return Object.freeze({ ...step });
}

/**
 * 检查一种输入。
 * @param {string} name 输入名
 * @param {unknown} input 输入的描述
 * @returns {Readonly<{title: string, format: string, formatOptions: Record<string, unknown>, boot: ReadonlyArray<Readonly<Record<string, unknown>>>}>} 规范化的输入
 */
function normalizeInput(name, input) {
  if (input === null || typeof input !== 'object' || Array.isArray(input)) invalid(`输入 ${name} 必须是对象`);
  if (typeof input.format !== 'string' || input.format === '') invalid(`输入 ${name} 要写 format（映像格式名）`);
  if (input.formatOptions !== undefined && (input.formatOptions === null || typeof input.formatOptions !== 'object' || Array.isArray(input.formatOptions))) {
    invalid(`输入 ${name} 的 formatOptions 必须是对象`);
  }
  if (input.boot !== undefined && !Array.isArray(input.boot)) invalid(`输入 ${name} 的 boot 必须是数组`);
  return Object.freeze({
    title: typeof input.title === 'string' ? input.title : name,
    format: input.format,
    formatOptions: Object.freeze({ ...(input.formatOptions ?? {}) }),
    boot: Object.freeze((input.boot ?? []).map((step, index) => normalizeStep(step, `输入 ${name} 的第 ${index + 1} 个启动步骤`))),
  });
}

/**
 * 检查并规范化目标描述。
 * @param {unknown} data 解析后的 JSON
 * @param {string} directory 描述文件所在的目录，符号集的相对路径据此换成绝对路径
 * @returns {Readonly<TargetProfile>} 目标描述
 * @throws {FcdevError} 描述不合法
 */
export function normalizeProfile(data, directory) {
  if (data === null || typeof data !== 'object' || Array.isArray(data)) invalid('目标描述必须是 JSON 对象');
  if (typeof data.name !== 'string' || data.name === '') invalid('目标描述要写 name');
  const rawInputs = data.inputs;
  if (rawInputs === null || typeof rawInputs !== 'object' || Array.isArray(rawInputs) || Object.keys(rawInputs).length === 0) invalid('目标描述要写至少一种输入（inputs）');
  const inputs = Object.fromEntries(Object.entries(rawInputs).map(([name, input]) => [name, normalizeInput(name, input)]));
  const inputNames = Object.keys(inputs);
  const defaultInput = data.defaultInput ?? (inputNames.length === 1 ? inputNames[0] : undefined);
  if (defaultInput !== undefined && !Object.hasOwn(inputs, defaultInput)) invalid(`defaultInput 指向不存在的输入：${String(defaultInput)}`);
  if (data.symbolSets !== undefined && (!Array.isArray(data.symbolSets) || data.symbolSets.some((item) => typeof item !== 'string' || item === ''))) invalid('symbolSets 必须是文件路径的数组');
  if (data.graphicsPorts !== undefined && !Array.isArray(data.graphicsPorts)) invalid('graphicsPorts 必须是地址的数组');
  return Object.freeze({
    name: data.name,
    title: typeof data.title === 'string' ? data.title : data.name,
    symbolSets: Object.freeze((data.symbolSets ?? []).map((item) => path.resolve(directory, item))),
    graphicsPorts: Object.freeze((data.graphicsPorts ?? []).map((port) => toAddress(port, 'graphicsPorts'))),
    defaultInput,
    inputs: Object.freeze(inputs),
  });
}

/**
 * @typedef {object} TargetProfile
 * @property {string} name 名字
 * @property {string} title 显示名
 * @property {readonly string[]} symbolSets 符号集文件的绝对路径
 * @property {readonly number[]} graphicsPorts 图形数据端口的 CPU 地址
 * @property {string|undefined} defaultInput 没有指定输入时用的输入名
 * @property {Readonly<Record<string, ReturnType<typeof normalizeInput>>>} inputs 各种输入
 */

/**
 * 读取目标描述。
 * @param {string} reference 内置描述的名字（如 nes），或描述文件的路径
 * @param {{cwd?: string}} [options] 相对路径的起点，默认当前目录
 * @returns {{profile: Readonly<TargetProfile>, file: string}} 目标描述与它的文件路径
 * @throws {FcdevError} 找不到或不合法
 */
export function readTargetProfile(reference, { cwd = process.cwd() } = {}) {
  const builtin = BUILTIN_NAME.test(reference) ? path.join(BUILTIN_DIRECTORY, reference, PROFILE_FILE) : undefined;
  const file = builtin !== undefined && existsSync(builtin) ? builtin : path.resolve(cwd, reference);
  if (!existsSync(file)) {
    throw new FcdevError(`找不到目标描述 ${reference}`, { code: 'PROFILE_NOT_FOUND', hint: `内置的目标描述有：${listBuiltinProfiles().join('、') || '没有'}；也可以给 profile.json 的路径` });
  }
  let data;
  try {
    data = JSON.parse(readFileSync(file, 'utf8'));
  } catch (error) {
    throw new FcdevError(`目标描述 ${file} 不是合法的 JSON`, { code: 'PROFILE_INVALID', cause: error });
  }
  try {
    return { profile: normalizeProfile(data, path.dirname(file)), file };
  } catch (error) {
    if (error instanceof FcdevError && error.code === 'PROFILE_INVALID') throw new FcdevError(`${file}：${error.message}`, { code: 'PROFILE_INVALID', hint: error.hint, cause: error });
    throw error;
  }
}

/**
 * 列出内置目标描述的名字。
 * @returns {string[]} 名字，按字母顺序
 */
export function listBuiltinProfiles() {
  if (!existsSync(BUILTIN_DIRECTORY)) return [];
  return readdirSync(BUILTIN_DIRECTORY, { withFileTypes: true })
    .filter((entry) => entry.isDirectory() && existsSync(path.join(BUILTIN_DIRECTORY, entry.name, PROFILE_FILE)))
    .map((entry) => entry.name)
    .sort();
}

/**
 * 选出目标描述里的一种输入。
 * @param {Readonly<TargetProfile>} profile 目标描述
 * @param {string|undefined} inputName 输入名；省略时用 defaultInput
 * @returns {{name: string} & ReturnType<typeof normalizeInput>} 输入
 * @throws {FcdevError} 输入不存在，或没有指定又没有默认输入
 */
export function selectProfileInput(profile, inputName) {
  const name = inputName ?? profile.defaultInput;
  const names = Object.keys(profile.inputs);
  if (name === undefined) throw new FcdevError(`目标 ${profile.name} 有多种输入，要指定 input`, { code: 'PROFILE_INPUT_UNKNOWN', hint: `可用的输入：${names.join('、')}` });
  if (!Object.hasOwn(profile.inputs, name)) throw new FcdevError(`目标 ${profile.name} 没有输入 ${name}`, { code: 'PROFILE_INPUT_UNKNOWN', hint: `可用的输入：${names.join('、')}` });
  return { name, ...profile.inputs[name] };
}
