// ------------------------------------------------------------------------
// 名称：toolchain.ts
// 说明：cc65 工具链的定位与运行接口。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-22
// 备注：工具链只有随扩展发布的一份，不使用 PATH 里的 cc65，避免版本不一致。
// ------------------------------------------------------------------------

/** 工具链。 */
export interface Toolchain {
  /** 工具链根目录。 */
  readonly home: string;
}

/** 工具链里的程序名。 */
export type ToolName = 'cc65' | 'ca65' | 'ld65' | 'ar65' | 'da65';

/** 外部程序的运行结果。 */
export interface ProcessResult {
  /** 退出码。 */
  readonly code: number;
  /** 标准输出。 */
  readonly stdout: string;
  /** 标准错误。 */
  readonly stderr: string;
}

/** 定位随扩展发布的工具链。 */
export interface ToolchainLocator {
  /**
   * 取得工具链。
   * @returns 工具链。
   * @throws FcdevError 工具链缺失或不完整。
   */
  locate(): Toolchain;
}

/** 运行工具链里的程序。 */
export interface ToolRunner {
  /**
   * 运行一个工具并等待结束，不经过 shell，参数逐个传递。
   * @param toolchain 工具链。
   * @param tool 工具名。
   * @param args 参数。
   * @param workingDirectory 工作目录；省略时用当前进程的。
   * @returns 运行结果；以非零退出码结束时不抛错，由调用者判断。
   * @throws FcdevError 程序无法启动或超时。
   */
  run(toolchain: Toolchain, tool: ToolName, args: readonly string[], workingDirectory?: string): Promise<ProcessResult>;
}
