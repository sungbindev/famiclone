// ------------------------------------------------------------------------
// 名称：endpoint.ts
// 说明：调试服务端点的类型与查找接口：端点文件的内容、查找连接目标的参数、连接目标，以及读取它们的
//       能力。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-22
// 备注：端点文件由 Ember 在调试服务开始监听时写到用户数据目录的 run/debug-endpoint.json，停止时删
//       除；地址与令牌也可以经环境变量 EMBER_DEBUG_URL、EMBER_DEBUG_TOKEN 给出。
// ------------------------------------------------------------------------

/** 端点文件的内容（调试服务开始监听时写入，停止时删除）。 */
export interface Endpoint {
  readonly url: string;
  readonly port: number;
  readonly token: string;
  /** Ember 主进程的进程号。 */
  readonly pid: number;
  readonly coreVersion: string;
  readonly appVersion: string;
  readonly startedAt: string;
}

/** 找连接目标时的显式参数；没给的部分依次取环境变量与端点文件。 */
export interface TargetOptions {
  /** 调试服务的地址（环境变量 EMBER_DEBUG_URL）。 */
  readonly url?: string;
  /** 令牌（环境变量 EMBER_DEBUG_TOKEN）。 */
  readonly token?: string;
  /** 端点文件的路径（环境变量 EMBER_DEBUG_ENDPOINT）。 */
  readonly endpointFile?: string;
  /**
   * Ember 的用户数据目录，端点文件在它的 run 子目录下（环境变量 EMBER_USER_DATA）；默认是当前系统上
   * Ember 的用户数据目录。
   */
  readonly userDataDirectory?: string;
}

/** 连接目标。 */
export interface Target {
  readonly url: string;
  readonly token: string;
  /** 地址与令牌的来源，给提示信息用。 */
  readonly source: string;
}

/** 查找正在运行的 Ember 的调试服务。 */
export interface EndpointResolver {
  /**
   * 端点文件的路径：显式参数、环境变量、用户数据目录，依次取第一个有的。
   * @param options 显式参数。
   * @returns 端点文件的路径。
   */
  endpointFilePath(options?: TargetOptions): string;

  /**
   * 读端点文件，并确认写它的 Ember 进程还在。
   * @param options 显式参数。
   * @returns 端点文件的内容。
   * @throws EmberConnectionError 文件不存在、内容不对或进程已退出（种类为 endpoint）。
   */
  readEndpoint(options?: TargetOptions): Endpoint;

  /**
   * 确定连接目标：显式参数优先，其次环境变量，缺的部分从端点文件补。
   * @param options 显式参数。
   * @returns 连接目标。
   * @throws EmberConnectionError 找不到完整的地址与令牌（种类为 endpoint）。
   */
  resolveTarget(options?: TargetOptions): Target;
}
