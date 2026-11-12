// ------------------------------------------------------------------------
// 名称：intellisense-sync-service.ts
// 说明：C 智能感知配置同步服务的接口：运行 fcdev sync，让 C/C++ 扩展与 clangd 找到 cc65 头文件。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-10
// 备注：同步失败不抛错，结果里带原因，由入口层决定是否提示用户。
// ------------------------------------------------------------------------

/** 一个项目的同步结果。 */
export interface SyncResult {
  /** 项目根目录。 */
  readonly root: string;
  /** 是否成功。 */
  readonly ok: boolean;
  /** 说明：成功时是 fcdev 的输出，失败时是原因。 */
  readonly message: string;
}

/** C 智能感知配置同步服务。 */
export interface IntelliSenseSyncService {
  /**
   * 同步一个项目的配置。同一个项目的同步按提交顺序依次运行。
   * @param root 项目根目录。
   * @returns 同步结果。
   */
  syncProject(root: string): Promise<SyncResult>;

  /**
   * 同步工作区里全部 fcdev 项目的配置。
   * @returns 每个项目的同步结果；工作区里没有 fcdev 项目时为空数组。
   */
  syncWorkspace(): Promise<readonly SyncResult[]>;
}
