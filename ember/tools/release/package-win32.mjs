/*
 * ------------------------------------------------------------------------
 * 名称：package-win32.mjs
 * 说明：打发布包（npm run package:win）：先打预装插件包、构建帮助文档并写出第三方许可全文，再用开启升级的 Release 配置构建 ember-win.exe（资源嵌进去，是单个文件），检查导入表在 Windows 7 SP1 的基线内，
 *       最后在 build/ember/release 下生成 Ember-<版本号>.exe（升级用，原样上传）与 Ember-<版本号>-开发调试模拟器.zip（发给用户：Ember.exe、LICENSE.txt、THIRD_PARTY_NOTICES.txt）。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-21
 * 备注：发布前把 package.json 的 version 升到新版本；帮助文档的构建要用到仓库根目录的依赖与调试 SDK（见 build-help.mjs）；不运行测试，测试用 npm run check。
 *       这个脚本不上传；上传升级文件用 npm run update:publish。
 * ------------------------------------------------------------------------
 */

import { execFileSync } from 'node:child_process';
import { copyFileSync, existsSync, mkdirSync, readFileSync, writeFileSync } from 'node:fs';
import { dirname, join, resolve } from 'node:path';
import { fileURLToPath } from 'node:url';
import { RELEASE_BUILD_DIRECTORY, WIN32_BUILD_DIRECTORY } from '../build/build-paths.mjs';
import { build, configure } from '../build/win32-cmake.mjs';
import { createZip } from '../build/pack-plugins.mjs';

const ROOT = resolve(dirname(fileURLToPath(import.meta.url)), '..', '..');
const NPM = process.platform === 'win32' ? 'npm.cmd' : 'npm';
/** 发给用户的 zip 文件名里的说明，让人一看就知道它是用于开发调试的模拟器。 */
const ZIP_LABEL = '开发调试模拟器';

/** 运行 npm 脚本。 */
function runScript(name) {
  execFileSync(NPM, ['run', name], { cwd: ROOT, stdio: 'inherit', shell: process.platform === 'win32' });
}

try {
  const version = JSON.parse(readFileSync(join(ROOT, 'package.json'), 'utf8')).version;
  runScript('plugins:pack');
  runScript('help:build');
  // 配置前会写出第三方许可全文并把资源打成包；开启升级只在发布配置里做
  configure(['-DEMBER_BUILD_TESTS=OFF', '-DEMBER_ENABLE_UPDATES=ON']);
  build(WIN32_BUILD_DIRECTORY, 'Release', ['ember_win']);
  const exe = join(WIN32_BUILD_DIRECTORY, 'app', 'Release', 'ember-win.exe');
  if (!existsSync(exe)) throw new Error(`找不到构建出的 ${exe}`);
  execFileSync(process.execPath, [join(ROOT, 'tools', 'check', 'check-win32-imports.mjs')], { cwd: ROOT, stdio: 'inherit' });

  mkdirSync(RELEASE_BUILD_DIRECTORY, { recursive: true });
  const updateFile = join(RELEASE_BUILD_DIRECTORY, `Ember-${version}.exe`);
  copyFileSync(exe, updateFile);
  const notices = join(dirname(RELEASE_BUILD_DIRECTORY), 'third-party', 'THIRD_PARTY_NOTICES.txt');
  const zipFile = join(RELEASE_BUILD_DIRECTORY, `Ember-${version}-${ZIP_LABEL}.zip`);
  writeFileSync(
    zipFile,
    createZip([
      { name: 'Ember.exe', data: readFileSync(exe) },
      { name: 'LICENSE.txt', data: readFileSync(join(ROOT, 'LICENSE')) },
      { name: 'THIRD_PARTY_NOTICES.txt', data: readFileSync(notices) }
    ])
  );
  console.log(`发布包：${updateFile}（升级用）\n发布包：${zipFile}（发给用户）`);
} catch (error) {
  console.error(error instanceof Error ? error.message : String(error));
  process.exit(1);
}
