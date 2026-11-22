// ------------------------------------------------------------------------
// 名称：build-cc65.mjs
// 说明：用 vscode/third_party/cc65 的源码构建 cc65 工具与 none、nes 运行库，并组装到扩展目录的 toolchain。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-10
// 备注：用法 node vscode/tools/build/build-cc65.mjs；用 mingw32-make 与 gcc 构建，环境变量 MAKE_PROGRAM 可指定 make 程序。
//       构建在 build/vscode/cc65 下进行，不污染源码目录；产物在 vscode/toolchain，随 .vsix 发布，不入库。
// ------------------------------------------------------------------------
import { spawn } from 'node:child_process';
import { createHash } from 'node:crypto';
import { cpSync, existsSync, mkdirSync, readdirSync, readFileSync, rmSync, writeFileSync } from 'node:fs';
import { cpus } from 'node:os';
import path from 'node:path';
import { fileURLToPath } from 'node:url';

/** 构建失败的错误，带下一步建议。 */
class Cc65BuildError extends Error {
  /**
   * @param {string} message 说明
   * @param {{code?: string, hint?: string}} [options] 错误码与建议
   */
  constructor(message, { code = 'CC65_BUILD_FAILED', hint = '' } = {}) {
    super(message);
    this.code = code;
    this.hint = hint;
  }
}

/**
 * 运行外部程序并收集输出。
 * @param {string} file 可执行文件
 * @param {string[]} args 参数
 * @param {{cwd: string, timeoutMs: number}} options 工作目录与超时
 * @returns {Promise<{code: number, stdout: string, stderr: string}>} 运行结果
 */
function runProcess(file, args, { cwd, timeoutMs }) {
  return new Promise((resolve, reject) => {
    const child = spawn(file, args, { cwd, windowsHide: true, stdio: ['ignore', 'pipe', 'pipe'] });
    const stdout = [];
    const stderr = [];
    const timer = setTimeout(() => {
      child.kill();
      reject(new Cc65BuildError(`${file} 运行超过 ${timeoutMs} 毫秒，已终止`));
    }, timeoutMs);
    child.stdout.on('data', (chunk) => stdout.push(chunk));
    child.stderr.on('data', (chunk) => stderr.push(chunk));
    child.on('error', (error) => {
      clearTimeout(timer);
      reject(new Cc65BuildError(`无法启动 ${file}：${error.message}`));
    });
    child.on('close', (code) => {
      clearTimeout(timer);
      resolve({ code: code ?? -1, stdout: Buffer.concat(stdout).toString('utf8'), stderr: Buffer.concat(stderr).toString('utf8') });
    });
  });
}

/** vscode/third_party/cc65 最初导入自的上游提交，写进工具链包清单并作为 cc65 的版本串 */
export const CC65_COMMIT = '357f64e4e3c733d54b82bc155afc841fd8d9ee8d';

/** 上游仓库地址 */
export const CC65_UPSTREAM = 'https://github.com/cc65/cc65';

/** 仓库根目录 */
const REPO_ROOT = path.resolve(path.dirname(fileURLToPath(import.meta.url)), '..', '..', '..');

/** 工具链包里的可执行程序 */
export const TOOLCHAIN_TOOLS = ['ar65', 'ca65', 'cc65', 'da65', 'ld65'];

/** 构建的运行库目标 */
export const LIBRARY_TARGETS = ['none', 'nes'];

/** 工具链目录里放入的目录 */
const PACKAGE_CONTENT_DIRS = ['bin', 'include', 'asminc', 'lib', 'cfg'];

/** 单次构建命令的超时，单位毫秒 */
const BUILD_TIMEOUT_MS = 15 * 60 * 1000;

/**
 * 计算文件的 SHA-256。
 * @param {string} file 文件路径
 * @returns {string} 十六进制摘要
 */
function hashFile(file) {
  return createHash('sha256').update(readFileSync(file)).digest('hex');
}

/**
 * 在目录下递归列出文件。
 * @param {string} dir 目录
 * @returns {string[]} 文件的绝对路径
 */
function listFiles(dir) {
  return readdirSync(dir, { withFileTypes: true }).flatMap((entry) => (entry.isDirectory() ? listFiles(path.join(dir, entry.name)) : [path.join(dir, entry.name)]));
}

/**
 * 运行 make，失败时带上输出的末尾。
 * @param {string} make make 程序
 * @param {string[]} args 参数
 * @param {string} cwd 工作目录
 * @returns {Promise<void>}
 * @throws {Cc65BuildError} make 失败
 */
async function runMake(make, args, cwd) {
  const result = await runProcess(make, args, { cwd, timeoutMs: BUILD_TIMEOUT_MS });
  if (result.code !== 0) {
    const tail = `${result.stdout}${result.stderr}`.trim().split('\n').slice(-15).join('\n');
    throw new Cc65BuildError(`${make} ${args.join(' ')} 失败`, { hint: tail });
  }
}

/**
 * 构建并组装工具链。
 * @param {{make?: string, workDir?: string}} [options] make 程序与工作目录
 * @returns {Promise<{packageDir: string, files: number}>} 工具链目录与文件数
 * @throws {Cc65BuildError} 构建失败，或缺少预期的产物
 */
export async function buildCc65(options = {}) {
  const make = options.make ?? process.env.MAKE_PROGRAM ?? 'mingw32-make';
  const workDir = options.workDir ?? path.join(REPO_ROOT, 'build', 'vscode', 'cc65');
  const sourceDir = path.join(REPO_ROOT, 'vscode', 'third_party', 'cc65');
  const packageDir = path.join(REPO_ROOT, 'vscode', 'toolchain');
  const jobs = String(Math.max(1, cpus().length));
  // 不指定时 Makefile 会用 git 取当前目录所在仓库（即 fcdev）的提交号，版本输出会指向错误的提交
  const buildId = `BUILD_ID=Git ${CC65_COMMIT.slice(0, 7)}`;

  rmSync(workDir, { recursive: true, force: true });
  cpSync(sourceDir, workDir, { recursive: true });
  await runMake(make, ['-C', 'src', '-j', jobs, buildId, `PROGS=${TOOLCHAIN_TOOLS.join(' ')}`], workDir);
  await runMake(make, ['-C', 'libsrc', '-j', jobs, `TARGETS=${LIBRARY_TARGETS.join(' ')}`], workDir);

  mkdirSync(packageDir, { recursive: true });
  for (const dir of PACKAGE_CONTENT_DIRS) rmSync(path.join(packageDir, dir), { recursive: true, force: true });
  mkdirSync(path.join(packageDir, 'bin'), { recursive: true });
  for (const tool of TOOLCHAIN_TOOLS) {
    const built = path.join(workDir, 'bin', `${tool}.exe`);
    if (!existsSync(built)) throw new Cc65BuildError(`没有生成 ${tool}.exe`);
    cpSync(built, path.join(packageDir, 'bin', `${tool}.exe`));
  }
  cpSync(path.join(sourceDir, 'include'), path.join(packageDir, 'include'), { recursive: true });
  cpSync(path.join(sourceDir, 'asminc'), path.join(packageDir, 'asminc'), { recursive: true });
  cpSync(path.join(sourceDir, 'cfg'), path.join(packageDir, 'cfg'), { recursive: true });
  mkdirSync(path.join(packageDir, 'lib'), { recursive: true });
  for (const target of LIBRARY_TARGETS) {
    const built = path.join(workDir, 'lib', `${target}.lib`);
    if (!existsSync(built)) throw new Cc65BuildError(`没有生成 ${target}.lib`);
    cpSync(built, path.join(packageDir, 'lib', `${target}.lib`));
  }
  cpSync(path.join(sourceDir, 'LICENSE'), path.join(packageDir, 'LICENSE'));

  const files = listFiles(packageDir).filter((file) => path.basename(file) !== 'manifest.json');
  writeFileSync(path.join(packageDir, 'manifest.json'), `${JSON.stringify({
    upstream: CC65_UPSTREAM,
    commit: CC65_COMMIT,
    files: Object.fromEntries(files.map((file) => [path.relative(packageDir, file).split(path.sep).join('/'), hashFile(file)])),
  }, null, 2)}\n`);
  return { packageDir, files: files.length };
}

if (process.argv[1] && path.resolve(process.argv[1]) === fileURLToPath(import.meta.url)) {
  try {
    const { packageDir, files } = await buildCc65();
    console.log(`已生成 ${packageDir}（${files} 个文件）`);
  } catch (error) {
    console.error(error.message);
    if (error.hint) console.error(error.hint);
    process.exit(1);
  }
}
