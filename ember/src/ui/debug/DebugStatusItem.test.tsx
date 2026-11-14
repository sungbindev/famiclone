/*
 * ------------------------------------------------------------------------
 * 名称：DebugStatusItem.test.tsx
 * 说明：调试服务状态栏项与控制器的测试：没读到时不显示，没开启时显示“调试已关闭”；监听中显示端口，有连接时显示连接数与绿色状态点；开启但没有监听时显示警告与原因；点击打开首选项；状态事件与核心重启后刷新；较早发出的读取结果晚到时丢弃。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-14
 * 备注：用假的预加载接口与真实的控制器。
 * ------------------------------------------------------------------------
 */

import { act, fireEvent, render, screen } from '@testing-library/react';
import type { ReactElement } from 'react';
import { describe, expect, it } from 'vitest';
import type { CoreStatus } from '../../contracts/core-gateway/core-messages';
import { ShellApiContext } from '../shell/shell-context';
import type { ShellApi } from '../shell/shell-registry';
import { DebugController, type DebugBackend } from './debug-controller';
import { DebugStatusItem } from './DebugStatusItem';

/** debug.status 的结果。 */
interface StatusResult {
  enabled: boolean;
  listening: boolean;
  port: number;
  error?: string;
  connections: Array<{ id: number; client: string; connectedAt: string }>;
}

/** 假的预加载接口：status 的结果可以改，也可以让读取悬停到手动完成。 */
class FakeDebugBackend {
  status: StatusResult = { enabled: false, listening: false, port: 0, connections: [] };
  /** 悬停的读取：不为空时 status 返回它的 promise。 */
  pending: Array<(result: StatusResult) => void> = [];
  hold = false;
  failing = false;
  private readonly eventListeners = new Set<(name: string, data: unknown) => void>();
  private readonly statusListeners = new Set<(status: CoreStatus) => void>();

  readonly backend: DebugBackend = {
    core: {
      debug: {
        status: () => {
          if (this.failing) return Promise.reject(new Error('核心不可用'));
          if (this.hold) return new Promise<StatusResult>((resolve) => this.pending.push(resolve));
          return Promise.resolve(this.status);
        }
      }
    },
    onCoreEvent: (listener: (name: string, data: unknown) => void) => {
      this.eventListeners.add(listener);
      return () => this.eventListeners.delete(listener);
    },
    onCoreStatusChange: (listener: (status: CoreStatus) => void) => {
      this.statusListeners.add(listener);
      return () => this.statusListeners.delete(listener);
    }
  } as unknown as DebugBackend;

  emit(name: string, data: unknown = {}): void {
    for (const listener of this.eventListeners) listener(name, data);
  }

  changeCore(state: 'starting' | 'ready'): void {
    for (const listener of this.statusListeners) listener({ state } as CoreStatus);
  }
}

/** 等待已经排队的 promise 完成。 */
async function settle(): Promise<void> {
  await Promise.resolve();
  await Promise.resolve();
}

/** 接上假接口并等初始读取完成。 */
async function connect(fake: FakeDebugBackend): Promise<DebugController> {
  const controller = new DebugController();
  controller.attach(fake.backend);
  await act(async () => settle());
  return controller;
}

/** 假的外壳接口：只记录执行过的命令。 */
function fakeShell(): { api: ShellApi; commands: string[] } {
  const commands: string[] = [];
  return { api: { runCommand: (id: string) => commands.push(id) } as unknown as ShellApi, commands };
}

/** 放进外壳上下文里渲染状态项。 */
function renderItem(controller: DebugController, shell: ShellApi = fakeShell().api): ReturnType<typeof render> {
  const element: ReactElement = (
    <ShellApiContext.Provider value={shell}>
      <DebugStatusItem controller={controller} />
    </ShellApiContext.Provider>
  );
  return render(element);
}

/** 状态项里状态点的含义。 */
function dotOf(container: HTMLElement): string | null {
  return container.querySelector('.ui-status-bar__dot')?.getAttribute('data-dot') ?? null;
}

const CONNECTION = { id: 1, client: 'copilot', connectedAt: '2026-11-14T10:00:00Z' };

describe('调试服务状态栏项', () => {
  it('没有读到状态时不显示', () => {
    const controller = new DebugController();
    const { container } = renderItem(controller);
    expect(container.textContent).toBe('');
  });

  it('没有开启时显示“调试已关闭”，灰色状态点', async () => {
    const fake = new FakeDebugBackend();
    const controller = await connect(fake);
    const { container } = renderItem(controller);
    expect(screen.getByText('调试已关闭')).toBeTruthy();
    expect(dotOf(container)).toBe('neutral');
  });

  it('点击状态项打开首选项（命令 preferences.open）', async () => {
    const fake = new FakeDebugBackend();
    fake.status = { enabled: true, listening: true, port: 5688, connections: [] };
    const controller = await connect(fake);
    const shell = fakeShell();
    renderItem(controller, shell.api);
    fireEvent.click(screen.getByText('调试服务 5688'));
    expect(shell.commands).toEqual(['preferences.open']);
  });

  it('监听中显示端口，没有连接是灰色状态点', async () => {
    const fake = new FakeDebugBackend();
    fake.status = { enabled: true, listening: true, port: 5688, connections: [] };
    const controller = await connect(fake);
    const { container } = renderItem(controller);
    expect(screen.getByText('调试服务 5688')).toBeTruthy();
    expect(dotOf(container)).toBe('neutral');
  });

  it('有连接时显示连接数与绿色状态点', async () => {
    const fake = new FakeDebugBackend();
    fake.status = { enabled: true, listening: true, port: 5688, connections: [CONNECTION, { ...CONNECTION, id: 2 }] };
    const controller = await connect(fake);
    const { container } = renderItem(controller);
    expect(screen.getByText('调试服务 5688（2 个连接）')).toBeTruthy();
    expect(dotOf(container)).toBe('success');
  });

  it('开启了但没有监听时显示警告，原因放在提示里', async () => {
    const fake = new FakeDebugBackend();
    fake.status = { enabled: true, listening: false, port: 0, error: '端口 5688 已被占用', connections: [] };
    const controller = await connect(fake);
    const { container } = renderItem(controller);
    expect(screen.getByText('调试服务未监听')).toBeTruthy();
    expect(dotOf(container)).toBe('warning');
    expect(container.querySelector('[title]')?.getAttribute('title')).toBe('调试服务没有监听：端口 5688 已被占用');
  });

  it('收到 debug.statusChanged 后重新读取：连接建立与服务停止', async () => {
    const fake = new FakeDebugBackend();
    fake.status = { enabled: true, listening: true, port: 5688, connections: [] };
    const controller = await connect(fake);
    const { container } = renderItem(controller);
    fake.status = { enabled: true, listening: true, port: 5688, connections: [CONNECTION] };
    fake.emit('debug.statusChanged', { enabled: true, listening: true, port: 5688, connections: 1 });
    await act(async () => settle());
    expect(screen.getByText('调试服务 5688（1 个连接）')).toBeTruthy();
    fake.status = { enabled: false, listening: false, port: 0, connections: [] };
    fake.emit('debug.statusChanged', { enabled: false, listening: false, port: 0, connections: 0 });
    await act(async () => settle());
    expect(container.textContent).toBe('调试已关闭');
  });

  it('其他事件不触发读取', async () => {
    const fake = new FakeDebugBackend();
    fake.status = { enabled: true, listening: true, port: 5688, connections: [] };
    const controller = await connect(fake);
    renderItem(controller);
    fake.status = { enabled: false, listening: false, port: 0, connections: [] };
    fake.emit('runtime.stateChanged', {});
    await act(async () => settle());
    expect(screen.getByText('调试服务 5688')).toBeTruthy();
  });

  it('核心重启：先不显示，就绪后显示新状态', async () => {
    const fake = new FakeDebugBackend();
    fake.status = { enabled: true, listening: true, port: 5688, connections: [] };
    const controller = await connect(fake);
    const { container } = renderItem(controller);
    act(() => fake.changeCore('starting'));
    expect(container.textContent).toBe('');
    fake.status = { enabled: true, listening: true, port: 5700, connections: [] };
    act(() => fake.changeCore('ready'));
    await act(async () => settle());
    expect(screen.getByText('调试服务 5700')).toBeTruthy();
  });

  it('读取失败时不显示', async () => {
    const fake = new FakeDebugBackend();
    fake.status = { enabled: true, listening: true, port: 5688, connections: [] };
    const controller = await connect(fake);
    const { container } = renderItem(controller);
    fake.failing = true;
    fake.emit('debug.statusChanged');
    await act(async () => settle());
    expect(container.textContent).toBe('');
  });

  it('较早发出的读取结果晚到时丢弃，以最新的为准', async () => {
    const fake = new FakeDebugBackend();
    fake.hold = true;
    const controller = new DebugController();
    controller.attach(fake.backend);
    fake.emit('debug.statusChanged');
    const { container } = renderItem(controller);
    // 第二次读取先完成，第一次后到
    await act(async () => {
      fake.pending[1]?.({ enabled: true, listening: true, port: 5700, connections: [] });
      await settle();
      fake.pending[0]?.({ enabled: true, listening: true, port: 5688, connections: [] });
      await settle();
    });
    expect(container.textContent).toBe('调试服务 5700');
  });

  it('断开之后不再读取也不再更新', async () => {
    const fake = new FakeDebugBackend();
    fake.status = { enabled: true, listening: true, port: 5688, connections: [] };
    const controller = new DebugController();
    const detach = controller.attach(fake.backend);
    detach();
    await act(async () => settle());
    expect(controller.snapshot().kind).toBe('unknown');
  });
});
