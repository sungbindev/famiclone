/*
 * ------------------------------------------------------------------------
 * 名称：memory-dropped-files-service.ts
 * 说明：拖入窗口的文件的服务实现：路径只记在内存里，编号递增，只保留最近的若干个。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-16
 * ------------------------------------------------------------------------
 */

import { basename } from 'node:path';
import type { DroppedFile, DroppedFilesService } from '../../contracts/shell/dropped-files';

/** 最多保留的文件个数：一次拖放用不了这么多，留出余量只为了界面还没来得及用完编号时不被顶掉。 */
const MAX_KEPT = 64;

/** 把拖入的文件路径记在内存里的服务。 */
export class MemoryDroppedFilesService implements DroppedFilesService {
  private nextId = 1;
  private readonly paths = new Map<number, string>();

  register(paths: readonly string[]): readonly DroppedFile[] {
    return paths.map((path) => {
      const id = this.nextId;
      this.nextId += 1;
      this.paths.set(id, path);
      // Map 按插入顺序遍历，第一个就是最早登记的。
      if (this.paths.size > MAX_KEPT) this.paths.delete(this.paths.keys().next().value as number);
      return { id, name: basename(path) };
    });
  }

  resolve(id: number): string | undefined {
    return this.paths.get(id);
  }
}
