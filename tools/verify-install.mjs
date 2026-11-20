// ------------------------------------------------------------------------
// 名称：verify-install.mjs
// 说明：安装验证：把四个包打成 tgz，在临时目录里走一遍“创建项目、安装、构建”，确认发布出去的包能用；再单独安装主包，检查随包的调试 SDK 与随包的 Ember。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-10
// 备注：用法 node tools/verify-install.mjs [--keep]；
//       不访问 npm 仓库：创建包与三个依赖包（主包、工具链包、模拟器包）都从本地 tgz 安装；不执行 npm publish；--keep 保留临时目录便于查看。
//       只验证构建，不启动模拟器。
// ------------------------------------------------------------------------
import { spawnSync } from 'node:child_process';
import { existsSync, mkdirSync, mkdtempSync, readFileSync, readdirSync, rmSync } from 'node:fs';
import { tmpdir } from 'node:os';
import path from 'node:path';
import { fileURLToPath } from 'node:url';

const ROOT = path.resolve(path.dirname(fileURLToPath(import.meta.url)), '..');
const PACKAGE_NAMES = ['fcdev', 'create-fcdev', 'fcdev-cc65', 'fcdev-ember'];
const keep = process.argv.includes('--keep');

/**
 * 运行命令，失败时抛出带输出的错误。
 * @param {string} command 命令
 * @param {string[]} args 参数
 * @param {string} cwd 工作目录
 * @returns {string} 标准输出
 */
function run(command, args, cwd) {
  const result = spawnSync(command, args, { cwd, encoding: 'utf8', shell: command === 'npm' });
  if (result.status !== 0) {
    throw new Error(`命令失败：${command} ${args.join(' ')}\n${result.stdout}\n${result.stderr}`);
  }
  return result.stdout;
}

/**
 * 在目录里找到以指定前缀开头的 tgz 文件。
 * @param {string} dir 目录
 * @param {string} prefix 文件名前缀（包名加连字符）
 * @returns {string} tgz 的绝对路径
 */
function findTarball(dir, prefix) {
  const name = readdirSync(dir).find((file) => file.startsWith(prefix) && file.endsWith('.tgz'));
  if (!name) throw new Error(`没有找到 ${prefix}*.tgz`);
  return path.join(dir, name);
}

/**
 * 断言条件成立。
 * @param {boolean} condition 条件
 * @param {string} message 不成立时的说明
 */
function assert(condition, message) {
  if (!condition) throw new Error(message);
  console.log(`通过：${message.replace(/^应当/, '')}`);
}

/**
 * 创建一个项目，安装本地包，构建并检查产物。
 * @param {string} workDir 临时目录
 * @param {string} creatorBin create-fcdev 的入口脚本
 * @param {string[]} tarballs 要安装的三个包：主包、工具链包与模拟器包（主包依赖后两者，不访问 npm 仓库时需要一并给出）
 * @param {string} target 目标名
 * @param {string} language 语言
 * @param {(file: Buffer) => string|null} inspect 检查产物，返回错误说明或 null
 * @param {string} extension 产物扩展名
 */
function verifyProject(workDir, creatorBin, tarballs, target, language, inspect, extension) {
  const appDir = path.join(workDir, `app-${target}-${language}`);
  run('node', [creatorBin, appDir, '--target', target, '--template', language, '--no-install'], workDir);
  run('npm', ['install', '--save-dev', '--no-audit', '--no-fund', ...tarballs.map((file) => `"${file}"`)], appDir);
  run('npm', ['run', 'build'], appDir);
  const outDir = path.join(appDir, 'build', target);
  const output = existsSync(outDir) ? readdirSync(outDir).find((file) => file.toLowerCase().endsWith(extension)) : undefined;
  assert(output !== undefined, `应当生成 ${target}/${language} 的 ${extension} 产物`);
  const problem = inspect(readFileSync(path.join(outDir, output)));
  assert(problem === null, `应当 ${target}/${language} 的产物格式正确${problem ? `：${problem}` : ''}`);
}

/**
 * 在临时目录里安装主包，确认随包的调试 SDK 两个入口能加载、命令行入口文件在，且 fcdev debug 能运行终端客户端；再确认随包的 Ember 能装到用户级目录（用临时目录，不碰真实的 %LOCALAPPDATA%）。
 * @param {string} workDir 临时目录
 * @param {string[]} tarballs 主包、工具链包与模拟器包的 tgz 路径
 */
function verifyDebugSdk(workDir, tarballs) {
  const sdkDir = path.join(workDir, 'sdk-check');
  mkdirSync(sdkDir);
  run('npm', ['install', '--prefix', `"${sdkDir}"`, '--no-audit', '--no-fund', ...tarballs.map((file) => `"${file}"`)], workDir);
  const script = "const sdk = await import('fcdev/dist/debug/index.js'); const node = await import('fcdev/dist/debug/node.js'); if (typeof sdk.connect !== 'function' || typeof node.connectToEmber !== 'function') throw new Error('missing export');";
  run('node', ['--input-type=module', '-e', script], sdkDir);
  assert(true, '应当调试 SDK 的两个入口都能加载');
  for (const file of ['cli.js', 'mcp.js']) {
    assert(existsSync(path.join(sdkDir, 'node_modules', 'fcdev', 'dist', 'debug', file)), `应当安装出调试 SDK 的 dist/debug/${file}`);
  }
  const version = run('node', [path.join(sdkDir, 'node_modules', 'fcdev', 'bin', 'fcdev.mjs'), 'debug', '--version'], sdkDir).trim();
  assert(/^\d+\.\d+\.\d+$/.test(version), '应当 fcdev debug --version 能经终端客户端输出版本号');

  const home = path.join(workDir, 'ember-home');
  const locateScript = `const { locateEmulator } = await import('fcdev/src/emulator/index.mjs'); const found = locateEmulator({ env: { FCDEV_EMBER_HOME: ${JSON.stringify(home)} }, platform: 'win32' }); console.log(found.executable);`;
  const located = run('node', ['--input-type=module', '-e', locateScript], sdkDir).trim();
  assert(existsSync(located) && path.dirname(located) === home, '应当随包的 Ember 装到用户级目录并能被 fcdev 找到');
}

const workDir = mkdtempSync(path.join(tmpdir(), 'fcdev-verify-'));
let failed = false;
try {
  const packDir = path.join(workDir, 'packs');
  mkdirSync(packDir);
  for (const name of PACKAGE_NAMES) run('npm', ['pack', '-w', `packages/${name}`, '--pack-destination', `"${packDir}"`], ROOT);
  const creatorDir = path.join(workDir, 'creator');
  mkdirSync(creatorDir);
  run('npm', ['install', '--prefix', `"${creatorDir}"`, '--no-audit', '--no-fund', `"${findTarball(packDir, 'create-fcdev-')}"`], workDir);
  const creatorBin = path.join(creatorDir, 'node_modules', 'create-fcdev', 'bin', 'create-fcdev.mjs');
  const tarballs = [findTarball(packDir, 'fcdev-0'), findTarball(packDir, 'fcdev-cc65-'), findTarball(packDir, 'fcdev-ember-')];

  const inspectNes = (file) => (file.subarray(0, 4).equals(Buffer.from([0x4e, 0x45, 0x53, 0x1a])) ? null : '缺少 iNES 文件头');
  const inspectSb2000 = (file) => (file[0] === 0x4c && file[3] === 0xaa && file[4] === 0x55 ? null : '文件头不是 4C ?? ?? AA 55');
  verifyProject(workDir, creatorBin, tarballs, 'nes', 'c', inspectNes, '.nes');
  verifyProject(workDir, creatorBin, tarballs, 'nes', 'asm', inspectNes, '.nes');
  verifyProject(workDir, creatorBin, tarballs, 'sb2000', 'c', inspectSb2000, '.exe');
  verifyProject(workDir, creatorBin, tarballs, 'sb2000', 'asm', inspectSb2000, '.exe');
  verifyDebugSdk(workDir, tarballs);
  console.log('安装验证通过');
} catch (error) {
  failed = true;
  console.error(`安装验证失败：${error.message}`);
} finally {
  if (keep) console.log(`临时目录保留在 ${workDir}`);
  else rmSync(workDir, { recursive: true, force: true });
}
process.exitCode = failed ? 1 : 0;
