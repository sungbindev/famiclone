// ------------------------------------------------------------------------
// 名称：emulator-operations.ts
// 说明：模拟器操作与运行程序的能力接口：按操作名与参数运行 Copilot 模拟器工具对应的操作；构建程序后
//       在 Ember 里运行（自由运行或调试运行）。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-22
// 备注：会话（到 Ember 的连接）保存在实现里，跨调用保持；Ember 的位置与端口取自扩展设置，操作之外
//       没有项目目录之类的参数；结果是可序列化的普通对象。
// ------------------------------------------------------------------------

import type { BuildInput, BuildReporter } from '../build/build-service';

/** 模拟器操作。 */
export interface EmulatorOperations {
  /** 全部操作名。 */
  readonly operationNames: readonly string[];

  /**
   * 运行一个操作。
   * @param operation 操作名，如 connect、load、run。
   * @param params 操作的参数，路径都已经是绝对路径。
   * @returns 操作的结果，是普通的可序列化对象。
   * @throws FcdevError 操作名不认识、参数不合法、没有连接、没有设置 Ember 的位置，或 Ember 报错。
   */
  run(operation: string, params: Record<string, unknown>): Promise<Record<string, unknown>>;

  /**
   * 关闭会话，并结束由本扩展启动的 Ember；没有用过模拟器时什么也不做。
   * 只结束自己启动的进程，不影响用户自己打开的 Ember。
   */
  shutdown(): Promise<void>;
}

/** 运行程序的输入。 */
export interface RunProgramInput {
  /** 构建输入：路径、起始目录与目标。 */
  readonly build: BuildInput;
  /** 调试运行：启动调试服务，程序载入后停在开机状态，由调试命令与 emu 工具继续操作。 */
  readonly debug: boolean;
}

/** 运行程序的结果。 */
export interface RunProgramResult {
  /** 启动的 Ember 的进程号。 */
  readonly pid: number | undefined;
  /** 载入内容的说明，如产物路径，或磁盘镜像与程序名。 */
  readonly description: string;
  /** 是否调试运行。 */
  readonly debug: boolean;
  /** 调试运行用的端口；不是调试运行时为 undefined。 */
  readonly port: number | undefined;
  /** 是整机（固件加磁盘镜像）运行：程序要在 SB-DOS 里输入程序名才会运行。 */
  readonly firmwareDisk: boolean;
}

/** 构建程序并在 Ember 里运行。 */
export interface ProgramRunner {
  /**
   * 构建程序后在 Ember 里运行。
   * @param input 输入。
   * @param reporter 构建进度汇报。
   * @returns 运行结果。
   * @throws FcdevError 目标没有运行方案、整机目标没有配置固件、没有设置 Ember 的位置、找不到 Embe
   *   r，
   *   或构建失败。
   */
  run(input: RunProgramInput, reporter?: BuildReporter): Promise<RunProgramResult>;
}
