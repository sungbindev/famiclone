// ------------------------------------------------------------------------
// 名称：emulator-install.mjs
// 说明：随包的 Ember：找到 fcdev-ember 包里的程序，复制到用户级目录（%LOCALAPPDATA%\fcdev\ember）并返回那份的路径；包里的版本更新时替换用户级副本。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-21
// 备注：Ember 自带升级，升级时覆盖自己所在位置的程序；放在项目的 node_modules 里每个项目一份、npm install 还会还原成旧版，所以全机共用一份用户级副本。
//       副本旁的 installed.json 记录复制时的 Ember 版本与 SHA-256：现在的文件与记录相同说明没被 Ember 升级过，包里的版本更新时才替换；
//       文件与记录不同（被 Ember 升级过，或用户自己放了别的）就不动它。程序正在运行（文件被占用）时替换会失败，这时继续用旧副本；用户级目录写不进去时直接运行包里的程序。
//       环境变量 FCDEV_EMBER_HOME 指定用户级目录，FCDEV_EMBER_PACKAGE 指定模拟器包的目录（开发与测试用，指向不存在的目录等于没有随包的 Ember）。
// ------------------------------------------------------------------------
import { createHash, randomBytes } from 'node:crypto';
import { copyFileSync, existsSync, mkdirSync, readFileSync, renameSync, rmSync, writeFileSync } from 'node:fs';
import { createRequire } from 'node:module';
import { homedir } from 'node:os';
import path from 'node:path';

/** 用户级目录的环境变量名 */
export const EMBER_HOME_ENV_NAME = 'FCDEV_EMBER_HOME';

/** 模拟器包目录的环境变量名 */
export const EMBER_PACKAGE_ENV_NAME = 'FCDEV_EMBER_PACKAGE';

/** 模拟器包的包名 */
const EMBER_PACKAGE_NAME = 'fcdev-ember';

/** 包内程序的相对路径 */
const PACKAGE_EXECUTABLE = path.join('bin', 'Ember.exe');

/** 用户级目录里的程序名与记录文件名 */
const INSTALLED_EXECUTABLE = 'Ember.exe';
const INSTALLED_RECORD = 'installed.json';

/**
 * 计算文件的 SHA-256。
 * @param {string} file 文件路径
 * @returns {string} 十六进制摘要
 */
function hashFile(file) {
  return createHash('sha256').update(readFileSync(file)).digest('hex');
}

/**
 * 读 JSON 文件，不存在或不合法时返回 null。
 * @param {string} file 文件路径
 * @returns {any} 解析结果或 null
 */
function readJsonOrNull(file) {
  try {
    return JSON.parse(readFileSync(file, 'utf8'));
  } catch {
    return null;
  }
}

/**
 * 比较两个“主.次.修订”版本号。
 * @param {string} left 版本号
 * @param {string} right 版本号
 * @returns {number} 正数表示 left 更新，负数表示 right 更新，0 相同
 */
export function compareVersions(left, right) {
  const a = String(left).split('.').map(Number);
  const b = String(right).split('.').map(Number);
  for (let index = 0; index < 3; index += 1) {
    const difference = (a[index] ?? 0) - (b[index] ?? 0);
    if (difference !== 0) return difference;
  }
  return 0;
}

/**
 * 用户级目录：当前平台不是 Windows 时没有。
 * @param {NodeJS.ProcessEnv} env 环境变量
 * @param {string} platform 平台名
 * @returns {string|null} 目录；没有时为 null
 */
export function userEmberDirectory(env = process.env, platform = process.platform) {
  if (env[EMBER_HOME_ENV_NAME]) return path.resolve(env[EMBER_HOME_ENV_NAME]);
  if (platform !== 'win32') return null;
  return path.join(env.LOCALAPPDATA ?? path.join(homedir(), 'AppData', 'Local'), 'fcdev', 'ember');
}

/**
 * 模拟器包的目录：环境变量指定的，否则从 fcdev 的依赖里找 fcdev-ember。
 * @param {NodeJS.ProcessEnv} env 环境变量
 * @returns {string|null} 目录；没有安装时为 null
 */
export function bundledEmberDirectory(env = process.env) {
  if (env[EMBER_PACKAGE_ENV_NAME]) return path.resolve(env[EMBER_PACKAGE_ENV_NAME]);
  try {
    return path.dirname(createRequire(import.meta.url).resolve(`${EMBER_PACKAGE_NAME}/package.json`));
  } catch {
    return null;
  }
}

/**
 * 把包里的程序复制到用户级目录（先写临时文件再改名，改名失败说明程序正在运行）。
 * @param {string} source 包里的程序
 * @param {string} installDirectory 用户级目录
 * @param {string} emberVersion 包里的 Ember 版本
 * @param {string} sha256 包里程序的 SHA-256
 * @returns {boolean} 成功为 true；写不进去（权限、文件被占用）为 false
 */
function copyIntoPlace(source, installDirectory, emberVersion, sha256) {
  const target = path.join(installDirectory, INSTALLED_EXECUTABLE);
  const temporary = `${target}.${randomBytes(4).toString('hex')}.tmp`;
  try {
    mkdirSync(installDirectory, { recursive: true });
    copyFileSync(source, temporary);
    renameSync(temporary, target);
    writeFileSync(path.join(installDirectory, INSTALLED_RECORD), `${JSON.stringify({ emberVersion, sha256 }, null, 2)}\n`);
    return true;
  } catch {
    rmSync(temporary, { force: true });
    return false;
  }
}

/**
 * 找到随包的 Ember：需要时安装到用户级目录，返回可运行的程序。
 * @param {{env?: NodeJS.ProcessEnv, platform?: string}} [options] 环境变量与平台
 * @returns {{executable: string, source: 'installed'|'package', emberVersion: string}|null} 程序、来源与包里的 Ember 版本；没有随包的 Ember 时为 null
 */
export function installBundledEmber({ env = process.env, platform = process.platform } = {}) {
  const installDirectory = userEmberDirectory(env, platform);
  const packageDirectory = bundledEmberDirectory(env);
  if (installDirectory === null || packageDirectory === null) return null;
  const manifest = readJsonOrNull(path.join(packageDirectory, 'manifest.json'));
  const packaged = path.join(packageDirectory, PACKAGE_EXECUTABLE);
  if (manifest === null || typeof manifest.emberVersion !== 'string' || !existsSync(packaged)) return null;
  const sha256 = manifest.files?.[PACKAGE_EXECUTABLE.split(path.sep).join('/')] ?? hashFile(packaged);
  const bundled = { emberVersion: manifest.emberVersion };

  const target = path.join(installDirectory, INSTALLED_EXECUTABLE);
  const record = readJsonOrNull(path.join(installDirectory, INSTALLED_RECORD));
  if (existsSync(target)) {
    const untouched = record !== null && typeof record.sha256 === 'string' && hashFile(target) === record.sha256;
    const needsReplace = untouched && compareVersions(manifest.emberVersion, record.emberVersion) > 0;
    if (!needsReplace) return { executable: target, source: 'installed', ...bundled };
  }
  if (copyIntoPlace(packaged, installDirectory, manifest.emberVersion, sha256)) return { executable: target, source: 'installed', ...bundled };
  // 替换失败：旧副本正在运行时继续用旧副本（下次再试）；用户级目录写不进去、又没有旧副本时，本次直接运行包里的程序
  return existsSync(target) && record !== null ? { executable: target, source: 'installed', ...bundled } : { executable: packaged, source: 'package', ...bundled };
}
