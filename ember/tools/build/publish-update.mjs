/*
 * ------------------------------------------------------------------------
 * 名称：publish-update.mjs
 * 说明：把 Squirrel 生成的升级文件（RELEASES 与 .nupkg）发布到阿里云 OSS 的升级目录：先检查文件齐全、大小与校验值和 RELEASES 一致、当前版本的完整包在其中，再先传 .nupkg、接着传安装程序的 zip、最后传 RELEASES。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-18
 * 备注：先 npm run make:win 生成安装程序（输出在 build/ember/make/squirrel.windows/x64），发布前把 package.json 的 version 升到新版本；
 *       上传用阿里云官方命令行工具 ossutil（要先 ossutil config 配好访问密钥，密钥不放进仓库）；--dry-run 只检查并打印要上传的文件，不上传。
 *       安装程序（.exe）打成 Ember-Setup-<版本号>.zip 一起传，供用户下载安装（浏览器与杀毒软件对 zip 更宽松）；文件名带版本号，新版本不会覆盖以前传的；
 *       zip 放在升级目录里（访问密钥只授权了这个前缀），Squirrel 只读 RELEASES 里列出的文件，多出来的 zip 不影响升级。
 *       最后上传 RELEASES：客户端按它找新版本，必须在 .nupkg 都传好之后才能让它对外可见。存储位置要与主进程 main.ts 的 UPDATE_FEED_URL 一致。
 *       可作为模块被测试引用（parseReleases、planPublish、buildInstallerZip），也可直接运行。
 * ------------------------------------------------------------------------
 */

import { spawnSync } from 'node:child_process';
import { createHash } from 'node:crypto';
import { existsSync, mkdtempSync, readdirSync, readFileSync, rmSync, statSync, writeFileSync } from 'node:fs';
import { tmpdir } from 'node:os';
import { basename, dirname, join, resolve } from 'node:path';
import { fileURLToPath, pathToFileURL } from 'node:url';
import { BUILD_ROOT } from './build-paths.mjs';
import { createZip } from './pack-plugins.mjs';

/** 存放升级文件的 OSS Bucket。 */
const BUCKET = 'famiclone';
/** 升级文件在 Bucket 里的目录，与 main.ts 的 UPDATE_FEED_URL 对应。 */
const KEY_PREFIX = 'updates/ember/windows';
/** Squirrel 包的标识，与 forge.config.mts 里安装程序的 name 一致。 */
const PACKAGE_ID = 'ember';
/** Squirrel 生成的升级文件所在的目录。 */
const RELEASE_DIRECTORY = join(BUILD_ROOT, 'make', 'squirrel.windows', 'x64');
/** RELEASES 里一行：校验值、文件名、大小。 */
const RELEASES_LINE = /^([0-9A-Fa-f]{40})\s+(\S+)\s+(\d+)\s*$/;
/** 文件名只允许的字符，防止 RELEASES 里带路径。 */
const SAFE_FILE_NAME = /^[A-Za-z0-9._-]+$/;
const RELEASES_FILE = 'RELEASES';
/** 安装程序 zip 的文件名前缀，完整名字是 前缀-版本号.zip。 */
const INSTALLER_ZIP_PREFIX = 'Ember-Setup';

/**
 * 解析 RELEASES：每行是“SHA1 文件名 大小”，空行忽略，其他格式的行报错。
 * @param {string} text RELEASES 的内容。
 * @returns {Array<{ sha1: string, file: string, size: number }>} 条目，按文件里的顺序。
 */
export function parseReleases(text) {
  const entries = [];
  for (const line of text.replace(/^\uFEFF/, '').split(/\r?\n/)) {
    if (line.trim() === '') continue;
    const match = RELEASES_LINE.exec(line);
    if (match === null) throw new Error(`RELEASES 里有无法识别的行：${line}`);
    entries.push({ sha1: match[1].toLowerCase(), file: match[2], size: Number(match[3]) });
  }
  return entries;
}

/**
 * 检查要发布的文件并排出上传顺序：.nupkg 在前，给了安装程序时它的 zip 在中间，RELEASES 在最后。
 * @param {{ version: string, releases: string, files: ReadonlyMap<string, { size: number, sha1: string }>, installer?: string }} input 版本号、RELEASES 的内容、目录里各文件的大小与校验值、安装程序（.exe）的文件名（不给就不传 zip）。
 * @returns {{ uploads: Array<{ file: string, key: string, zipOf?: string }> }} 要上传的文件与它们在 Bucket 里的路径；zip 条目的 zipOf 是要打包的安装程序文件名。
 */
export function planPublish({ version, releases, files, installer }) {
  const entries = parseReleases(releases);
  if (entries.length === 0) throw new Error('RELEASES 是空的');
  const uploads = [];
  for (const entry of entries) {
    if (!SAFE_FILE_NAME.test(entry.file) || entry.file === RELEASES_FILE) throw new Error(`RELEASES 里的文件名不合格：${entry.file}`);
    const actual = files.get(entry.file);
    if (actual === undefined) throw new Error(`RELEASES 里列出的 ${entry.file} 不在目录里`);
    if (actual.size !== entry.size) throw new Error(`${entry.file} 的大小与 RELEASES 不一致（文件 ${actual.size}，RELEASES ${entry.size}）`);
    if (actual.sha1.toLowerCase() !== entry.sha1) throw new Error(`${entry.file} 的校验值与 RELEASES 不一致`);
    uploads.push({ file: entry.file, key: `${KEY_PREFIX}/${entry.file}` });
  }
  const full = `${PACKAGE_ID}-${version}-full.nupkg`;
  if (!entries.some((entry) => entry.file === full)) throw new Error(`RELEASES 里没有当前版本的完整包 ${full}：确认 package.json 的 version 与刚生成的安装程序一致`);
  if (installer !== undefined) {
    if (!files.has(installer)) throw new Error(`目录里没有安装程序 ${installer}`);
    const zip = `${INSTALLER_ZIP_PREFIX}-${version}.zip`;
    uploads.push({ file: zip, key: `${KEY_PREFIX}/${zip}`, zipOf: installer });
  }
  uploads.push({ file: RELEASES_FILE, key: `${KEY_PREFIX}/${RELEASES_FILE}` });
  return { uploads };
}

/**
 * 把安装程序打成只含这一个文件的 zip（文件名用 UTF-8，不压缩更小时原样存放）。
 * @param {string} installerPath 安装程序（.exe）的路径。
 * @param {string} zipPath 要写出的 zip 路径。
 */
export function buildInstallerZip(installerPath, zipPath) {
  writeFileSync(zipPath, createZip([{ name: basename(installerPath), data: readFileSync(installerPath) }]));
}

/** 读出目录里每个文件的大小与 SHA1。 */
function describeFiles(directory) {
  const files = new Map();
  for (const name of readdirSync(directory)) {
    const path = join(directory, name);
    if (!statSync(path).isFile()) continue;
    files.set(name, { size: statSync(path).size, sha1: createHash('sha1').update(readFileSync(path)).digest('hex') });
  }
  return files;
}

/** 命令行：检查并上传。 */
function main() {
  const root = resolve(dirname(fileURLToPath(import.meta.url)), '..', '..');
  const dryRun = process.argv.includes('--dry-run');
  const releasesPath = join(RELEASE_DIRECTORY, RELEASES_FILE);
  if (!existsSync(releasesPath)) throw new Error(`找不到 ${releasesPath}：先运行 npm run make:win`);
  const { version } = JSON.parse(readFileSync(join(root, 'package.json'), 'utf8'));
  const files = describeFiles(RELEASE_DIRECTORY);
  const installers = [...files.keys()].filter((name) => name.toLowerCase().endsWith('.exe'));
  if (installers.length !== 1) throw new Error(`${RELEASE_DIRECTORY} 里应该有且只有一个安装程序（.exe），实际有 ${installers.length} 个：先运行 npm run make:win`);
  const { uploads } = planPublish({ version, releases: readFileSync(releasesPath, 'utf8'), files, installer: installers[0] });
  console.log(`版本 ${version}：检查通过，将上传 ${uploads.length} 个文件到 oss://${BUCKET}/${KEY_PREFIX}/`);
  for (const upload of uploads) {
    const target = `oss://${BUCKET}/${upload.key}`;
    console.log(`${dryRun ? '（仅检查）' : '上传'} ${upload.file} -> ${target}${upload.zipOf === undefined ? '' : `（由 ${upload.zipOf} 打包）`}`);
    if (dryRun) continue;
    // zip 先在临时目录里生成，传完就删
    const workDirectory = upload.zipOf === undefined ? null : mkdtempSync(join(tmpdir(), 'ember-installer-zip-'));
    try {
      let source = join(RELEASE_DIRECTORY, upload.file);
      if (workDirectory !== null) {
        source = join(workDirectory, upload.file);
        buildInstallerZip(join(RELEASE_DIRECTORY, upload.zipOf), source);
      }
      const result = spawnSync('ossutil', ['cp', '-f', source, target], { stdio: 'inherit', shell: process.platform === 'win32' });
      if (result.error !== undefined || result.status !== 0) throw new Error(`上传 ${upload.file} 失败：先确认已安装 ossutil 并用 ossutil config 配好访问密钥`);
    } finally {
      if (workDirectory !== null) rmSync(workDirectory, { recursive: true, force: true });
    }
  }
  console.log(dryRun ? '检查完成，没有上传。' : '发布完成。');
}

if (import.meta.url === pathToFileURL(process.argv[1] ?? '').href) {
  try {
    main();
  } catch (error) {
    console.error(error instanceof Error ? error.message : String(error));
    process.exit(1);
  }
}
