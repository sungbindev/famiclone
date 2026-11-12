// ------------------------------------------------------------------------
// 名称：fcdev-command-runner.ts
// 说明：运行 fcdev 命令行的外部能力接口，以及查找工作区里 fcdev 项目的接口。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-10
// 备注：由实现层提供；逻辑层只经这两个接口使用命令行与文件系统。
// ------------------------------------------------------------------------

/** 一次命令行运行的结果。 */
export interface FcdevCommandResult {
  /** 退出码，0 表示成功。 */
  readonly exitCode: number;
  /** 标准输出。 */
  readonly stdout: string;
  /** 标准错误。 */
  readonly stderr: string;
}

/** 运行项目里的 fcdev 命令行。 */
export interface FcdevCommandRunner {
  /**
   * 在项目目录里运行 fcdev 命令。
   * @param projectRoot 项目根目录，即 fcdev.json 所在目录。
   * @param args 命令行参数，如 ['info', '--json']。
   * @returns 运行结果；命令以非零退出码结束时不抛错，由调用者判断。
   * @throws UserFacingError 项目里没有安装 fcdev、找不到 node，或运行超时。
   */
  run(projectRoot: string, args: readonly string[]): Promise<FcdevCommandResult>;
}

/** 查找工作区里的 fcdev 项目。 */
export interface FcdevProjectLocator {
  /**
   * 列出工作区里全部 fcdev 项目的根目录。
   * @returns 根目录的绝对路径，不含 node_modules 里的项目。
   */
  findProjectRoots(): Promise<readonly string[]>;
}
