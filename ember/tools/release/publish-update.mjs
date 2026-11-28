/*
 * ------------------------------------------------------------------------
 * 名称：publish-update.mjs
 * 说明：发布升级文件（npm run update:publish）：用私钥给 build/ember/release/Ember-<版本号>.exe 生成签名清单 latest.json，再复制程序文件、下载用的 Ember-<版本号>.zip，最后复制 latest.json 到仓库的 updates/ember/windows/；复制完成后自动构建站点（site 的 npm run build，首页下载地址指向刚复制的压缩包）。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-21
 * 备注：先 npm run package:win 打好发布包，并把 package.json 的 version 升到新版本；复制之后要自己提交并推送，推送后由 fcdev.cn 的 GitHub Pages 对外，程序里的升级地址是 https://fcdev.cn/updates/ember/windows；--dry-run 只检查并写出 latest.json，不写仓库；
 *       客户端按 latest.json 找新版本，必须在程序文件复制好之后才让它对外可见，所以最后复制它；程序文件名带版本号，新版本不会覆盖以前发布的。
 *       发布前会核对：程序里编译进去的公钥必须就是这把私钥的公钥，否则发布出去的升级没有一个已装程序能接受；签名生成后还会用公钥再校验一遍。
 *       可作为模块被测试引用（buildLatest、planFeedFiles、publicKeyMatches），也可直接运行。
 * ------------------------------------------------------------------------
 */

import { execFileSync } from 'node:child_process';
import { createPublicKey, createVerify } from 'node:crypto';
import { copyFileSync, existsSync, mkdirSync, readFileSync, writeFileSync } from 'node:fs';
import { dirname, join, resolve } from 'node:path';
import { fileURLToPath, pathToFileURL } from 'node:url';
import { RELEASE_BUILD_DIRECTORY } from '../build/build-paths.mjs';
import { PUBLIC_KEY_HEADER, buildManifest, formatPublicKeyHeader, loadPrivateKey, publicKeyParts, signedMessage } from './update-signing.mjs';

/** 升级文件在仓库里的目录（相对仓库根目录），与程序里的升级地址 https://fcdev.cn/updates/ember/windows 对应。 */
const FEED_DIRECTORY = ['updates', 'ember', 'windows'];
/** 清单文件名。 */
export const LATEST_FILE = 'latest.json';

/**
 * 生成清单并签名，写成 latest.json 的内容。
 * @param {import('node:crypto').KeyObject} privateKey 签名私钥。
 * @param {string} version 版本号。
 * @param {Buffer} content 程序文件的内容。
 */
export function buildLatest(privateKey, version, content) {
  if (!/^\d+\.\d+\.\d+$/.test(version)) throw new Error(`版本号必须是“主.次.修订”：${version}`);
  const manifest = buildManifest(privateKey, { version, content });
  const verified = createVerify('sha256').update(signedMessage(manifest)).verify(createPublicKey(privateKey), Buffer.from(manifest.signature, 'base64'));
  if (!verified) throw new Error('签名生成后用公钥校验没有通过');
  return `${JSON.stringify(manifest, null, 2)}\n`;
}

/** 要复制到升级目录的文件名：程序文件与下载用的压缩包在前，清单在最后。 */
export function planFeedFiles(version) {
  return [`Ember-${version}.exe`, `Ember-${version}.zip`, LATEST_FILE];
}

/** 程序里编译进去的公钥头文件是否就是这把私钥的公钥。 */
export function publicKeyMatches(privateKey, headerText) {
  return headerText.replaceAll('\r\n', '\n') === formatPublicKeyHeader(publicKeyParts(privateKey));
}

/** 命令行：检查、签名并复制到升级目录。 */
function main() {
  const root = resolve(dirname(fileURLToPath(import.meta.url)), '..', '..');
  const dryRun = process.argv.includes('--dry-run');
  const { version } = JSON.parse(readFileSync(join(root, 'package.json'), 'utf8'));
  const exe = join(RELEASE_BUILD_DIRECTORY, `Ember-${version}.exe`);
  if (!existsSync(exe)) throw new Error(`找不到 ${exe}：先运行 npm run package:win（并确认 package.json 的 version 就是要发布的版本）`);
  const zip = join(RELEASE_BUILD_DIRECTORY, `Ember-${version}.zip`);
  if (!existsSync(zip)) throw new Error(`找不到 ${zip}：先运行 npm run package:win`);
  const privateKey = loadPrivateKey();
  if (!publicKeyMatches(privateKey, readFileSync(PUBLIC_KEY_HEADER, 'utf8'))) {
    throw new Error(`${PUBLIC_KEY_HEADER} 里的公钥不是这把私钥的公钥：重新运行 npm run update:keygen -- --export-public 并重新打包`);
  }
  const latest = join(RELEASE_BUILD_DIRECTORY, LATEST_FILE);
  writeFileSync(latest, buildLatest(privateKey, version, readFileSync(exe)));
  const files = planFeedFiles(version);
  const feedDirectory = join(root, '..', ...FEED_DIRECTORY);
  console.log(`版本 ${version}：签名清单已写到 ${latest}，将复制 ${files.length} 个文件到 ${feedDirectory}`);
  if (!dryRun) mkdirSync(feedDirectory, { recursive: true });
  for (const file of files) {
    console.log(`${dryRun ? '（仅检查）' : '复制'} ${file}`);
    if (!dryRun) copyFileSync(join(RELEASE_BUILD_DIRECTORY, file), join(feedDirectory, file));
  }
  if (!dryRun) {
    // 升级文件就位后接着构建站点：首页的下载地址指向刚复制的压缩包，不用再单独构建
    console.log('构建站点…');
    execFileSync(process.platform === 'win32' ? 'npm.cmd' : 'npm', ['run', 'build'], { cwd: join(root, '..', 'site'), stdio: 'inherit', shell: process.platform === 'win32' });
  }
  console.log(dryRun ? '检查完成，没有写仓库。' : '发布文件已写入仓库，站点已构建：提交并推送后才会在 fcdev.cn 上生效。');
}

if (import.meta.url === pathToFileURL(process.argv[1] ?? '').href) {
  try {
    main();
  } catch (error) {
    console.error(error instanceof Error ? error.message : String(error));
    process.exit(1);
  }
}
