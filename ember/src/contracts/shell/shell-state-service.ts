/*
 * ------------------------------------------------------------------------
 * 名称：shell-state-service.ts
 * 说明：界面状态服务的契约：读取与修改窗口位置、各区域展开状态，并保证写入磁盘。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-14
 * 备注：状态的类型与校验在 shared/shell-state；实现把状态保存到用户数据目录的本地文件（infra/shell-state）。
 * ------------------------------------------------------------------------
 */

import type { ShellLayout, ShellState, WindowPlacement } from '../../shared/shell-state/shell-state';

export type { ShellLayout, ShellState, WindowPlacement } from '../../shared/shell-state/shell-state';

/** 界面状态服务。 */
export interface ShellStateService {
  /** 当前的界面状态。 */
  get(): ShellState;
  /** 修改布局；只改传入的字段。 */
  updateLayout(patch: Partial<ShellLayout>): void;
  /** 记下窗口的位置与大小。 */
  updateWindow(placement: WindowPlacement): void;
  /** 立即写盘（应用退出前调用）。 */
  flush(): void;
}
