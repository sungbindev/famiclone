// ------------------------------------------------------------------------
// 名称：target-parser.ts
// 说明：校验并规范化 target.json 的内容，得到目标定义。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-22
// 备注：新增机型只需新增目标目录，不改这里；字段含义见 docs/02-目标规范.md 第 2 节。
// ------------------------------------------------------------------------

import * as path from 'node:path';
import { FcdevError } from '../errors/fcdev-error';
import { isRecord } from '../json/json-text';
import type { BankedSpec, RunMode, Target, TargetOptionSpec, TargetOptionValue } from './target';

/** 运行方案允许的模式。 */
const RUN_MODES: readonly string[] = ['rom', 'firmware-disk', 'firmware'];

/** 目标专用选项允许的类型。 */
const OPTION_TYPES: readonly string[] = ['boolean', 'string', 'number', 'address'];

/** 源文件转码支持的目标编码。 */
const SOURCE_CHARSETS: readonly string[] = ['gb2312'];

/** 目标定义里必须是非空字符串的字段（extension 可以为空字符串，单独校验）。 */
const REQUIRED_STRING_FIELDS = ['name', 'title', 'cpu', 'cc65Target', 'linkConfig', 'packager'] as const;

/** 16 位地址的上限。 */
const ADDRESS_LIMIT = 0xffff;

/**
 * 抛出目标定义不合法的错误。
 * @param message 说明。
 * @returns 不会返回。
 */
function invalid(message: string): never {
  throw new FcdevError(message, { code: 'TARGET_INVALID' });
}

/**
 * 判断值是不是字符串数组。
 * @param value 任意值。
 * @returns 是字符串数组时为 true。
 */
function isStringArray(value: unknown): value is string[] {
  return Array.isArray(value) && value.every((item) => typeof item === 'string');
}

/**
 * 读取一个字符串数组字段，缺省为空数组。
 * @param raw target.json 的内容。
 * @param field 字段名。
 * @param name 目标名。
 * @returns 字段值。
 */
function readStringArray(raw: Record<string, unknown>, field: string, name: string): string[] {
  const value = raw[field] ?? [];
  if (!isStringArray(value)) invalid(`目标 ${name} 的 ${field} 必须是字符串数组`);
  return value;
}

/**
 * 校验并规范化多页程序的定义。
 * @param raw target.json 里的 banked；没有时为 undefined。
 * @param name 目标名。
 * @param options 已解析的目标专用选项。
 * @returns 多页定义；没有时为 null。
 */
function parseBanked(raw: unknown, name: string, options: Record<string, TargetOptionSpec>): BankedSpec | null {
  if (raw === undefined) return null;
  const fail = (reason: string): never => invalid(`目标 ${name} 的 banked ${reason}`);
  if (!isRecord(raw)) return fail('必须是对象');
  const text = (field: string): string => {
    const value = raw[field];
    return typeof value === 'string' && value !== '' ? value : fail(`缺少字符串字段 ${field}`);
  };
  const count = (field: string): number => {
    const value = raw[field];
    return typeof value === 'number' && Number.isInteger(value) && value >= 1 ? value : fail(`的 ${field} 必须是不小于 1 的整数`);
  };
  const option = text('option');
  const spec: BankedSpec = {
    option,
    linkConfig: text('linkConfig'),
    generatedFrom: count('generatedFrom'),
    maxBanks: count('maxBanks'),
    memory: text('memory'),
    segment: text('segment')
  };
  if (options[option]?.type !== 'number') fail(`的 option（${option}）必须是 options 里的数字选项`);
  return spec;
}

/**
 * 校验并规范化 options 字段。
 * @param raw target.json 里的 options；没有时为 undefined。
 * @param name 目标名。
 * @returns 目标专用选项。
 */
function parseOptions(raw: unknown, name: string): Record<string, TargetOptionSpec> {
  const options: Record<string, TargetOptionSpec> = {};
  for (const [key, spec] of Object.entries(raw ?? {})) {
    // address 选项的缺省值写成数字
    const defaultType = isRecord(spec) && spec.type === 'address' ? 'number' : isRecord(spec) ? spec.type : undefined;
    if (!isRecord(spec) || typeof spec.type !== 'string' || !OPTION_TYPES.includes(spec.type) || typeof spec.default !== defaultType) {
      invalid(`目标 ${name} 的选项 ${key} 定义不合法`);
    }
    if (spec.links !== undefined && (spec.type !== 'boolean' || !isStringArray(spec.links))) {
      invalid(`目标 ${name} 的选项 ${key} 的 links 只能用于布尔选项，且必须是字符串数组`);
    }
    if (spec.linkSymbol !== undefined && ((spec.type !== 'number' && spec.type !== 'address') || typeof spec.linkSymbol !== 'string' || !/^[A-Za-z_][A-Za-z0-9_]*$/.test(spec.linkSymbol))) {
      invalid(`目标 ${name} 的选项 ${key} 的 linkSymbol 只能用于数字与地址选项，且必须是符号名`);
    }
    const bound = (field: 'min' | 'max'): number | null => {
      const value = spec[field];
      if (value === undefined) return null;
      if (spec.type !== 'address' || typeof value !== 'number' || !Number.isInteger(value) || value < 0 || value > ADDRESS_LIMIT) {
        return invalid(`目标 ${name} 的选项 ${key} 的 ${field} 只能用于地址选项，且必须是 0 到 65535 的整数`);
      }
      return value;
    };
    const min = bound('min');
    const max = bound('max');
    if (spec.type === 'address') {
      const value = spec.default as number;
      if (!Number.isInteger(value) || value < (min ?? 0) || value > (max ?? ADDRESS_LIMIT)) invalid(`目标 ${name} 的选项 ${key} 的缺省值不是范围内的地址`);
    }
    options[key] = {
      type: spec.type as TargetOptionSpec['type'],
      default: spec.default as TargetOptionValue,
      links: (spec.links as string[] | undefined) ?? [],
      linkSymbol: (spec.linkSymbol as string | undefined) ?? null,
      min,
      max
    };
  }
  return options;
}

/**
 * 校验并规范化目标定义。
 * @param raw target.json 的内容。
 * @param dir 目标目录的绝对路径。
 * @returns 目标。
 * @throws FcdevError 缺字段或类型不对。
 */
export function parseTarget(raw: unknown, dir: string): Target {
  const fallbackName = path.basename(dir);
  if (!isRecord(raw)) return invalid(`目标 ${fallbackName} 的 target.json 必须是对象`);
  for (const field of REQUIRED_STRING_FIELDS) {
    if (typeof raw[field] !== 'string' || raw[field] === '') invalid(`目标 ${fallbackName} 缺少字符串字段 ${field}`);
  }
  const name = raw.name as string;
  if (name !== fallbackName) invalid(`目标目录 ${fallbackName} 与 target.json 中的 name（${name}）不一致`);
  if (typeof raw.extension !== 'string') invalid(`目标 ${fallbackName} 缺少字符串字段 extension（没有扩展名时写空字符串）`);

  const run = raw.run ?? null;
  if (run !== null && (!isRecord(run) || typeof run.emulator !== 'string' || typeof run.mode !== 'string' || !RUN_MODES.includes(run.mode))) {
    invalid(`目标 ${name} 的 run 必须包含 emulator，且 mode 为 ${RUN_MODES.join(' 或 ')}`);
  }
  if (run !== null && (run as { mode: string }).mode === 'firmware-disk' && typeof (run as { machine?: unknown }).machine !== 'string') {
    invalid(`目标 ${name} 的 run 在 firmware-disk 模式下必须包含 machine（整机机型）`);
  }
  const options = parseOptions(raw.options, name);
  const banked = parseBanked(raw.banked, name, options);
  if (raw.sourceCharset !== undefined && (typeof raw.sourceCharset !== 'string' || !SOURCE_CHARSETS.includes(raw.sourceCharset))) {
    invalid(`目标 ${name} 的 sourceCharset 只能是 ${SOURCE_CHARSETS.join(' 或 ')}`);
  }
  const disk = raw.disk ?? null;
  if (disk !== null && (!isRecord(disk) || typeof disk.packager !== 'string' || (disk.baseImage !== undefined && typeof disk.baseImage !== 'boolean'))) {
    invalid(`目标 ${name} 的 disk 必须包含 packager，且 baseImage 只能是布尔值`);
  }

  return {
    name,
    title: raw.title as string,
    dir,
    cpu: raw.cpu as string,
    cc65Target: raw.cc65Target as string,
    linkConfig: raw.linkConfig as string,
    startup: readStringArray(raw, 'startup', name),
    forceImports: readStringArray(raw, 'forceImports', name),
    libraries: readStringArray(raw, 'libraries', name),
    include: readStringArray(raw, 'include', name),
    asminclude: readStringArray(raw, 'asminclude', name),
    defines: (raw.defines ?? {}) as Record<string, string | number>,
    packager: raw.packager as string,
    packagerOptions: (raw.packagerOptions ?? {}) as Record<string, unknown>,
    extension: raw.extension as string,
    nameRule: typeof raw.nameRule === 'string' ? raw.nameRule : null,
    disk: disk === null ? null : { packager: (disk as { packager: string }).packager, baseImage: (disk as { baseImage?: boolean }).baseImage === true },
    run:
      run === null
        ? null
        : {
            emulator: (run as { emulator: string }).emulator,
            mode: (run as { mode: string }).mode as RunMode,
            machine: typeof (run as { machine?: unknown }).machine === 'string' ? (run as { machine: string }).machine : null
          },
    options,
    banked,
    sourceCharset: raw.sourceCharset === undefined ? null : (raw.sourceCharset as 'gb2312')
  };
}
