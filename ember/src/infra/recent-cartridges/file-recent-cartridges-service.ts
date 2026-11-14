/*
 * ------------------------------------------------------------------------
 * 名称：file-recent-cartridges-service.ts
 * 说明：最近打开的卡带服务的实现：启动时从本地文件读出并逐项校验（文件不存在或损坏时得到空清单，不报错弹窗），修改后交给文件存储防抖写盘。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-14
 * 备注：文件存储由组装根注入；每个路径在本次运行内有固定的编号（从 1 递增），重新加入已有的路径时编号不变。
 * ------------------------------------------------------------------------
 */

import type { RecentCartridge, RecentCartridgesService } from '../../contracts/cartridges/recent-cartridges-service';
import { addRecentCartridge, parseRecentCartridges, recentCartridgeName, removeRecentCartridge, toRecentCartridgesFile } from '../../shared/recent-cartridges/recent-cartridges';

/** 本地存储：读出保存的内容、安排写入、立即写盘。 */
export interface RecentCartridgesStorage {
  read(): unknown;
  write(value: unknown): void;
  flush(): void;
}

/** 保存在本地文件里的最近打开的卡带服务。 */
export class FileRecentCartridgesService implements RecentCartridgesService {
  private paths: readonly string[];
  private readonly ids = new Map<string, number>();
  private nextId = 1;

  constructor(private readonly storage: RecentCartridgesStorage) {
    this.paths = parseRecentCartridges(storage.read());
    for (const path of this.paths) this.idOf(path);
  }

  list(): readonly RecentCartridge[] {
    return this.paths.map((path) => ({ id: this.idOf(path), name: recentCartridgeName(path) }));
  }

  add(path: string): void {
    this.replace(addRecentCartridge(this.paths, path));
  }

  remove(path: string): void {
    this.replace(removeRecentCartridge(this.paths, path));
  }

  resolve(id: number): string | undefined {
    return this.paths.find((path) => this.ids.get(path) === id);
  }

  flush(): void {
    this.storage.flush();
  }

  /** 路径的编号：第一次遇到时分配。 */
  private idOf(path: string): number {
    const existing = this.ids.get(path);
    if (existing !== undefined) return existing;
    const id = this.nextId;
    this.nextId += 1;
    this.ids.set(path, id);
    return id;
  }

  private replace(next: readonly string[]): void {
    if (next === this.paths) return;
    this.paths = next;
    this.storage.write(toRecentCartridgesFile(next));
  }
}
