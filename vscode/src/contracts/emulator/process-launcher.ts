// ------------------------------------------------------------------------
// 名称：process-launcher.ts
// 说明：启动与结束 Ember 进程的接口：检查端口、准备调试用的用户数据目录、启动脱离的进程、结束进程。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-22
// 备注：Ember 与启动它的程序脱离，扩展退出后窗口保持打开；只结束自己启动的进程，不碰用户自己打开的
//       Ember。
// ------------------------------------------------------------------------

/** 启动的 Ember 进程。 */
export interface EmulatorProcess {
  /** 进程号；没有拿到时为 undefined。 */
  readonly pid: number | undefined;
  /**
   * 进程启动失败或提前退出时以 FcdevError（EMULATOR_START_FAILED）失败；启动成功后 Ember 退出也会失
   * 败，
   * 没有人等它时不算未处理的拒绝。
   */
  readonly failure: Promise<never>;
  /** 结束进程。 */
  kill(): void;
}

/** 启动与结束 Ember 进程。 */
export interface EmulatorProcessLauncher {
  /**
   * 确认本机端口空闲。端口被占用时 Ember 的调试服务不会启动，客户端反而可能连到别的 Ember。
   * @param port 端口。
   * @throws FcdevError 端口已被占用（EMULATOR_PORT_BUSY）。
   */
  assertPortFree(port: number): Promise<void>;

  /**
   * 调试模式下 Ember 使用的用户数据目录：每个端口一个，与用户平时使用的 Ember 互不影响（单实例锁与
   * 设置
   * 都按用户数据目录区分）。
   * @param port 调试端口。
   * @returns 目录路径，不保证已存在。
   */
  userDataDirectory(port: number): string;

  /**
   * 准备调试用的用户数据目录：创建目录，删掉上一次会话遗留的端点文件，让客户端不会读到过期的端点。
   * @param port 调试端口。
   * @returns 目录路径。
   */
  prepareUserDataDirectory(port: number): string;

  /**
   * 启动一个与当前进程脱离的 Ember。
   * @param executable Ember 可执行文件。
   * @param args 命令行参数。
   * @param failOnExit 进程退出是否算失败。调试模式算；自由运行不算，因为 Ember 已有实例时
   *   新进程把任务交给它就退出。
   * @returns 进程。
   */
  start(executable: string, args: readonly string[], failOnExit: boolean): EmulatorProcess;

  /**
   * 给启动失败留出时间：等几百毫秒，期间进程启动失败就抛出。
   * @param child 刚启动的进程。
   * @throws FcdevError 进程启动失败（EMULATOR_START_FAILED）。
   */
  confirmStarted(child: EmulatorProcess): Promise<void>;

  /**
   * 结束进程。
   * @param pid 进程号。
   * @returns 是否结束了进程；进程已经退出时为 false。
   */
  terminate(pid: number): boolean;

  /**
   * 生成调试令牌：随机的十六进制文字，经命令行交给 Ember，客户端握手时用它。
   * @returns 令牌。
   */
  createToken(): string;
}
