// ------------------------------------------------------------------------
// 名称：target-registry.mjs
// 说明：目标注册表：读取 targets/<名>/target.json，校验字段，解析目标专用选项与路径。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-10
// 备注：目标定义的字段含义见仓库根目录 docs/02-目标规范.md 第 2 节；新增机型只需新增目标目录，不改这里。
// ------------------------------------------------------------------------
import { readdirSync } from 'node:fs';
import path from 'node:path';
import { fileURLToPath } from 'node:url';
import { FcdevError, isDirectory, readJsonFile } from '../shared/index.mjs';

/** 内置目标所在的目录 */
export const BUILTIN_TARGETS_DIR = fileURLToPath(new URL('../../targets/', import.meta.url));

/** 路径值里表示“相对于工具链根目录”的前缀 */
const TOOLCHAIN_PREFIX = 'toolchain:';

/** 运行方案的模式：rom 直接加载产物；firmware-disk 加载固件并挂载磁盘镜像 */
const RUN_MODES = new Set(['rom', 'firmware-disk']);

/** 目标专用选项允许的类型 */
const OPTION_TYPES = new Set(['boolean', 'string', 'number']);

/** 源文件转码支持的目标编码 */
const SOURCE_CHARSETS = new Set(['gb2312']);

/**
 * @typedef {object} BankedSpec 多页（换页）程序的定义
 * @property {string} option 目标专用选项名，值为库页个数
 * @property {string} linkConfig 链接配置模板（路径值），含 %BANK_MEMORY% 与 %BANK_SEGMENTS% 两行
 * @property {number} generatedFrom 模板里已写出前面的库页，从这一页起由 fcdev 按个数生成
 * @property {number} maxBanks 库页个数的上限
 * @property {string} memory 一个库页的内存区行，{n} 换成库页号
 * @property {string} segment 一个库页的段行，{n} 换成库页号
 */

/**
 * @typedef {object} Target
 * @property {string} name 标识
 * @property {string} title 显示名
 * @property {string} dir 目标目录的绝对路径
 * @property {string} cpu CPU 型号
 * @property {string} cc65Target 传给 cc65 工具的目标名
 * @property {string} linkConfig 链接配置（路径值）
 * @property {string[]} startup 启动代码（路径值）
 * @property {string[]} forceImports 链接时强制引入的符号（让库里的启动代码一定被链接）
 * @property {string[]} libraries 链接的库（路径值）
 * @property {string[]} include 头文件目录（路径值）
 * @property {string[]} asminclude 汇编引入目录（路径值）
 * @property {Record<string, string|number>} defines 预定义宏
 * @property {string} packager 打包器名
 * @property {Record<string, unknown>} packagerOptions 打包器选项
 * @property {string} extension 产物扩展名
 * @property {string|null} nameRule 程序名规则
 * @property {object|null} disk 磁盘镜像能力
 * @property {object|null} run 运行方案
 * @property {Record<string, {type: string, default: unknown, links: string[]}>} options 目标专用选项；布尔选项为 true 时额外链接 links 里的文件（路径值）
 * @property {BankedSpec|null} banked 多页（换页）程序的定义；不支持时为 null
 * @property {string|null} sourceCharset 机器用的文字编码；不为 null 时，构建前把 UTF-8 的源文件转成这个编码（目前只有 gb2312）
 */

/**
 * 校验一个字段是字符串数组。
 * @param {unknown} value 字段值
 * @param {string} field 字段名，用于出错提示
 * @param {string} name 目标名
 * @returns {string[]} 字段值
 */
function requireStringArray(value, field, name) {
  if (!Array.isArray(value) || !value.every((item) => typeof item === 'string')) {
    throw new FcdevError(`目标 ${name} 的 ${field} 必须是字符串数组`, { code: 'TARGET_INVALID' });
  }
  return value;
}

/**
 * 校验并规范化多页程序的定义。
 * @param {unknown} raw target.json 里的 banked；没有时为 undefined
 * @param {string} name 目标名
 * @param {Record<string, {type: string}>} options 已解析的目标专用选项
 * @returns {BankedSpec|null} 多页定义；没有时为 null
 * @throws {FcdevError} 字段缺失、类型不对，或对应的选项不是数字选项
 */
function parseBanked(raw, name, options) {
  if (raw === undefined) return null;
  const invalid = (reason) => new FcdevError(`目标 ${name} 的 banked ${reason}`, { code: 'TARGET_INVALID' });
  if (typeof raw !== 'object' || raw === null || Array.isArray(raw)) throw invalid('必须是对象');
  for (const field of ['option', 'linkConfig', 'memory', 'segment']) {
    if (typeof raw[field] !== 'string' || raw[field] === '') throw invalid(`缺少字符串字段 ${field}`);
  }
  for (const field of ['generatedFrom', 'maxBanks']) {
    if (!Number.isInteger(raw[field]) || raw[field] < 1) throw invalid(`的 ${field} 必须是不小于 1 的整数`);
  }
  if (options[raw.option]?.type !== 'number') throw invalid(`的 option（${raw.option}）必须是 options 里的数字选项`);
  return { option: raw.option, linkConfig: raw.linkConfig, generatedFrom: raw.generatedFrom, maxBanks: raw.maxBanks, memory: raw.memory, segment: raw.segment };
}

/**
 * 校验并规范化目标定义。
 * @param {unknown} raw target.json 的内容
 * @param {string} dir 目标目录
 * @returns {Target} 目标
 * @throws {FcdevError} 缺字段或类型不对
 */
export function parseTarget(raw, dir) {
  const fallbackName = path.basename(dir);
  if (typeof raw !== 'object' || raw === null || Array.isArray(raw)) {
    throw new FcdevError(`目标 ${fallbackName} 的 target.json 必须是对象`, { code: 'TARGET_INVALID' });
  }
  for (const field of ['name', 'title', 'cpu', 'cc65Target', 'linkConfig', 'packager', 'extension']) {
    if (typeof raw[field] !== 'string' || raw[field] === '') {
      throw new FcdevError(`目标 ${fallbackName} 缺少字符串字段 ${field}`, { code: 'TARGET_INVALID' });
    }
  }
  if (raw.name !== fallbackName) {
    throw new FcdevError(`目标目录 ${fallbackName} 与 target.json 中的 name（${raw.name}）不一致`, { code: 'TARGET_INVALID' });
  }
  const options = {};
  if (raw.run !== undefined && raw.run !== null) {
    if (typeof raw.run.emulator !== 'string' || !RUN_MODES.has(raw.run.mode)) {
      throw new FcdevError(`目标 ${raw.name} 的 run 必须包含 emulator，且 mode 为 ${[...RUN_MODES].join(' 或 ')}`, { code: 'TARGET_INVALID' });
    }
  }
  for (const [key, spec] of Object.entries(raw.options ?? {})) {
    if (!spec || !OPTION_TYPES.has(spec.type) || typeof spec.default !== spec.type) {
      throw new FcdevError(`目标 ${raw.name} 的选项 ${key} 定义不合法`, { code: 'TARGET_INVALID' });
    }
    if (spec.links !== undefined && (spec.type !== 'boolean' || !Array.isArray(spec.links) || !spec.links.every((item) => typeof item === 'string'))) {
      throw new FcdevError(`目标 ${raw.name} 的选项 ${key} 的 links 只能用于布尔选项，且必须是字符串数组`, { code: 'TARGET_INVALID' });
    }
    options[key] = { type: spec.type, default: spec.default, links: spec.links ?? [] };
  }
  const banked = parseBanked(raw.banked, raw.name, options);
  if (raw.sourceCharset !== undefined && !SOURCE_CHARSETS.has(raw.sourceCharset)) {
    throw new FcdevError(`目标 ${raw.name} 的 sourceCharset 只能是 ${[...SOURCE_CHARSETS].join(' 或 ')}`, { code: 'TARGET_INVALID' });
  }
  return {
    name: raw.name,
    title: raw.title,
    dir,
    cpu: raw.cpu,
    cc65Target: raw.cc65Target,
    linkConfig: raw.linkConfig,
    startup: requireStringArray(raw.startup ?? [], 'startup', raw.name),
    forceImports: requireStringArray(raw.forceImports ?? [], 'forceImports', raw.name),
    libraries: requireStringArray(raw.libraries ?? [], 'libraries', raw.name),
    include: requireStringArray(raw.include ?? [], 'include', raw.name),
    asminclude: requireStringArray(raw.asminclude ?? [], 'asminclude', raw.name),
    defines: raw.defines ?? {},
    packager: raw.packager,
    packagerOptions: raw.packagerOptions ?? {},
    extension: raw.extension,
    nameRule: raw.nameRule ?? null,
    disk: raw.disk ?? null,
    run: raw.run ?? null,
    options,
    banked,
    sourceCharset: raw.sourceCharset ?? null,
  };
}

/**
 * 列出目标目录下全部目标的名称。
 * @param {string} [targetsDir] 目标所在目录
 * @returns {string[]} 目标名，按字母排序
 */
export function listTargetNames(targetsDir = BUILTIN_TARGETS_DIR) {
  return readdirSync(targetsDir, { withFileTypes: true })
    .filter((entry) => entry.isDirectory())
    .map((entry) => entry.name)
    .sort();
}

/**
 * 读取一个目标。
 * @param {string} name 目标名
 * @param {string} [targetsDir] 目标所在目录
 * @returns {Target} 目标
 * @throws {FcdevError} 目标不存在或定义不合法
 */
export function loadTarget(name, targetsDir = BUILTIN_TARGETS_DIR) {
  const dir = path.join(targetsDir, name);
  if (!/^[a-z0-9-]+$/.test(name) || !isDirectory(dir)) {
    throw new FcdevError(`不支持的目标：${name}`, {
      code: 'TARGET_UNKNOWN',
      hint: `可用的目标：${listTargetNames(targetsDir).join('、')}`,
    });
  }
  return parseTarget(readJsonFile(path.join(dir, 'target.json')), dir);
}

/**
 * 把目标定义里的路径值换成绝对路径。
 * @param {Target} target 目标
 * @param {string} value 路径值：相对目标目录，或以 toolchain: 开头表示相对工具链根目录
 * @param {string} toolchainHome 工具链根目录
 * @returns {string} 绝对路径
 */
export function resolveTargetPath(target, value, toolchainHome) {
  if (value.startsWith(TOOLCHAIN_PREFIX)) return path.join(toolchainHome, value.slice(TOOLCHAIN_PREFIX.length));
  return path.join(target.dir, value);
}

/**
 * 合并用户给的目标专用选项与默认值，并校验。
 * @param {Target} target 目标
 * @param {Record<string, unknown>} given 配置里的 targetOptions
 * @returns {Record<string, unknown>} 完整的选项
 * @throws {FcdevError} 出现目标没有定义的选项，或类型不对
 */
export function resolveTargetOptions(target, given) {
  const result = {};
  for (const [key, spec] of Object.entries(target.options)) result[key] = spec.default;
  for (const [key, value] of Object.entries(given)) {
    const spec = target.options[key];
    if (!spec) {
      throw new FcdevError(`目标 ${target.name} 没有选项 ${key}`, {
        code: 'TARGET_OPTION_UNKNOWN',
        hint: Object.keys(target.options).length > 0 ? `可用的选项：${Object.keys(target.options).join('、')}` : '该目标没有可配置的选项',
      });
    }
    if (typeof value !== spec.type) {
      throw new FcdevError(`目标 ${target.name} 的选项 ${key} 类型应为 ${spec.type}`, { code: 'TARGET_OPTION_INVALID' });
    }
    if (target.banked?.option === key && (!Number.isInteger(value) || value < 0 || value > target.banked.maxBanks)) {
      throw new FcdevError(`目标 ${target.name} 的选项 ${key} 必须是 0 到 ${target.banked.maxBanks} 的整数`, { code: 'TARGET_OPTION_INVALID' });
    }
    result[key] = value;
  }
  return result;
}
