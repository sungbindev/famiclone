/*
 * ------------------------------------------------------------------------
 * 名称：BiosSettings.test.tsx
 * 说明：“机型与 BIOS”分组的测试：经首选项页的分组导航进入、列出四个整机机型且不含经典 FC、各状态的文字与按钮（未添加、已添加、暂未支持）、添加或替换弹出文件选择并用通知给结果、取消选择什么都不发生、移除不弹确认框、错误用通知提示、machines.changed 后更新。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-15
 * 备注：用假的预加载接口；通知经外壳接口的 notify 记录。
 * ------------------------------------------------------------------------
 */

import { UiProvider } from '@rujianjs/ui-kit';
import { act, render, screen, waitFor } from '@testing-library/react';
import userEvent from '@testing-library/user-event';
import { beforeEach, describe, expect, it, vi } from 'vitest';
import type { CoreStatus } from '../../contracts/core-gateway/core-messages';
import type { MachineInfo } from '../../contracts/machines/generated/machines';
import type { EmberBridge } from '../../contracts/shell/shell-bridge';
import { PreferencesPage } from './PreferencesPage';
import { ShellApiContext } from '../shell/shell-context';
import type { ShellApi } from '../shell/shell-registry';

const notify = vi.fn();
const shell = { notify } as unknown as ShellApi;

/** 默认的机型清单：经典 FC 与四个整机机型都已支持。 */
function machines(): MachineInfo[] {
  return [
    { id: 'fc', name: '经典 FC', needsBios: false, supported: true },
    { id: 'sb97', name: '小霸王 97', needsBios: true, supported: true },
    { id: 'sb2000', name: '小霸王 2000', needsBios: true, supported: true },
    { id: 'bbk10', name: '步步高软驱一号', needsBios: true, supported: true },
    { id: 'bbk20', name: '步步高 98', needsBios: true, supported: true }
  ];
}

/** 假预加载接口：记录请求，机型清单可以改写，可以主动触发事件。 */
function setup(initial: MachineInfo[] = machines()) {
  let current = initial;
  const eventListeners = new Set<(name: string, data: unknown) => void>();
  const statusListeners = new Set<(status: CoreStatus) => void>();
  const list = vi.fn(() => Promise.resolve({ machines: current }));
  const removeBios = vi.fn((params: { machine: string }) => {
    current = current.map((item) => (item.id === params.machine ? { ...item, bios: undefined } : item));
    return Promise.resolve({ machine: current.find((item) => item.id === params.machine) as MachineInfo });
  });
  const openBios = vi.fn((params: { machine: string }): Promise<{ canceled: boolean; machine?: MachineInfo }> => {
    current = current.map((item) => (item.id === params.machine ? { ...item, bios: { fileName: 'SB2K_BIOS.NES', size: 524304 } } : item));
    return Promise.resolve({ canceled: false, machine: current.find((item) => item.id === params.machine) as MachineInfo });
  });
  const bridge = {
    core: {
      preferences: { list: () => Promise.resolve({ groups: [{ id: 'display', label: '画面' }], settings: [] }), set: vi.fn(), reset: vi.fn() },
      machines: { list, removeBios }
    },
    main: { machines: { openBios } },
    onCoreEvent: (listener: (name: string, data: unknown) => void) => {
      eventListeners.add(listener);
      return () => eventListeners.delete(listener);
    },
    onCoreStatusChange: (listener: (status: CoreStatus) => void) => {
      statusListeners.add(listener);
      return () => statusListeners.delete(listener);
    }
  } as unknown as EmberBridge;
  render(
    <UiProvider>
      <ShellApiContext.Provider value={shell}>
        <PreferencesPage bridge={bridge} />
      </ShellApiContext.Provider>
    </UiProvider>
  );
  return {
    list,
    openBios,
    removeBios,
    change: (next: MachineInfo[]) => {
      current = next;
    },
    emit: (name: string, data: unknown) => act(() => eventListeners.forEach((listener) => listener(name, data)))
  };
}

/** 进入“机型与 BIOS”分组。 */
async function openGroup(): Promise<void> {
  await userEvent.click(await screen.findByText('机型与 BIOS'));
  await screen.findByText('小霸王 2000');
}

beforeEach(() => notify.mockClear());

describe('“机型与 BIOS”分组', () => {
  it('分组出现在左侧导航里；列出四个整机机型，不含经典 FC，并有说明文字', async () => {
    setup();
    await openGroup();
    expect(screen.getByText('已支持的机型自带默认 BIOS，启动时会自动装好，一般不用添加。想换成别的 BIOS，点“替换”；添加的文件会复制到软件的 BIOS 目录，开机时直接使用，原文件可以删除或移动。')).toBeTruthy();
    for (const name of ['小霸王 97', '小霸王 2000', '步步高软驱一号', '步步高 98']) expect(screen.getByText(name)).toBeTruthy();
    expect(screen.queryByText('经典 FC')).toBeNull();
  });

  it('未添加显示“未添加”与“添加 BIOS 文件”；核心标为暂未支持的机型整行按钮置灰并注明“暂未支持”', async () => {
    setup(machines().map((item) => (item.id === 'sb97' ? { ...item, supported: false } : item)));
    await openGroup();
    expect(screen.getAllByText('未添加')).toHaveLength(3);
    expect((screen.getByRole('button', { name: '添加 BIOS 文件：小霸王 2000' }) as HTMLButtonElement).disabled).toBe(false);
    expect(screen.getAllByText('暂未支持')).toHaveLength(1);
    expect((screen.getByRole('button', { name: '添加 BIOS 文件：小霸王 97' }) as HTMLButtonElement).disabled).toBe(true);
  });

  it('已添加显示文件名与大小，按钮是“替换”和“移除”', async () => {
    setup(machines().map((item) => (item.id === 'sb2000' ? { ...item, bios: { fileName: 'SB2K_BIOS.NES', size: 524304 } } : item)));
    await openGroup();
    expect(screen.getByText('SB2K_BIOS.NES（512 KB）')).toBeTruthy();
    expect(screen.getByRole('button', { name: '替换：小霸王 2000' })).toBeTruthy();
    expect(screen.getByRole('button', { name: '移除：小霸王 2000' })).toBeTruthy();
    expect(screen.queryByRole('button', { name: '添加 BIOS 文件：小霸王 2000' })).toBeNull();
  });

  it('添加 BIOS：经主进程弹出文件选择，成功后用通知告知，页面随 machines.changed 更新', async () => {
    const page = setup();
    await openGroup();
    await userEvent.click(screen.getByRole('button', { name: '添加 BIOS 文件：小霸王 2000' }));
    expect(page.openBios).toHaveBeenCalledWith({ machine: 'sb2000' });
    await waitFor(() => expect(notify).toHaveBeenCalledWith('已添加 小霸王 2000 的 BIOS：SB2K_BIOS.NES', 'success'));
    await page.emit('machines.changed', { machine: 'sb2000' });
    expect(await screen.findByText('SB2K_BIOS.NES（512 KB）')).toBeTruthy();
  });

  it('替换 BIOS：通知写“已替换”', async () => {
    const page = setup(machines().map((item) => (item.id === 'sb2000' ? { ...item, bios: { fileName: 'old.nes', size: 524304 } } : item)));
    await openGroup();
    await userEvent.click(screen.getByRole('button', { name: '替换：小霸王 2000' }));
    expect(page.openBios).toHaveBeenCalledWith({ machine: 'sb2000' });
    await waitFor(() => expect(notify).toHaveBeenCalledWith('已替换 小霸王 2000 的 BIOS：SB2K_BIOS.NES', 'success'));
  });

  it('用户取消选择：没有通知，清单不变', async () => {
    const page = setup();
    page.openBios.mockResolvedValueOnce({ canceled: true });
    await openGroup();
    await userEvent.click(screen.getByRole('button', { name: '添加 BIOS 文件：小霸王 2000' }));
    await waitFor(() => expect(page.openBios).toHaveBeenCalledTimes(1));
    expect(notify).not.toHaveBeenCalled();
    expect(screen.getAllByText('未添加')).toHaveLength(4);
  });

  it('文件不符合机型：用通知提示核心给出的说明', async () => {
    const page = setup();
    page.openBios.mockRejectedValueOnce({ code: 'machines.biosInvalid', message: '小霸王 2000 的 BIOS 的 PRG 必须是 512KB' });
    await openGroup();
    await userEvent.click(screen.getByRole('button', { name: '添加 BIOS 文件：小霸王 2000' }));
    await waitFor(() => expect(notify).toHaveBeenCalledWith('小霸王 2000 的 BIOS 的 PRG 必须是 512KB', 'error'));
  });

  it('移除 BIOS：不弹确认框，直接移除并用通知告知', async () => {
    const page = setup(machines().map((item) => (item.id === 'sb2000' ? { ...item, bios: { fileName: 'SB2K_BIOS.NES', size: 524304 } } : item)));
    await openGroup();
    await userEvent.click(screen.getByRole('button', { name: '移除：小霸王 2000' }));
    expect(page.removeBios).toHaveBeenCalledWith({ machine: 'sb2000' });
    await waitFor(() => expect(notify).toHaveBeenCalledWith('已移除 小霸王 2000 的 BIOS', 'success'));
    expect(screen.queryByRole('dialog')).toBeNull();
    await page.emit('machines.changed', { machine: 'sb2000' });
    await waitFor(() => expect(screen.getAllByText('未添加')).toHaveLength(4));
  });
});
