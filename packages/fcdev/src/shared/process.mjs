// ------------------------------------------------------------------------
// 名称：process.mjs
// 说明：启动外部程序并收集输出：不经过 shell，参数逐个传递，带超时。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-10
// 备注：编译、链接这类工具的输出原样保留，供上层透传给用户或问题匹配器。
// ------------------------------------------------------------------------
import { spawn } from 'node:child_process';
import { FcdevError } from './errors.mjs';

/** 外部工具默认的超时时间，单位毫秒 */
export const DEFAULT_TOOL_TIMEOUT_MS = 120_000;

/**
 * @typedef {object} ProcessResult
 * @property {number} code 退出码
 * @property {string} stdout 标准输出
 * @property {string} stderr 标准错误
 */

/**
 * 运行外部程序并等待结束。
 * @param {string} file 可执行文件路径
 * @param {string[]} args 参数列表
 * @param {{cwd?: string, env?: NodeJS.ProcessEnv, timeoutMs?: number}} [options] 工作目录、环境变量与超时
 * @returns {Promise<ProcessResult>} 程序以非零退出码结束时不抛错，由调用者判断
 * @throws {FcdevError} 程序无法启动或超时
 */
export function runProcess(file, args, { cwd, env, timeoutMs = DEFAULT_TOOL_TIMEOUT_MS } = {}) {
  return new Promise((resolve, reject) => {
    const child = spawn(file, args, { cwd, env, windowsHide: true, stdio: ['ignore', 'pipe', 'pipe'] });
    const stdout = [];
    const stderr = [];
    const timer = setTimeout(() => {
      child.kill();
      reject(new FcdevError(`${file} 运行超过 ${timeoutMs} 毫秒，已终止`, { code: 'PROCESS_TIMEOUT' }));
    }, timeoutMs);
    child.stdout.on('data', (chunk) => stdout.push(chunk));
    child.stderr.on('data', (chunk) => stderr.push(chunk));
    child.on('error', (error) => {
      clearTimeout(timer);
      reject(new FcdevError(`无法启动 ${file}`, { code: 'PROCESS_START_FAILED', cause: error }));
    });
    child.on('close', (code) => {
      clearTimeout(timer);
      resolve({ code: code ?? -1, stdout: Buffer.concat(stdout).toString('utf8'), stderr: Buffer.concat(stderr).toString('utf8') });
    });
  });
}
