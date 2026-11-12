// ------------------------------------------------------------------------
// 名称：release.mjs
// 说明：发布脚本：prepare 升版本号并重新生成两个包的 package.json；publish 做检查、试发布，确认后按顺序发布四个 npm 包。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-12
// 备注：用法 node tools/release.mjs prepare <版本号>；node tools/release.mjs publish [--dry-run] [--allow-dirty]。
//       不暂存、不提交：prepare 之后由使用者自己提交，再运行 publish；publish 需要先 npm login，验证码由 npm 在终端里询问。
//       --dry-run 只做检查与试发布，不登录、不真正发布；--allow-dirty 允许工作区有未提交的改动，只用于演练。
// ------------------------------------------------------------------------
import { spawnSync } from 'node:child_process';
import { readFileSync, writeFileSync } from 'node:fs';
import path from 'node:path';
import { createInterface } from 'node:readline/promises';
import { fileURLToPath } from 'node:url';

const ROOT = path.resolve(path.dirname(fileURLToPath(import.meta.url)), '..');
const PACKAGES_DIR = path.join(ROOT, 'packages');

/** 发布顺序：工具链包与模拟器包先发，主包随后，创建包最后 */
const PUBLISH_ORDER = [
  { directory: 'fcdev-cc65', name: '@rujianjs/fcdev-cc65', scoped: true },
  { directory: 'fcdev-emulator', name: '@rujianjs/fcdev-emulator', scoped: true },
  { directory: 'fcdev', name: 'fcdev', scoped: false },
  { directory: 'create-fcdev', name: 'create-fcdev', scoped: false },
];

/**
 * 在仓库根目录运行命令，输出直接显示在终端里。
 * @param {string} command 命令
 * @param {string[]} args 参数（固定值，不含用户输入）
 * @throws {Error} 命令失败
 */
function run(command, args) {
  console.log(`\n> ${command} ${args.join(' ')}`);
  const result = spawnSync(command, args, { cwd: ROOT, stdio: 'inherit', shell: command === 'npm' });
  if (result.status !== 0) throw new Error(`命令失败：${command} ${args.join(' ')}`);
}

/**
 * 运行命令并取得标准输出，失败时不抛出。
 * @param {string} command 命令
 * @param {string[]} args 参数
 * @returns {{code: number, stdout: string}} 退出码与标准输出
 */
function capture(command, args) {
  const result = spawnSync(command, args, { cwd: ROOT, encoding: 'utf8', shell: command === 'npm', stdio: ['ignore', 'pipe', 'pipe'] });
  return { code: result.status ?? 1, stdout: result.stdout ?? '' };
}

/**
 * 读取包的 package.json。
 * @param {string} directory 包目录名
 * @returns {{file: string, document: any}} 文件路径与内容
 */
function readManifest(directory) {
  const file = path.join(PACKAGES_DIR, directory, 'package.json');
  return { file, document: JSON.parse(readFileSync(file, 'utf8')) };
}

/**
 * 写回 package.json，格式与 npm 一致（两格缩进、末尾换行）。
 * @param {string} file 文件路径
 * @param {any} document 内容
 */
function writeManifest(file, document) {
  writeFileSync(file, `${JSON.stringify(document, null, 2)}\n`);
}

/**
 * 升版本号：主包与创建包的 version，以及主包对两个包的精确依赖；再重新生成两个包的 package.json 与 npm 的锁文件。
 * @param {string} version 新版本号
 * @throws {Error} 版本号格式不对或与当前相同
 */
function prepare(version) {
  if (!/^\d+\.\d+\.\d+$/.test(version ?? '')) throw new Error('用法：node tools/release.mjs prepare <版本号>，如 0.4.0');
  const main = readManifest('fcdev');
  if (main.document.version === version) throw new Error(`版本号没有变化：${version}`);

  main.document.version = version;
  for (const name of Object.keys(main.document.optionalDependencies ?? {})) main.document.optionalDependencies[name] = version;
  writeManifest(main.file, main.document);
  const creator = readManifest('create-fcdev');
  creator.document.version = version;
  writeManifest(creator.file, creator.document);

  run(process.execPath, ['tools/build-cc65.mjs']);
  run(process.execPath, ['tools/build-emulator.mjs']);
  run('npm', ['install', '--no-audit', '--no-fund']);
  console.log(`\n版本号已升到 ${version}。请检查并提交改动，然后运行：node tools/release.mjs publish`);
}

/**
 * 确认工作区没有未提交的改动。
 * @param {boolean} allowDirty 为 true 时只警告
 * @throws {Error} 有未提交的改动且不允许
 */
function checkClean(allowDirty) {
  const status = capture('git', ['status', '--porcelain']).stdout.trim();
  if (status === '') return;
  const message = `工作区有未提交的改动：\n${status}`;
  if (!allowDirty) throw new Error(`${message}\n先提交，或加 --allow-dirty 只做演练`);
  console.warn(`警告：${message}`);
}

/**
 * 某个版本是否已经发布到 npm。
 * @param {string} name 包名
 * @param {string} version 版本号
 * @returns {boolean} 已发布时为 true
 */
function isPublished(name, version) {
  const result = capture('npm', ['view', `${name}@${version}`, 'version']);
  return result.code === 0 && result.stdout.trim() !== '';
}

/**
 * 询问用户是否继续。
 * @param {string} question 提示
 * @returns {Promise<boolean>} 输入 yes 时为 true
 */
async function confirm(question) {
  const rl = createInterface({ input: process.stdin, output: process.stdout });
  try {
    return (await rl.question(`${question}（输入 yes 继续）：`)).trim().toLowerCase() === 'yes';
  } finally {
    rl.close();
  }
}

/**
 * 检查、试发布，确认后按顺序发布。
 * @param {{dryRun: boolean, allowDirty: boolean}} options 选项
 * @throws {Error} 任一步失败
 */
async function publish({ dryRun, allowDirty }) {
  checkClean(allowDirty);
  const version = readManifest('fcdev').document.version;
  console.log(`准备发布版本 ${version}${dryRun ? '（演练，不会真正发布）' : ''}`);

  if (!dryRun) {
    const who = capture('npm', ['whoami']);
    if (who.code !== 0) throw new Error('npm 没有登录，先运行 npm login');
    console.log(`npm 登录用户：${who.stdout.trim()}`);
  }

  const published = new Set();
  for (const item of PUBLISH_ORDER) {
    if (!isPublished(item.name, version)) continue;
    const message = `${item.name}@${version} 已经发布过，先运行 node tools/release.mjs prepare <新版本号>`;
    if (!dryRun) throw new Error(message);
    console.warn(`警告：${message}，演练时跳过它的试发布`);
    published.add(item.name);
  }

  run('npm', ['test']);
  run(process.execPath, ['tools/build-emulator.mjs']);
  run('npm', ['run', 'release:check', ...(allowDirty ? ['--', '--allow-dirty'] : [])]);
  run('npm', ['run', 'release:verify']);
  for (const item of PUBLISH_ORDER.filter((entry) => !published.has(entry.name))) run('npm', ['publish', '-w', `packages/${item.directory}`, '--dry-run']);

  if (dryRun) {
    console.log('\n演练完成，没有发布任何包。');
    return;
  }
  if (!(await confirm(`\n将按顺序发布 ${PUBLISH_ORDER.map((item) => item.name).join('、')} 的 ${version} 版`))) {
    console.log('已取消，没有发布任何包。');
    return;
  }
  for (const item of PUBLISH_ORDER) {
    run('npm', ['publish', '-w', `packages/${item.directory}`, ...(item.scoped ? ['--access', 'public'] : [])]);
  }
  for (const item of PUBLISH_ORDER) run('npm', ['view', `${item.name}@${version}`, 'version']);
  console.log(`\n四个包的 ${version} 版都已发布。`);
}

const [command, ...rest] = process.argv.slice(2);
try {
  if (command === 'prepare') prepare(rest[0]);
  else if (command === 'publish') await publish({ dryRun: rest.includes('--dry-run'), allowDirty: rest.includes('--allow-dirty') });
  else throw new Error('用法：node tools/release.mjs prepare <版本号> | publish [--dry-run] [--allow-dirty]');
} catch (error) {
  console.error(`\n错误：${error.message}`);
  process.exitCode = 1;
}
