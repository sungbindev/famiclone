// ------------------------------------------------------------------------
// 名称：config-loader.mjs
// 说明：查找并读取项目配置 fcdev.json：校验键与类型，补默认值，把相对路径换成绝对路径。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-10
// 备注：未知的键直接报错，避免拼写错误被忽略；配置项含义见 docs/02-命令行与构建.md 第 3 节。
// ------------------------------------------------------------------------
import { existsSync } from 'node:fs';
import path from 'node:path';
import { FcdevError, readJsonFile } from '../shared/index.mjs';

/** 配置文件名 */
export const CONFIG_FILE_NAME = 'fcdev.json';

/** 配置项的类型描述 */
const KEY_TYPES = {
  target: 'string',
  name: 'string',
  out: 'string',
  optimize: 'boolean',
  define: 'object',
  include: 'string-array',
  toolchain: 'string',
  firmware: 'string',
  emulator: 'string',
  targetOptions: 'object',
};

/**
 * @typedef {object} ProjectConfig
 * @property {string|null} file 配置文件路径；没有配置文件时为 null
 * @property {string} root 项目根目录（配置文件所在目录，没有配置时为起始目录）
 * @property {string|undefined} target 目标名
 * @property {string|undefined} name 目录构建时的程序名
 * @property {string} outDir 输出目录的绝对路径
 * @property {boolean} optimize 是否优化
 * @property {Record<string, string|number>} define 预定义宏
 * @property {string[]} include 额外头文件目录的绝对路径
 * @property {string|undefined} toolchain 自备 cc65 根目录的绝对路径
 * @property {string|undefined} firmware 固件文件的绝对路径
 * @property {string|undefined} emulator Ember 可执行文件的路径
 * @property {Record<string, unknown>} targetOptions 目标专用选项
 */

/**
 * 从起始目录向上查找最近的 fcdev.json。
 * @param {string} startDir 起始目录
 * @returns {string|null} 配置文件的绝对路径；找不到时为 null
 */
export function findConfigFile(startDir) {
  let current = path.resolve(startDir);
  for (;;) {
    const candidate = path.join(current, CONFIG_FILE_NAME);
    if (existsSync(candidate)) return candidate;
    const parent = path.dirname(current);
    if (parent === current) return null;
    current = parent;
  }
}

/**
 * 检查一个配置值是否符合类型描述。
 * @param {unknown} value 配置值
 * @param {string} type 类型描述
 * @returns {boolean} 符合时为 true
 */
function matchesType(value, type) {
  if (type === 'string-array') return Array.isArray(value) && value.every((item) => typeof item === 'string');
  if (type === 'object') return typeof value === 'object' && value !== null && !Array.isArray(value);
  return typeof value === type;
}

/**
 * 校验并规范化配置内容。
 * @param {unknown} raw 解析后的 JSON
 * @param {string|null} file 配置文件路径，用于出错提示与解析相对路径
 * @param {string} fallbackRoot 没有配置文件时的项目根目录
 * @returns {ProjectConfig} 规范化后的配置
 * @throws {FcdevError} 出现未知的键或类型不对
 */
export function normalizeConfig(raw, file, fallbackRoot) {
  const where = file ?? CONFIG_FILE_NAME;
  if (typeof raw !== 'object' || raw === null || Array.isArray(raw)) {
    throw new FcdevError(`${where} 的内容必须是对象`, { code: 'CONFIG_INVALID' });
  }
  for (const [key, value] of Object.entries(raw)) {
    const type = KEY_TYPES[key];
    if (!type) {
      throw new FcdevError(`${where} 中有未知的配置项：${key}`, {
        code: 'CONFIG_UNKNOWN_KEY',
        hint: `可用的配置项：${Object.keys(KEY_TYPES).join('、')}`,
      });
    }
    if (!matchesType(value, type)) {
      throw new FcdevError(`${where} 中的 ${key} 类型不对，应为 ${type}`, { code: 'CONFIG_INVALID_TYPE' });
    }
  }
  const root = file ? path.dirname(file) : path.resolve(fallbackRoot);
  const resolveFromRoot = (value) => (value === undefined ? undefined : path.resolve(root, value));
  for (const [key, value] of Object.entries(raw.define ?? {})) {
    if (typeof value !== 'string' && typeof value !== 'number') {
      throw new FcdevError(`${where} 的 define.${key} 必须是字符串或数字`, { code: 'CONFIG_INVALID_TYPE' });
    }
  }
  return {
    file,
    root,
    target: raw.target,
    name: raw.name,
    outDir: path.resolve(root, raw.out ?? 'build'),
    optimize: raw.optimize ?? true,
    define: raw.define ?? {},
    include: (raw.include ?? []).map((item) => path.resolve(root, item)),
    toolchain: resolveFromRoot(raw.toolchain),
    firmware: resolveFromRoot(raw.firmware),
    emulator: resolveFromRoot(raw.emulator),
    targetOptions: raw.targetOptions ?? {},
  };
}

/**
 * 读取构建路径对应的项目配置。
 * @param {string} startPath 构建路径（文件或目录）所在的起始目录
 * @param {string} fallbackRoot 找不到配置文件时用作项目根的目录（终端当前目录）
 * @returns {ProjectConfig} 项目配置
 */
export function loadProjectConfig(startPath, fallbackRoot) {
  const file = findConfigFile(startPath);
  if (!file) return normalizeConfig({}, null, fallbackRoot);
  return normalizeConfig(readJsonFile(file), file, fallbackRoot);
}
