/*
 * ------------------------------------------------------------------------
 * 名称：recent-cartridges-service.ts
 * 说明：最近打开的卡带服务的契约：读出清单（显示名与编号）、加入、移出、按编号取路径，并保证写入磁盘。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-14
 * 备注：路径只在主进程里使用；对渲染进程只给显示名与编号，编号只在本次运行内有效，重启后重新编号。
 * ------------------------------------------------------------------------
 */

/** 清单里的一项：给界面看的部分。 */
export interface RecentCartridge {
  /** 编号：本次运行内有效，从 1 开始。 */
  readonly id: number;
  /** 显示名：文件名。 */
  readonly name: string;
}

/** 最近打开的卡带服务。 */
export interface RecentCartridgesService {
  /** 当前的清单，最近的在前。 */
  list(): readonly RecentCartridge[];
  /** 把卡带加到清单最前面。 */
  add(path: string): void;
  /** 把卡带移出清单。 */
  remove(path: string): void;
  /** 按编号取卡带路径；编号不在清单里时为 undefined。 */
  resolve(id: number): string | undefined;
  /** 立即写盘（应用退出前调用）。 */
  flush(): void;
}
