/*
 * ------------------------------------------------------------------------
 * 名称：file-shell-state-service.test.ts
 * 说明：界面状态服务的测试：没有保存过与内容损坏时用默认值、读出保存的状态、修改后写入存储、flush 转给存储，并用真实的文件存储走一遍“写盘后重启读回”。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-14
 * ------------------------------------------------------------------------
 */

// @vitest-environment node

import { mkdtempSync, rmSync, writeFileSync } from 'node:fs';
import { tmpdir } from 'node:os';
import { join } from 'node:path';
import { describe, expect, it, vi } from 'vitest';
import { JsonFileStore } from '../../platform/shell-state/json-file-store';
import { DEFAULT_SHELL_STATE } from '../../shared/shell-state/shell-state';
import { FileShellStateService } from './file-shell-state-service';

/** 假存储：读出的内容由测试决定，写入记录在 writes 里。 */
function createStorage(saved: unknown) {
  const writes: unknown[] = [];
  return { read: () => saved, write: (value: unknown) => writes.push(value), flush: vi.fn(), writes };
}

describe('FileShellStateService', () => {
  it('没有保存过或内容损坏时用默认值', () => {
    expect(new FileShellStateService(createStorage(undefined)).get()).toEqual(DEFAULT_SHELL_STATE);
    expect(new FileShellStateService(createStorage('坏的')).get()).toEqual(DEFAULT_SHELL_STATE);
  });

  it('读出保存的状态，不合格的项用默认值', () => {
    const service = new FileShellStateService(
      createStorage({ window: { x: 1, y: 2, width: 1000, height: 700, maximized: false }, layout: { rightBarOpen: true, bottomPanelHeight: -5 } })
    );
    expect(service.get().window).toEqual({ x: 1, y: 2, width: 1000, height: 700, maximized: false });
    expect(service.get().layout).toEqual({ bottomPanelHeight: null });
  });

  it('修改布局只改传入的字段，并把整个状态交给存储', () => {
    const storage = createStorage(undefined);
    const service = new FileShellStateService(storage);
    service.updateLayout({ bottomPanelHeight: 260 });
    service.updateLayout({ bottomPanelHeight: 300 });
    expect(service.get().layout).toEqual({ bottomPanelHeight: 300 });
    expect(storage.writes).toHaveLength(2);
    expect(storage.writes[1]).toEqual(service.get());
  });

  it('记下窗口位置，不影响布局', () => {
    const storage = createStorage(undefined);
    const service = new FileShellStateService(storage);
    service.updateWindow({ x: 10, y: 20, width: 1100, height: 700, maximized: true });
    expect(service.get().window).toEqual({ x: 10, y: 20, width: 1100, height: 700, maximized: true });
    expect(service.get().layout).toEqual(DEFAULT_SHELL_STATE.layout);
  });

  it('读出的状态不会被后续修改改变', () => {
    const service = new FileShellStateService(createStorage(undefined));
    const before = service.get();
    service.updateLayout({ bottomPanelHeight: 400 });
    expect(before.layout.bottomPanelHeight).toBeNull();
    expect(DEFAULT_SHELL_STATE.layout.bottomPanelHeight).toBeNull();
  });

  it('flush 转给存储', () => {
    const storage = createStorage(undefined);
    new FileShellStateService(storage).flush();
    expect(storage.flush).toHaveBeenCalledTimes(1);
  });
});

describe('配合真实的文件存储', () => {
  it('写盘后重新创建服务能读回；文件被写坏后用默认值且不抛异常', () => {
    const directory = mkdtempSync(join(tmpdir(), 'ember-shell-state-'));
    try {
      const path = join(directory, 'config', 'shell-state.json');
      const first = new FileShellStateService(new JsonFileStore({ path }));
      first.updateLayout({ bottomPanelHeight: 300 });
      first.updateWindow({ x: 5, y: 6, width: 1300, height: 820, maximized: false });
      first.flush();

      const second = new FileShellStateService(new JsonFileStore({ path }));
      expect(second.get()).toEqual(first.get());

      writeFileSync(path, '{"layout": {"sideBar', 'utf8');
      expect(new FileShellStateService(new JsonFileStore({ path })).get()).toEqual(DEFAULT_SHELL_STATE);
    } finally {
      rmSync(directory, { recursive: true, force: true });
    }
  });
});
