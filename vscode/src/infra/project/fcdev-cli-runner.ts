// ------------------------------------------------------------------------
// 名称：fcdev-cli-runner.ts
// 说明：运行项目里的 fcdev 命令行：向上找 node_modules/fcdev，用 PATH 里的 node 启动，不走 shell。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-10
// 备注：只运行项目自己安装的 fcdev，保证与项目锁定的版本一致；超时后终止进程。
// ------------------------------------------------------------------------

import { spawn } from 'node:child_process';
import { existsSync } from 'node:fs';
import * as path from 'node:path';
import type { FcdevCommandResult, FcdevCommandRunner } from '../../contracts/project/fcdev-command-runner';
import { UserFacingError } from '../../shared/errors/user-facing-error';

/** fcdev 命令行脚本相对 node_modules 所在目录的路径。 */
const FCDEV_SCRIPT = path.join('node_modules', 'fcdev', 'bin', 'fcdev.mjs');

/** 单次命令的最长运行时间，单位毫秒。 */
const COMMAND_TIMEOUT_MS = 60_000;

/**
 * 从项目目录向上查找已安装的 fcdev 脚本。
 * @param projectRoot 项目根目录。
 * @returns 脚本的绝对路径；没有安装时为 undefined。
 */
export function locateFcdevScript(projectRoot: string): string | undefined {
  let directory = projectRoot;
  for (;;) {
    const candidate = path.join(directory, FCDEV_SCRIPT);
    if (existsSync(candidate)) return candidate;
    const parent = path.dirname(directory);
    if (parent === directory) return undefined;
    directory = parent;
  }
}

/** 经 PATH 里的 node 运行项目里的 fcdev。 */
export class FcdevCliRunner implements FcdevCommandRunner {
  /** @inheritdoc */
  run(projectRoot: string, args: readonly string[]): Promise<FcdevCommandResult> {
    const script = locateFcdevScript(projectRoot);
    if (script === undefined) {
      return Promise.reject(new UserFacingError('项目里没有安装 fcdev（找不到 node_modules/fcdev），请先运行 npm install'));
    }
    return new Promise((resolve, reject) => {
      const child = spawn('node', [script, ...args], { cwd: projectRoot, windowsHide: true, stdio: ['ignore', 'pipe', 'pipe'] });
      let stdout = '';
      let stderr = '';
      child.stdout.setEncoding('utf8').on('data', (chunk: string) => (stdout += chunk));
      child.stderr.setEncoding('utf8').on('data', (chunk: string) => (stderr += chunk));
      const timer = setTimeout(() => {
        child.kill();
        reject(new UserFacingError(`fcdev 运行超过 ${COMMAND_TIMEOUT_MS / 1000} 秒，已终止`));
      }, COMMAND_TIMEOUT_MS);
      child.once('error', (error: NodeJS.ErrnoException) => {
        clearTimeout(timer);
        reject(error.code === 'ENOENT' ? new UserFacingError('找不到 node，请确认已安装 Node.js 并且在 PATH 里', error) : error);
      });
      child.once('close', (code) => {
        clearTimeout(timer);
        resolve({ exitCode: code ?? -1, stdout, stderr });
      });
    });
  }
}
