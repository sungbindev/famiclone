// ------------------------------------------------------------------------
// 名称：toolchain-locator.mjs
// 说明：定位 cc65 工具链并启动其中的工具：配置项、环境变量、工具链包，按顺序查找。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-10
// 备注：不使用 PATH 里的 cc65，避免版本不一致；调用工具时设置 CC65_HOME。
// ------------------------------------------------------------------------
import { existsSync } from 'node:fs';
import { createRequire } from 'node:module';
import path from 'node:path';
import { FcdevError, runProcess } from '../shared/index.mjs';

/** 环境变量：自备 cc65 的根目录 */
export const TOOLCHAIN_ENV_NAME = 'FCDEV_CC65_HOME';

/** 工具链里必须存在的程序 */
const REQUIRED_TOOLS = ['cc65', 'ca65', 'ld65'];

/**
 * @typedef {object} Toolchain
 * @property {string} home 工具链根目录
 * @property {string} source 来源：config、env 或 package
 */

/** 工具链包的名称，放在 @rujianjs 作用域下 */
export const TOOLCHAIN_PACKAGE_NAME = '@rujianjs/fcdev-cc65';

/**
 * 工具程序的完整路径。
 * @param {Toolchain} toolchain 工具链
 * @param {string} tool 工具名，如 cc65
 * @returns {string} 可执行文件路径
 */
export function getToolPath(toolchain, tool) {
  return path.join(toolchain.home, 'bin', `${tool}.exe`);
}

/**
 * 检查目录是否是完整的工具链根目录。
 * @param {string} home 候选目录
 * @returns {boolean} 含全部必需程序时为 true
 */
function isToolchainHome(home) {
  return REQUIRED_TOOLS.every((tool) => existsSync(getToolPath({ home, source: '' }, tool)));
}

/**
 * 按顺序定位工具链：配置项 toolchain、环境变量、工具链包。
 * @param {{configured?: string, env?: NodeJS.ProcessEnv}} [options] configured 为配置里的 toolchain 绝对路径
 * @returns {Toolchain} 工具链
 * @throws {FcdevError} 都找不到，或指定的目录不完整
 */
export function locateToolchain({ configured, env = process.env } = {}) {
  const explicit = configured ? { home: configured, source: 'config' } : env[TOOLCHAIN_ENV_NAME] ? { home: path.resolve(env[TOOLCHAIN_ENV_NAME]), source: 'env' } : null;
  if (explicit) {
    if (!isToolchainHome(explicit.home)) {
      throw new FcdevError(`${explicit.home} 不是完整的 cc65 目录（缺少 bin 下的 ${REQUIRED_TOOLS.join('、')}）`, { code: 'TOOLCHAIN_INCOMPLETE' });
    }
    return explicit;
  }
  try {
    const packageJson = createRequire(import.meta.url).resolve(`${TOOLCHAIN_PACKAGE_NAME}/package.json`);
    const home = path.dirname(packageJson);
    if (isToolchainHome(home)) return { home, source: 'package' };
  } catch {
    // 工具链包没有安装，落到下面统一的报错
  }
  throw new FcdevError('找不到 cc65 工具链', {
    code: 'TOOLCHAIN_NOT_FOUND',
    hint: `任选一种：安装工具链包 ${TOOLCHAIN_PACKAGE_NAME}；设置环境变量 ${TOOLCHAIN_ENV_NAME} 指向自备的 cc65 目录；在 fcdev.json 里用 toolchain 指定目录`,
  });
}

/**
 * 运行工具链里的一个工具。
 * @param {Toolchain} toolchain 工具链
 * @param {string} tool 工具名
 * @param {string[]} args 参数
 * @param {{cwd?: string, timeoutMs?: number}} [options] 工作目录与超时
 * @returns {Promise<import('../shared/index.mjs').ProcessResult>} 运行结果
 */
export function runTool(toolchain, tool, args, options = {}) {
  return runProcess(getToolPath(toolchain, tool), args, { ...options, env: { ...process.env, CC65_HOME: toolchain.home } });
}

/**
 * 读取工具链版本，用于增量构建的指纹。
 * @param {Toolchain} toolchain 工具链
 * @returns {Promise<string>} cc65 的版本输出，如 “cc65 V2.19 - Git 357f64e”
 */
export async function getToolchainVersion(toolchain) {
  const result = await runTool(toolchain, 'cc65', ['--version']);
  return (result.stderr || result.stdout).trim();
}
