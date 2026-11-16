/*
 * ------------------------------------------------------------------------
 * 名称：build-core.mjs
 * 说明：编译 C++ 原生模块：先生成 Schema 代码并打包官方插件，再用 cmake-js 按当前 Electron 版本编译，输出 build/ember/core/Release/ember_core.node。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-13
 * 备注：参数 --arch x64|arm64 指定目标架构，默认当前架构；核心版本取自 package.json。
 * ------------------------------------------------------------------------
 */

import { execFileSync } from 'node:child_process';
import { readFileSync } from 'node:fs';
import { createRequire } from 'node:module';
import { dirname, join, resolve } from 'node:path';
import { fileURLToPath } from 'node:url';
import { generate } from '../schema-gen/schema-gen.mjs';
import { findCmake } from './find-cmake.mjs';
import { CORE_BUILD_DIRECTORY } from './build-paths.mjs';
import { packPlugins } from './pack-plugins.mjs';

const root = resolve(dirname(fileURLToPath(import.meta.url)), '..', '..');
const require = createRequire(import.meta.url);

/** 读取命令行参数 --arch。 */
function readArch() {
  const index = process.argv.indexOf('--arch');
  const arch = index >= 0 ? process.argv[index + 1] : process.arch;
  if (arch !== 'x64' && arch !== 'arm64') throw new Error(`不支持的架构：${arch}，只能是 x64 或 arm64`);
  return arch;
}

try {
  const arch = readArch();
  const version = JSON.parse(readFileSync(join(root, 'package.json'), 'utf8')).version;
  const electronVersion = JSON.parse(readFileSync(require.resolve('electron/package.json'), 'utf8')).version;
  const cmakeJs = join(dirname(require.resolve('cmake-js/package.json')), 'bin', 'cmake-js');

  generate(root);
  // 开发版预装的官方插件随核心一起打包，输出在 build/ember/plugins。
  packPlugins(root);
  const { cmake } = findCmake();
  execFileSync(
    process.execPath,
    [cmakeJs, 'compile', '--directory', 'core', '--out', CORE_BUILD_DIRECTORY, '--cmake-path', cmake, '--runtime', 'electron', '--runtime-version', electronVersion, '--arch', arch, '--config', 'Release', `--CDEMBER_CORE_VERSION=${version}`],
    { cwd: root, stdio: 'inherit' }
  );
} catch (error) {
  console.error(error instanceof Error ? error.message : String(error));
  process.exit(1);
}
