// ------------------------------------------------------------------------
// 名称：project-config.ts
// 说明：项目配置 fcdev.json 的数据类型，以及配置内容的校验与规范化。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-22
// 备注：未知的键直接报错，避免拼写错误被忽略；相对路径换成相对配置文件所在目录的绝对路径。
// ------------------------------------------------------------------------

import * as path from 'node:path';
import { FcdevError } from '../errors/fcdev-error';
import { isRecord } from '../json/json-text';

/** 项目配置文件名。 */
export const CONFIG_FILE_NAME = 'fcdev.json';

/** 配置项允许的类型。 */
const KEY_TYPES: Readonly<Record<string, 'string' | 'boolean' | 'object' | 'string-array'>> = {
  target: 'string',
  name: 'string',
  out: 'string',
  optimize: 'boolean',
  disk: 'boolean',
  define: 'object',
  include: 'string-array',
  firmware: 'string',
  diskImage: 'string',
  linkConfig: 'string',
  targetOptions: 'object'
};

/** 规范化后的项目配置。 */
export interface ProjectConfig {
  /** 配置文件路径；没有配置文件时为 null。 */
  readonly file: string | null;
  /** 项目根目录：配置文件所在目录，没有配置时为起始目录。 */
  readonly root: string;
  /** 目标名。 */
  readonly target: string | undefined;
  /** 目录构建时的程序名。 */
  readonly name: string | undefined;
  /** 输出目录的绝对路径。 */
  readonly outDir: string;
  /** 是否优化。 */
  readonly optimize: boolean;
  /** 构建时是否同时生成磁盘镜像（只对有磁盘能力的目标有效）。 */
  readonly disk: boolean;
  /** 预定义宏。 */
  readonly define: Readonly<Record<string, string | number>>;
  /** 额外头文件目录的绝对路径。 */
  readonly include: readonly string[];
  /** 固件文件的绝对路径。 */
  readonly firmware: string | undefined;
  /** 运行时挂到软驱 0 的磁盘镜像的绝对路径；只对运行方案为 firmware 的目标有效。 */
  readonly diskImage: string | undefined;
  /** 自定义链接配置的绝对路径；没有时用目标自带的链接配置。 */
  readonly linkConfig: string | undefined;
  /** 目标专用选项。 */
  readonly targetOptions: Readonly<Record<string, unknown>>;
}

/**
 * 判断配置值是否符合类型描述。
 * @param value 配置值。
 * @param type 类型描述。
 * @returns 符合时为 true。
 */
function matchesType(value: unknown, type: string): boolean {
  if (type === 'string-array') return Array.isArray(value) && value.every((item) => typeof item === 'string');
  if (type === 'object') return isRecord(value);
  return typeof value === type;
}

/**
 * 校验并规范化配置内容。
 * @param raw 解析后的 JSON。
 * @param file 配置文件路径，用于出错提示与解析相对路径；没有配置文件时为 null。
 * @param fallbackRoot 没有配置文件时的项目根目录。
 * @returns 规范化后的配置。
 * @throws FcdevError 出现未知的键或类型不对。
 */
export function normalizeConfig(raw: unknown, file: string | null, fallbackRoot: string): ProjectConfig {
  const where = file ?? CONFIG_FILE_NAME;
  if (!isRecord(raw)) throw new FcdevError(`${where} 的内容必须是对象`, { code: 'CONFIG_INVALID' });
  for (const [key, value] of Object.entries(raw)) {
    const type = KEY_TYPES[key];
    if (type === undefined) {
      throw new FcdevError(`${where} 中有未知的配置项：${key}`, {
        code: 'CONFIG_UNKNOWN_KEY',
        hint: `可用的配置项：${Object.keys(KEY_TYPES).join('、')}`
      });
    }
    if (!matchesType(value, type)) {
      throw new FcdevError(`${where} 中的 ${key} 类型不对，应为 ${type}`, { code: 'CONFIG_INVALID_TYPE' });
    }
  }
  const root = file === null ? path.resolve(fallbackRoot) : path.dirname(file);
  const define = (raw.define ?? {}) as Record<string, unknown>;
  for (const [key, value] of Object.entries(define)) {
    if (typeof value !== 'string' && typeof value !== 'number') {
      throw new FcdevError(`${where} 的 define.${key} 必须是字符串或数字`, { code: 'CONFIG_INVALID_TYPE' });
    }
  }
  const firmware = raw.firmware as string | undefined;
  const diskImage = raw.diskImage as string | undefined;
  const linkConfig = raw.linkConfig as string | undefined;
  return {
    file,
    root,
    target: raw.target as string | undefined,
    name: raw.name as string | undefined,
    outDir: path.resolve(root, (raw.out as string | undefined) ?? 'build'),
    optimize: (raw.optimize as boolean | undefined) ?? true,
    disk: (raw.disk as boolean | undefined) ?? true,
    define: define as Record<string, string | number>,
    include: ((raw.include as string[] | undefined) ?? []).map((item) => path.resolve(root, item)),
    firmware: firmware === undefined ? undefined : path.resolve(root, firmware),
    diskImage: diskImage === undefined ? undefined : path.resolve(root, diskImage),
    linkConfig: linkConfig === undefined ? undefined : path.resolve(root, linkConfig),
    targetOptions: (raw.targetOptions as Record<string, unknown> | undefined) ?? {}
  };
}
