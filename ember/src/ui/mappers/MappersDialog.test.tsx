/*
 * ------------------------------------------------------------------------
 * 名称：MappersDialog.test.tsx
 * 说明：“mapper 插件”对话框的测试：表格显示各列、搜索与来源筛选、选中显示清单摘要、预装插件不能卸载、停用与启用、卸载前确认、安装（成功、取消、失败）、
 *       别处变化（mappers.changed）后重新读取、读取失败的提示、核心重启后重新读取。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-15
 * 备注：用假的预加载接口；失败与成功的说明走外壳的通知（假的 notify）。
 * ------------------------------------------------------------------------
 */

import { UiProvider } from '@rujianjs/ui-kit';
import { act, render, screen, waitFor, within } from '@testing-library/react';
import userEvent from '@testing-library/user-event';
import { beforeEach, describe, expect, it, vi } from 'vitest';
import type { CoreStatus } from '../../contracts/core-gateway/core-messages';
import type { MapperDetail, MapperSummary } from '../../contracts/mappers/generated/mappers';
import type { EmberBridge } from '../../contracts/shell/shell-bridge';
import { ShellApiContext } from '../shell/shell-context';
import type { ShellApi } from '../shell/shell-registry';
import { MappersDialog } from './MappersDialog';

const notify = vi.fn();
const shell = { notify } as unknown as ShellApi;

const NROM: MapperSummary = { id: 'nrom', name: 'NROM', version: '1.0.0', publisher: 'Ember', description: '最简单的卡带', host: ['fc'], devices: [], match: [{ mapper: 0 }], source: 'bundled', enabled: true };
const CUSTOM: MapperSummary = { id: 'custom', name: '自制插件', version: '2.1.0', publisher: '某作者', host: ['fc'], devices: [], match: [{ mapper: 3, variant: 1 }], source: 'user', enabled: true };

function detailOf(summary: MapperSummary): MapperDetail {
  return {
    summary,
    storage: [{ id: 'prg', kind: 'rom', from: 'file.prg', battery: false }],
    windows: [],
    nametable: 'vertical',
    registers: [{ bus: 'cpu', range: '$8000-$FFFF', read: false, write: true }],
    state: [],
    settings: []
  };
}

/** 假预加载接口：插件列表保存在闭包里，卸载、停用、安装会真的改它。 */
function setup(options: { list?: () => Promise<{ plugins: MapperSummary[] }>; install?: () => Promise<{ canceled: boolean; plugin?: MapperSummary }> } = {}) {
  let plugins: MapperSummary[] = [NROM, CUSTOM];
  const eventListeners = new Set<(name: string, data: unknown) => void>();
  const statusListeners = new Set<(status: CoreStatus) => void>();
  const list = vi.fn(options.list ?? (() => Promise.resolve({ plugins: [...plugins] })));
  const info = vi.fn((params: { id: string }) => Promise.resolve({ plugin: detailOf(plugins.find((plugin) => plugin.id === params.id) ?? NROM) }));
  const uninstall = vi.fn((params: { id: string }) => {
    plugins = plugins.filter((plugin) => plugin.id !== params.id);
    return Promise.resolve({});
  });
  const setEnabled = vi.fn((params: { id: string; enabled: boolean }) => {
    plugins = plugins.map((plugin) => (plugin.id === params.id ? { ...plugin, enabled: params.enabled } : plugin));
    return Promise.resolve({ plugin: plugins.find((plugin) => plugin.id === params.id) as MapperSummary });
  });
  const openPackage = vi.fn(options.install ?? (() => Promise.resolve({ canceled: true })));
  const bridge = {
    core: { mappers: { list, info, uninstall, setEnabled } },
    main: { mappers: { openPackage } },
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
        <MappersDialog bridge={bridge} />
      </ShellApiContext.Provider>
    </UiProvider>
  );
  return {
    list,
    info,
    uninstall,
    setEnabled,
    openPackage,
    addPlugin: (plugin: MapperSummary) => {
      plugins = [...plugins, plugin];
    },
    emit: (name: string, data: unknown) => act(() => eventListeners.forEach((listener) => listener(name, data))),
    setStatus: (status: CoreStatus) => act(() => statusListeners.forEach((listener) => listener(status)))
  };
}

beforeEach(() => notify.mockClear());

describe('MappersDialog：列表', () => {
  it('表格显示名称、版本、匹配、主机型号、来源与状态', async () => {
    setup();
    const table = await screen.findByRole('table', { name: 'mapper 插件' });
    const nrom = within(table).getByText('NROM').closest('tr') as HTMLElement;
    for (const text of ['1.0.0', 'mapper 0', '经典 FC', '预装', '已启用']) expect(within(nrom).getByText(text)).toBeTruthy();
    const custom = within(table).getByText('自制插件').closest('tr') as HTMLElement;
    for (const text of ['2.1.0', 'mapper 3，变体 1', '用户安装']) expect(within(custom).getByText(text)).toBeTruthy();
  });

  it('搜索框按名称筛选，没有结果时提示', async () => {
    setup();
    await screen.findByText('NROM');
    await userEvent.type(screen.getByRole('searchbox', { name: '搜索插件' }), '自制');
    expect(screen.queryByText('NROM')).toBeNull();
    expect(screen.getByText('自制插件')).toBeTruthy();
    await userEvent.clear(screen.getByRole('searchbox', { name: '搜索插件' }));
    await userEvent.type(screen.getByRole('searchbox', { name: '搜索插件' }), '不存在的');
    expect(screen.getByText('没有符合条件的插件')).toBeTruthy();
  });

  it('来源筛选只显示选中的来源', async () => {
    setup();
    await screen.findByText('NROM');
    await userEvent.click(screen.getByRole('combobox', { name: '来源' }));
    await userEvent.click(await screen.findByRole('option', { name: '用户安装' }));
    expect(screen.queryByText('NROM')).toBeNull();
    expect(screen.getByText('自制插件')).toBeTruthy();
  });

  it('读取失败时显示核心给出的说明', async () => {
    setup({ list: () => Promise.reject({ code: 'core.unavailable', message: '核心未启动' }) });
    expect(await screen.findByText('核心未启动')).toBeTruthy();
  });

  it('别处安装了插件（mappers.changed）后重新读取列表', async () => {
    const page = setup();
    await screen.findByText('NROM');
    page.addPlugin({ ...CUSTOM, id: 'other', name: '另一个插件' });
    page.emit('mappers.changed', { id: 'other' });
    expect(await screen.findByText('另一个插件')).toBeTruthy();
    expect(page.list).toHaveBeenCalledTimes(2);
  });

  it('核心重启就绪后重新读取列表', async () => {
    const page = setup();
    await screen.findByText('NROM');
    page.setStatus({ state: 'ready' } as CoreStatus);
    await waitFor(() => expect(page.list).toHaveBeenCalledTimes(2));
  });
});

describe('MappersDialog：清单摘要与操作', () => {
  it('选中一行后读取并显示清单摘要', async () => {
    const page = setup();
    await userEvent.click(await screen.findByText('NROM'));
    await waitFor(() => expect(page.info).toHaveBeenCalledWith({ id: 'nrom' }));
    expect(await screen.findByText('prg：只读（来自 PRG）')).toBeTruthy();
    expect(screen.getByText('CPU $8000-$FFFF（写）')).toBeTruthy();
  });

  it('预装插件的“卸载”置灰，只能停用；用户安装的可以卸载', async () => {
    setup();
    await userEvent.click(await screen.findByText('NROM'));
    expect((await screen.findByRole('button', { name: '卸载' })).hasAttribute('disabled')).toBe(true);
    expect(screen.getByRole('button', { name: '停用' })).toBeTruthy();
    await userEvent.click(screen.getByText('自制插件'));
    await waitFor(() => expect(screen.getByRole('button', { name: '卸载' }).hasAttribute('disabled')).toBe(false));
  });

  it('停用后状态变为已停用，按钮变成“启用”，再点启用恢复', async () => {
    const page = setup();
    await userEvent.click(await screen.findByText('NROM'));
    await userEvent.click(await screen.findByRole('button', { name: '停用' }));
    expect(page.setEnabled).toHaveBeenCalledWith({ id: 'nrom', enabled: false });
    expect(await screen.findByText('已停用')).toBeTruthy();
    await userEvent.click(screen.getByRole('button', { name: '启用' }));
    expect(page.setEnabled).toHaveBeenLastCalledWith({ id: 'nrom', enabled: true });
    await waitFor(() => expect(screen.queryByText('已停用')).toBeNull());
  });

  it('停用失败时用通知提示，状态保持原样', async () => {
    const page = setup();
    page.setEnabled.mockRejectedValueOnce({ code: 'mappers.notFound', message: '没有这个插件' });
    await userEvent.click(await screen.findByText('NROM'));
    await userEvent.click(await screen.findByRole('button', { name: '停用' }));
    await waitFor(() => expect(notify).toHaveBeenCalledWith('没有这个插件', 'error'));
    expect(screen.queryByText('已停用')).toBeNull();
  });

  it('卸载先确认：确认后卸载并从列表消失、用通知告知；取消则什么都不发生', async () => {
    const page = setup();
    await userEvent.click(await screen.findByText('自制插件'));
    await userEvent.click(await screen.findByRole('button', { name: '卸载' }));
    const cancel = within(await screen.findByRole('alertdialog')).getByRole('button', { name: '取消' });
    await userEvent.click(cancel);
    expect(page.uninstall).not.toHaveBeenCalled();

    await userEvent.click(screen.getByRole('button', { name: '卸载' }));
    await userEvent.click(within(await screen.findByRole('alertdialog')).getByRole('button', { name: '卸载' }));
    await waitFor(() => expect(page.uninstall).toHaveBeenCalledWith({ id: 'custom' }));
    await waitFor(() => expect(screen.queryByText('自制插件')).toBeNull());
    expect(notify).toHaveBeenCalledWith('已卸载插件：自制插件', 'success');
  });
});

describe('MappersDialog：安装', () => {
  it('安装成功：通知告知、新插件出现在列表里并被选中', async () => {
    const added: MapperSummary = { ...CUSTOM, id: 'fresh', name: '新装插件', version: '0.9.0' };
    const page = setup({ install: () => Promise.resolve({ canceled: false, plugin: added }) });
    await screen.findByText('NROM');
    await userEvent.click(screen.getByRole('button', { name: '安装插件' }));
    expect(await within(screen.getByRole('table', { name: 'mapper 插件' })).findByText('新装插件')).toBeTruthy();
    expect(notify).toHaveBeenCalledWith('已安装插件：新装插件 0.9.0', 'success');
    await waitFor(() => expect(page.info).toHaveBeenCalledWith({ id: 'fresh' }));
  });

  it('在选择文件的窗口里取消：没有通知，列表不变', async () => {
    const page = setup();
    await screen.findByText('NROM');
    await userEvent.click(screen.getByRole('button', { name: '安装插件' }));
    await waitFor(() => expect(page.openPackage).toHaveBeenCalledTimes(1));
    expect(notify).not.toHaveBeenCalled();
    expect(screen.getAllByRole('row')).toHaveLength(3);
  });

  it('核心拒绝插件包时用通知给出核心的说明', async () => {
    setup({ install: () => Promise.reject({ code: 'mappers.invalidPackage', message: '插件包缺少 manifest.json' }) });
    await screen.findByText('NROM');
    await userEvent.click(screen.getByRole('button', { name: '安装插件' }));
    await waitFor(() => expect(notify).toHaveBeenCalledWith('插件包缺少 manifest.json', 'error'));
  });
});
