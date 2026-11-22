// ------------------------------------------------------------------------
// 名称：release.mjs
// 说明：发布 fcdev 扩展：检查版本与更新记录、运行全部检查、打包并核对 .vsix，加 --publish 才上传到扩展市场。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-22
// 备注：用法 npm run release [-- --publish]（在 vscode/ 里）；不带参数只检查与打包。
//       上传需要环境变量 VSCE_PAT，或事先执行 vsce login sungbin；凭据不写进脚本。
// ------------------------------------------------------------------------
import { spawnSync } from 'node:child_process';
import { existsSync, readFileSync } from 'node:fs';
import path from 'node:path';
import { fileURLToPath } from 'node:url';

/** 仓库根目录 */
const REPOSITORY_ROOT = path.resolve(path.dirname(fileURLToPath(import.meta.url)), '..', '..', '..');

/** 扩展目录 */
const EXTENSION_DIRECTORY = path.join(REPOSITORY_ROOT, 'vscode');

/** 打包输出目录，与扩展的 package 脚本一致 */
const OUTPUT_DIRECTORY = path.join(REPOSITORY_ROOT, 'build', 'vscode');

/**
 * 在扩展目录里运行一条命令，输出直接显示在终端里，失败时抛出错误。
 * @param {string} command 命令名。
 * @param {string[]} args 参数。
 */
function run(command, args) {
  const result = spawnSync(command, args, { cwd: EXTENSION_DIRECTORY, stdio: 'inherit', shell: command !== process.execPath && process.platform === 'win32' });
  if (result.error !== undefined || result.status !== 0) {
    throw new Error(`${[command, ...args].join(' ')} 失败（${result.error?.message ?? `退出码 ${result.status}`}）`);
  }
}

/**
 * 列出仓库里还没有提交的改动。
 * @returns {string[]} 每行一条 git status 的输出。
 */
function uncommittedChanges() {
  const result = spawnSync('git', ['status', '--short'], { cwd: REPOSITORY_ROOT, encoding: 'utf8' });
  if (result.status !== 0) throw new Error('无法读取 git 状态，请确认在仓库里运行');
  return result.stdout.split('\n').filter((line) => line.trim() !== '');
}

/** 命令行入口 */
function main() {
  const publish = process.argv.includes('--publish');
  const { version } = JSON.parse(readFileSync(path.join(EXTENSION_DIRECTORY, 'package.json'), 'utf8'));
  console.log(`发布检查：fcdev ${version}`);

  const changes = uncommittedChanges();
  if (changes.length > 0) {
    const message = `仓库里还有未提交的改动：\n${changes.join('\n')}`;
    if (publish) throw new Error(`${message}\n先提交全部改动再发布`);
    console.warn(`警告：${message}`);
  }

  const changelog = readFileSync(path.join(EXTENSION_DIRECTORY, 'CHANGELOG.md'), 'utf8').split(/\r?\n/);
  if (!changelog.includes(`## ${version}`)) throw new Error(`CHANGELOG.md 里没有 “## ${version}” 一节`);

  run('npm', ['run', 'check']);
  run('npm', ['run', 'package']);

  const vsix = path.join(OUTPUT_DIRECTORY, `fcdev-win32-x64-${version}.vsix`);
  if (!existsSync(vsix)) throw new Error(`找不到 ${vsix}`);
  run(process.execPath, [path.join(EXTENSION_DIRECTORY, 'tools', 'check', 'check-package.mjs'), vsix]);

  if (!publish) {
    console.log(`已打包并通过核对：${vsix}\n确认后执行 npm run release -- --publish 上传。`);
    return;
  }
  run('npx', ['vsce', 'publish', '--packagePath', vsix]);
  console.log(`已发布 fcdev ${version}`);
}

try {
  main();
} catch (error) {
  console.error(error instanceof Error ? error.message : error);
  process.exit(1);
}
