// ------------------------------------------------------------------------
// 名称：build-ember.mjs
// 说明：把 Ember 的发布产物组装成随包的模拟器包 fcdev-ember：Ember.exe、许可文件、第三方声明与清单 manifest.json，并写出包的 package.json。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-21
// 备注：用法 node tools/build-ember.mjs [--package]；程序取 build/ember/release/Ember-<版本>.exe（版本取 ember/package.json），
//       没有时先在 ember 目录运行 npm run package:win，或加 --package 由本脚本运行；包内的程序、许可、声明与清单是生成物，不入库，package.json 入库。
//       清单记录 Ember 的版本与每个文件的 SHA-256，fcdev 用它判断用户级副本要不要更新，release-check 用它核对包内文件。
// ------------------------------------------------------------------------
import { spawnSync } from 'node:child_process';
import { createHash } from 'node:crypto';
import { cpSync, existsSync, mkdirSync, readFileSync, rmSync, writeFileSync } from 'node:fs';
import path from 'node:path';
import { fileURLToPath } from 'node:url';
import { FcdevError } from '../packages/fcdev/src/shared/index.mjs';

/** 仓库根目录 */
const REPO_ROOT = path.resolve(path.dirname(fileURLToPath(import.meta.url)), '..');

/** 模拟器包的包名 */
export const EMBER_PACKAGE_NAME = 'fcdev-ember';

/** 包内程序的相对路径 */
export const EMBER_EXECUTABLE = 'bin/Ember.exe';

/** 包里的生成物（不含 package.json）：程序、许可、第三方声明 */
export const EMBER_PACKAGE_FILES = [EMBER_EXECUTABLE, 'LICENSE', 'THIRD_PARTY_NOTICES.txt'];

/** 运行 npm run package:win 的超时，单位毫秒 */
const PACKAGE_TIMEOUT_MS = 60 * 60 * 1000;

/**
 * 计算文件的 SHA-256。
 * @param {string} file 文件路径
 * @returns {string} 十六进制摘要
 */
function hashFile(file) {
  return createHash('sha256').update(readFileSync(file)).digest('hex');
}

/**
 * Ember 发布产物的位置。
 * @param {string} repoRoot 仓库根目录
 * @returns {{version: string, executable: string, license: string, notices: string, packageJson: string}} Ember 的版本与各文件路径
 */
export function emberArtifacts(repoRoot = REPO_ROOT) {
  const emberDir = path.join(repoRoot, 'ember');
  const version = JSON.parse(readFileSync(path.join(emberDir, 'package.json'), 'utf8')).version;
  return {
    version,
    executable: path.join(repoRoot, 'build', 'ember', 'release', `Ember-${version}.exe`),
    license: path.join(emberDir, 'LICENSE'),
    notices: path.join(repoRoot, 'build', 'ember', 'third-party', 'THIRD_PARTY_NOTICES.txt'),
    packageJson: path.join(repoRoot, 'packages', 'fcdev', 'package.json'),
  };
}

/**
 * 组装模拟器包：复制程序、许可与第三方声明，写出清单与 package.json。
 * @param {{executable: string, license: string, notices: string, emberVersion: string, fcdevVersion: string, packageDir: string}} input 来源文件、版本与包目录
 * @returns {{packageDir: string, files: string[]}} 包目录与清单里的文件
 * @throws {FcdevError} 来源文件不存在
 */
export function assembleEmberPackage({ executable, license, notices, emberVersion, fcdevVersion, packageDir }) {
  for (const [file, hint] of [[executable, '先在 ember 目录运行 npm run package:win，或给本脚本加 --package'], [license, 'Ember 的 LICENSE 不在仓库里'], [notices, '先在 ember 目录运行 npm run package:win（它会生成第三方声明）']]) {
    if (!existsSync(file)) throw new FcdevError(`找不到 ${file}`, { code: 'EMBER_ARTIFACT_MISSING', hint });
  }
  rmSync(path.join(packageDir, 'bin'), { recursive: true, force: true });
  mkdirSync(path.join(packageDir, 'bin'), { recursive: true });
  cpSync(executable, path.join(packageDir, EMBER_EXECUTABLE));
  cpSync(license, path.join(packageDir, 'LICENSE'));
  cpSync(notices, path.join(packageDir, 'THIRD_PARTY_NOTICES.txt'));

  writeFileSync(path.join(packageDir, 'manifest.json'), `${JSON.stringify({
    emberVersion,
    files: Object.fromEntries(EMBER_PACKAGE_FILES.map((file) => [file, hashFile(path.join(packageDir, file))])),
  }, null, 2)}\n`);
  writeFileSync(path.join(packageDir, 'package.json'), `${JSON.stringify({
    name: EMBER_PACKAGE_NAME,
    version: fcdevVersion,
    description: 'fcdev 随包的 Ember 模拟器（Windows 64 位）：fcdev run、fcdev debug 与调试工具使用',
    license: 'GPL-3.0-only',
    os: ['win32'],
    cpu: ['x64'],
    files: ['bin', 'LICENSE', 'THIRD_PARTY_NOTICES.txt', 'manifest.json'],
  }, null, 2)}\n`);
  return { packageDir, files: EMBER_PACKAGE_FILES };
}

/**
 * 在 ember 目录运行 npm run package:win。
 * @param {string} repoRoot 仓库根目录
 * @throws {FcdevError} 打包失败
 */
function runPackage(repoRoot) {
  const result = spawnSync('npm', ['run', 'package:win'], { cwd: path.join(repoRoot, 'ember'), timeout: PACKAGE_TIMEOUT_MS, shell: true, stdio: 'inherit' });
  if (result.status !== 0) throw new FcdevError('npm run package:win 失败', { code: 'EMBER_PACKAGE_FAILED' });
}

if (process.argv[1] && path.resolve(process.argv[1]) === fileURLToPath(import.meta.url)) {
  try {
    if (process.argv.includes('--package')) runPackage(REPO_ROOT);
    const artifacts = emberArtifacts();
    const fcdevVersion = JSON.parse(readFileSync(artifacts.packageJson, 'utf8')).version;
    const result = assembleEmberPackage({ ...artifacts, emberVersion: artifacts.version, fcdevVersion, packageDir: path.join(REPO_ROOT, 'packages', EMBER_PACKAGE_NAME) });
    console.log(`模拟器包已组装：${result.packageDir}（Ember ${artifacts.version}，fcdev ${fcdevVersion}）`);
  } catch (error) {
    if (!(error instanceof FcdevError)) throw error;
    console.error(`错误：${error.message}`);
    if (error.hint) console.error(error.hint);
    process.exitCode = 1;
  }
}
