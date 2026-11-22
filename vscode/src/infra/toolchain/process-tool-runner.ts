// ------------------------------------------------------------------------
// 名称：process-tool-runner.ts
// 说明：工具运行接口的实现：启动工具链里的程序并收集输出，不经过 shell，参数逐个传递，带超时。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-22
// 备注：编译、链接这类工具的输出原样保留；调用时设置环境变量 CC65_HOME 为工具链根目录。
// ------------------------------------------------------------------------

import { spawn } from 'node:child_process';
import type { ProcessResult, ToolName, ToolRunner, Toolchain } from '../../contracts/toolchain/toolchain';
import { FcdevError } from '../../shared/errors/fcdev-error';
import { getToolPath } from './bundled-toolchain-locator';

/** 外部工具默认的超时时间，单位毫秒。 */
export const DEFAULT_TOOL_TIMEOUT_MS = 120_000;

/** 经子进程运行工具链里的程序。 */
export class ProcessToolRunner implements ToolRunner {
  /**
   * 创建运行器。
   * @param timeoutMs 单个工具的超时时间，单位毫秒。
   */
  constructor(private readonly timeoutMs: number = DEFAULT_TOOL_TIMEOUT_MS) {}

  /** @inheritdoc */
  run(toolchain: Toolchain, tool: ToolName, args: readonly string[], workingDirectory?: string): Promise<ProcessResult> {
    const file = getToolPath(toolchain, tool);
    return new Promise((resolve, reject) => {
      const child = spawn(file, [...args], {
        cwd: workingDirectory,
        env: { ...process.env, CC65_HOME: toolchain.home },
        windowsHide: true,
        stdio: ['ignore', 'pipe', 'pipe']
      });
      const stdout: Buffer[] = [];
      const stderr: Buffer[] = [];
      const timer = setTimeout(() => {
        child.kill();
        reject(new FcdevError(`${file} 运行超过 ${this.timeoutMs} 毫秒，已终止`, { code: 'PROCESS_TIMEOUT' }));
      }, this.timeoutMs);
      child.stdout.on('data', (chunk: Buffer) => stdout.push(chunk));
      child.stderr.on('data', (chunk: Buffer) => stderr.push(chunk));
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
}
