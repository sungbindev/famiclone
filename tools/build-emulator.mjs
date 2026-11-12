// ------------------------------------------------------------------------
// 名称：build-emulator.mjs
// 说明：编译 virtuanes/ 的 Release 版，并组装成模拟器包 @rujianjs/fcdev-emulator。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-10
// 备注：用法 node tools/build-emulator.mjs；需要 Visual Studio（含“使用 C++ 的桌面开发”）。
//       模拟器是 32 位程序并动态链接 VC++ 运行库，所以把 vcruntime140.dll、msvcp140.dll 一并放进 bin（应用本地部署）。
//       模拟器包随附 GPLv3 许可文件，manifest.json 记录各文件的校验值。
// ------------------------------------------------------------------------
import { createHash } from 'node:crypto';
import { cpSync, existsSync, mkdirSync, readFileSync, readdirSync, rmSync, writeFileSync } from 'node:fs';
import path from 'node:path';
import { fileURLToPath } from 'node:url';
import { FcdevError, runProcess } from '../packages/fcdev/src/shared/index.mjs';

/** 仓库根目录 */
const REPO_ROOT = path.resolve(path.dirname(fileURLToPath(import.meta.url)), '..');

/** 模拟器包目录 */
export const EMULATOR_PACKAGE_DIR = path.join(REPO_ROOT, 'packages', 'fcdev-emulator');

/** 随模拟器放进 bin 的 VC++ 运行库 */
const REDIST_FILES = ['vcruntime140.dll', 'msvcp140.dll'];

/** 编译的超时，单位毫秒 */
const BUILD_TIMEOUT_MS = 20 * 60 * 1000;

/**
 * 计算文件的 SHA-256。
 * @param {string} file 文件路径
 * @returns {string} 十六进制摘要
 */
function hashFile(file) {
  return createHash('sha256').update(readFileSync(file)).digest('hex');
}

/**
 * 在 Visual Studio 安装目录里查找 32 位 VC++ 运行库所在的目录。
 * @returns {Promise<string|null>} 目录；找不到时为 null
 */
async function findRedistDirectory() {
  const vswhere = path.join(process.env['ProgramFiles(x86)'] ?? '', 'Microsoft Visual Studio', 'Installer', 'vswhere.exe');
  if (!existsSync(vswhere)) return null;
  const result = await runProcess(vswhere, ['-latest', '-products', '*', '-property', 'installationPath'], { timeoutMs: 30_000 });
  const installation = result.stdout.trim().split('\n')[0]?.trim();
  if (result.code !== 0 || !installation) return null;
  const redistRoot = path.join(installation, 'VC', 'Redist', 'MSVC');
  if (!existsSync(redistRoot)) return null;
  for (const version of readdirSync(redistRoot).sort().reverse()) {
    const x86 = path.join(redistRoot, version, 'x86');
    if (!existsSync(x86)) continue;
    const crt = readdirSync(x86).find((name) => /^Microsoft\.VC\d+\.CRT$/i.test(name));
    if (crt) return path.join(x86, crt);
  }
  return null;
}

/**
 * 读取仓库当前提交，以及模拟器源码目录是否有未提交的改动。
 * @returns {Promise<{commit: string, dirty: boolean}>} 提交号与是否有未提交改动
 */
async function readSourceState() {
  const head = await runProcess('git', ['-C', REPO_ROOT, 'rev-parse', 'HEAD'], { timeoutMs: 15_000 });
  const status = await runProcess('git', ['-C', REPO_ROOT, 'status', '--porcelain', 'virtuanes'], { timeoutMs: 15_000 });
  return { commit: head.code === 0 ? head.stdout.trim() : 'unknown', dirty: status.stdout.trim() !== '' };
}

/**
 * 编译模拟器并组装模拟器包。
 * @returns {Promise<{packageDir: string, files: number}>} 模拟器包目录与文件数
 * @throws {FcdevError} 编译失败或缺少产物
 */
export async function buildEmulator() {
  const sourceDir = path.join(REPO_ROOT, 'virtuanes');
  const build = await runProcess('powershell', ['-NoProfile', '-ExecutionPolicy', 'Bypass', '-File', path.join(sourceDir, 'build.ps1'), '-Configuration', 'Release'], { cwd: sourceDir, timeoutMs: BUILD_TIMEOUT_MS });
  if (build.code !== 0) {
    throw new FcdevError('编译模拟器失败', { code: 'EMULATOR_BUILD_FAILED', hint: `${build.stdout}${build.stderr}`.trim().split('\n').slice(-15).join('\n') });
  }

  const builtDir = path.join(REPO_ROOT, 'build', 'virtuanes', 'Release', 'bin');
  const binDir = path.join(EMULATOR_PACKAGE_DIR, 'bin');
  rmSync(binDir, { recursive: true, force: true });
  mkdirSync(binDir, { recursive: true });
  for (const name of ['VirtuaNES.exe', 'Chinese.vlp']) {
    if (!existsSync(path.join(builtDir, name))) throw new FcdevError(`没有生成 ${name}`, { code: 'EMULATOR_BUILD_FAILED' });
    cpSync(path.join(builtDir, name), path.join(binDir, name));
  }
  const redist = await findRedistDirectory();
  if (!redist) {
    throw new FcdevError('找不到 VC++ 运行库（vcruntime140.dll、msvcp140.dll）', { code: 'EMULATOR_REDIST_MISSING', hint: '在 Visual Studio 里安装“使用 C++ 的桌面开发”' });
  }
  for (const name of REDIST_FILES) cpSync(path.join(redist, name), path.join(binDir, name));

  cpSync(path.join(sourceDir, 'LICENSE'), path.join(EMULATOR_PACKAGE_DIR, 'LICENSE'));
  const source = await readSourceState();

  const files = readdirSync(binDir).map((name) => path.join(binDir, name));
  writeFileSync(path.join(EMULATOR_PACKAGE_DIR, 'manifest.json'), `${JSON.stringify({
    sourceCommit: source.commit,
    sourceDirty: source.dirty,
    files: Object.fromEntries(files.map((file) => [`bin/${path.basename(file)}`, hashFile(file)])),
  }, null, 2)}\n`);

  const version = JSON.parse(readFileSync(path.join(REPO_ROOT, 'packages', 'fcdev', 'package.json'), 'utf8')).version;
  writeFileSync(path.join(EMULATOR_PACKAGE_DIR, 'package.json'), `${JSON.stringify({
    name: '@rujianjs/fcdev-emulator',
    version,
    description: 'fcdev 使用的调试模拟器（VirtuaNES 调试版，带调试服务）',
    license: 'GPL-3.0-only',
    os: ['win32'],
    cpu: ['x64'],
    files: ['bin', 'LICENSE', 'manifest.json'],
    publishConfig: { access: 'public' },
  }, null, 2)}\n`);
  return { packageDir: EMULATOR_PACKAGE_DIR, files: files.length };
}

if (process.argv[1] && path.resolve(process.argv[1]) === fileURLToPath(import.meta.url)) {
  try {
    const { packageDir, files } = await buildEmulator();
    console.log(`已生成 ${packageDir}（bin 里 ${files} 个文件）`);
  } catch (error) {
    console.error(error.message);
    if (error.hint) console.error(error.hint);
    process.exit(1);
  }
}
