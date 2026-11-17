/*
 * ------------------------------------------------------------------------
 * 名称：file-shell-state-service.ts
 * 说明：界面状态服务的实现：启动时从本地文件读出并逐项校验（文件不存在或损坏时用默认值，不报错弹窗），修改后交给文件存储防抖写盘。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-14
 * 备注：文件存储由组装根注入；状态只在内存里整体替换，读出的对象不会被后续修改改变。
 * ------------------------------------------------------------------------
 */

import type { ShellStateService } from '../../contracts/shell/shell-state-service';
import { parseShellState, type ShellLayout, type ShellState, type WindowPlacement } from '../../shared/shell-state/shell-state';

/** 本地存储：读出保存的内容、安排写入、立即写盘。 */
export interface ShellStateStorage {
  read(): unknown;
  write(value: unknown): void;
  flush(): void;
}

/** 保存在本地文件里的界面状态服务。 */
export class FileShellStateService implements ShellStateService {
  private state: ShellState;

  constructor(private readonly storage: ShellStateStorage) {
    this.state = parseShellState(storage.read());
  }

  get(): ShellState {
    return this.state;
  }

  updateLayout(patch: Partial<ShellLayout>): void {
    this.replace({ ...this.state, layout: { ...this.state.layout, ...patch } });
  }

  updateWindow(placement: WindowPlacement): void {
    this.replace({ ...this.state, window: placement });
  }

  updateReleaseNotesVersion(version: string): void {
    this.replace({ ...this.state, releaseNotesVersion: version });
  }

  flush(): void {
    this.storage.flush();
  }

  private replace(next: ShellState): void {
    this.state = next;
    this.storage.write(next);
  }
}
